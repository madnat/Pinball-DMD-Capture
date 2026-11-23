#pragma once
#include <TFT_eSPI.h>
#include <new>
#include "grey_decoder.h"
#if defined(USE_HSPI_PORT) || defined(USE_FSPI_PORT)
#error "Replica TFT must use VSPI/SPI3, not capture SPI2"
#endif
static_assert(SPI_PORT==VSPI && VSPI==3 && HOST==SPI2_HOST,"SPI host isolation required");
static TFT_eSPI replicaTft;
struct ReplicaBinary {uint8_t data[512];uint32_t sequence,candidate,epoch,endUs;};
struct GreyRecord {uint32_t n,us,epoch,first,last,crc;uint16_t histogram[4];};
struct GreyImage {GreyRecord record;uint8_t values[4096];};
#ifdef DMD_FREEZY_EXPORT
#include "freezy_export.h"
#endif
#ifdef DMD_WS_EXPORT
#include "dmd_ws_sender.h"
#endif
struct ReplicaHold {
    uint32_t detectedUs,heldN,heldCRC,lastCompleteUs,firstNewN,drawStartUs,drawEndUs;
};
static ReplicaBinary replicaBinaryPool[12];
static QueueHandle_t replicaFree,replicaInput,replicaCrcQueue,replicaRenderQueue;
static GreyDecoder *replicaDecoder=nullptr;
static volatile uint32_t replicaEpoch=0,replicaErrors=0,replicaRecoveries=0,replicaInjected=0,replicaFilteredRD=0;
static volatile uint32_t replicaBinaryDrops=0,replicaRenderDrops=0,replicaCrcLost=0,replicaCrcEmitted=0;
static volatile uint32_t replicaOffered=0,replicaConsumed=0,replicaBuilt=0,replicaRendered=0,replicaLatestCRC=0;
static volatile uint32_t replicaLastDrawUs=0,replicaLcdN=0,replicaLcdCRC=0,replicaRendererBusy=0,replicaDecoderBusy=0;
static volatile uint32_t replicaStatusErrors=0,replicaStatusRecoveries=0,replicaStatusFiltered=0;
static ReplicaHold replicaHolds[MAX_RECOVERIES];
static uint8_t replicaLagHistory[6][512];
static uint64_t replicaLagBits[4]={};
static uint32_t replicaLagSamples[4]={},replicaAllShades=0,replicaStableRun=0,replicaMaxStableRun=0;
static GreyRecord replicaLastRecord={};
static uint16_t *replicaRgb=nullptr;

#ifdef DMD_CONTINUOUS
#include "continuous_state.h"
#endif
void replicaFault(unsigned index,uint32_t detected,bool injected) {
    ++replicaErrors;replicaInjected+=injected;
    if(index<MAX_RECOVERIES)replicaHolds[index].detectedUs=detected;
    __atomic_add_fetch(&replicaEpoch,1,__ATOMIC_RELEASE);
}
void replicaOffer(const uint8_t *data,uint32_t sequence,uint32_t candidate,uint32_t epoch,uint32_t endUs) {
    ReplicaBinary *slot=nullptr;
    if(xQueueReceive(replicaFree,&slot,0)!=pdTRUE) {++replicaBinaryDrops;return;}
    memcpy(slot->data,data,512);slot->sequence=sequence;slot->candidate=candidate;slot->epoch=epoch;slot->endUs=endUs;
    if(xQueueSend(replicaInput,&slot,0)!=pdTRUE) {++replicaBinaryDrops;xQueueSend(replicaFree,&slot,0);return;}
    ++replicaOffered;
}
void replicaDecoderTask(void *) {
    static GreyImage image;
    uint32_t previousCandidate=0,previousEpoch=0,consecutive=0,lastCRC=0;
    while(true) {
        ReplicaBinary *slot=nullptr;
#ifdef DMD_CONTINUOUS
        const uint32_t live=__atomic_load_n(&replicaEpoch,__ATOMIC_ACQUIRE);
        if(replicaDecoder->havePrevious && replicaDecoder->epoch!=live)replicaDecoder->gap();
        if(xQueueReceive(replicaInput,&slot,pdMS_TO_TICKS(10))!=pdTRUE)continue;
        if(slot->epoch!=__atomic_load_n(&replicaEpoch,__ATOMIC_ACQUIRE)) {
            xQueueSend(replicaFree,&slot,0);++replicaConsumed;++contStaleBinary;continue;
        }
#else
        if(xQueueReceive(replicaInput,&slot,portMAX_DELAY)!=pdTRUE)continue;
#endif
        replicaDecoderBusy=1;
        if(slot->candidate!=previousCandidate+1 || slot->epoch!=previousEpoch)consecutive=0;
        const unsigned lags[4]={1,2,3,6};
        for(unsigned k=0;k<4;++k)if(consecutive>=lags[k]) {
            const uint8_t *old=replicaLagHistory[(slot->candidate-lags[k])%6];
            uint32_t bits=0;
            for(unsigned i=0;i<512;++i)bits+=__builtin_popcount(unsigned(old[i]^slot->data[i]));
            replicaLagBits[k]+=bits;++replicaLagSamples[k];
        }
        memcpy(replicaLagHistory[slot->candidate%6],slot->data,512);
        ++consecutive;previousCandidate=slot->candidate;previousEpoch=slot->epoch;
        const bool complete=replicaDecoder->push(slot->data,slot->candidate,slot->epoch);
        const uint32_t candidate=slot->candidate,epoch=slot->epoch;
#if defined(DMD_FREEZY_EXPORT) || defined(DMD_WS_EXPORT)
        const uint32_t acquiredUs=slot->endUs;
#endif
        xQueueSend(replicaFree,&slot,0);++replicaConsumed;
        if(!complete) {replicaDecoderBusy=0;continue;}
        GreyRecord record={};record.n=++replicaBuilt;record.us=micros();record.epoch=epoch;
        record.first=replicaDecoder->firstCandidate;record.last=candidate;
        memcpy(image.values,replicaDecoder->image,4096);
        record.crc=greyCRC32(image.values);
        for(unsigned i=0;i<4096;++i)++record.histogram[image.values[i]];
        replicaAllShades+=record.histogram[0] && record.histogram[1] && record.histogram[2] && record.histogram[3];
        replicaStableRun=record.n>1 && record.crc==lastCRC ? replicaStableRun+1 : 1;
        if(replicaStableRun>replicaMaxStableRun)replicaMaxStableRun=replicaStableRun;
        lastCRC=record.crc;replicaLatestCRC=record.crc;replicaLastRecord=record;image.record=record;
#ifdef DMD_FREEZY_EXPORT
        freezyOffer(image,acquiredUs);
#endif
#ifdef DMD_WS_EXPORT
        wsOffer(image.values,acquiredUs);
#endif
        // Record every image, including identical CRCs. No UART writes here.
#ifdef DMD_CONTINUOUS
        contGrayReady(record);
#endif
#ifndef DMD_FREEZY_EXPORT
        if(xQueueSend(replicaCrcQueue,&record,0)!=pdTRUE)++replicaCrcLost;
#endif
        if(uxQueueMessagesWaiting(replicaRenderQueue))++replicaRenderDrops;
        xQueueOverwrite(replicaRenderQueue,&image);replicaDecoderBusy=0;
    }
}
#ifndef DMD_FREEZY_EXPORT
void replicaSerialTask(void *) {
    GreyRecord r;
    while(true)if(xQueueReceive(replicaCrcQueue,&r,portMAX_DELAY)==pdTRUE) {
        Serial.printf("GRAY n=%lu us=%lu epoch=%lu first=%lu last=%lu crc=%08lX hist=%u,%u,%u,%u\n",
            (unsigned long)r.n,(unsigned long)r.us,(unsigned long)r.epoch,(unsigned long)r.first,(unsigned long)r.last,
            (unsigned long)r.crc,r.histogram[0],r.histogram[1],r.histogram[2],r.histogram[3]);
        ++replicaCrcEmitted;
    }
}
#endif
void replicaRendererTask(void *) {
    static GreyImage image;
    uint32_t lastStatus=0,lastEpoch=0;
    while(true) {
        if(xQueueReceive(replicaRenderQueue,&image,pdMS_TO_TICKS(20))==pdTRUE) {
            if(image.record.epoch!=__atomic_load_n(&replicaEpoch,__ATOMIC_ACQUIRE)) {++replicaRenderDrops;continue;}
            replicaRendererBusy=1;
            for(unsigned i=0;i<4096;++i)replicaRgb[i]=GREY_PALETTE[image.values[i]];
            const uint32_t start=micros();
            if(image.record.epoch!=lastEpoch && image.record.epoch<=MAX_RECOVERIES) {
                auto &h=replicaHolds[image.record.epoch-1];
                h.heldN=replicaLcdN;h.heldCRC=replicaLcdCRC;h.lastCompleteUs=replicaLastDrawUs;
                h.firstNewN=image.record.n;h.drawStartUs=start;
            }
            replicaTft.startWrite();replicaTft.pushImage(56,51,128,32,replicaRgb);replicaTft.endWrite();
            replicaLastDrawUs=micros();replicaLcdN=image.record.n;replicaLcdCRC=image.record.crc;++replicaRendered;
            if(image.record.epoch!=lastEpoch && image.record.epoch<=MAX_RECOVERIES)
                replicaHolds[image.record.epoch-1].drawEndUs=replicaLastDrawUs;
#ifdef DMD_CONTINUOUS
            contLcdReady(image.record,replicaLastDrawUs);
#endif
            lastEpoch=image.record.epoch;replicaRendererBusy=0;
        }
        if(uint32_t(millis()-lastStatus)>=100) {
            replicaTft.setTextColor(TFT_WHITE,TFT_BLACK);replicaTft.setTextSize(1);
#ifdef DMD_CONTINUOUS
            replicaTft.fillRect(0,16,240,10,TFT_BLACK);replicaTft.setCursor(2,17);
            replicaTft.print(contStopped?"Capture fault - see serial":(contWaiting?"Waiting for DMD":"DMD active"));
#endif
            replicaTft.fillRect(0,90,240,45,TFT_BLACK);
            replicaTft.setCursor(2,91);replicaTft.printf("Errors:%lu Recoveries:%lu",(unsigned long)replicaErrors,(unsigned long)replicaRecoveries);
            replicaTft.setCursor(2,102);
#ifdef DMD_CONTINUOUS
            replicaTft.printf("Filtered RD:%lu Loss:%lu",(unsigned long)replicaFilteredRD,(unsigned long)contSession.losses);
#else
            replicaTft.printf("Filtered RD:%lu",(unsigned long)replicaFilteredRD);
#endif
            replicaTft.setCursor(2,113);replicaTft.printf("LCD drops:%lu Serial lost:%lu",(unsigned long)replicaRenderDrops,(unsigned long)replicaCrcLost);
            replicaTft.setCursor(2,124);replicaTft.printf("CRC:%08lX #%lu",(unsigned long)replicaLatestCRC,(unsigned long)replicaBuilt);
            replicaStatusErrors=replicaErrors;replicaStatusRecoveries=replicaRecoveries;replicaStatusFiltered=replicaFilteredRD;
            lastStatus=millis();
        }
    }
}
void replicaInit() {
    void *decoderMemory=heap_caps_malloc(sizeof(GreyDecoder),MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
    replicaRgb=static_cast<uint16_t *>(heap_caps_malloc(8192,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));
    if(!decoderMemory || !replicaRgb)fatal("Replica bounded work-buffer allocation");
    replicaDecoder=new(decoderMemory)GreyDecoder();
    pinMode(TFT_BL,OUTPUT);digitalWrite(TFT_BL,HIGH);
    replicaTft.init();replicaTft.setRotation(1);replicaTft.setSwapBytes(true);replicaTft.fillScreen(TFT_BLACK);
    replicaTft.setTextColor(TFT_WHITE,TFT_BLACK);replicaTft.setTextSize(1);replicaTft.setCursor(2,4);
#ifdef DMD_REPLICA_FAULT_TEST
    replicaTft.print("DMD REPLICA INJECTED TEST");
#else
#ifdef DMD_CONTINUOUS
    replicaTft.print("CONTINUOUS DMD REPLICA");
#else
    replicaTft.print("DMD REPLICA");
#endif
#endif
    replicaTft.setCursor(2,17);replicaTft.print("3-refresh SUM; phase unverified");
    for(unsigned i=0;i<4;++i)replicaTft.fillRect(56+i*32,36,32,6,GREY_PALETTE[i]);
    replicaFree=xQueueCreate(12,sizeof(ReplicaBinary *));replicaInput=xQueueCreate(12,sizeof(ReplicaBinary *));
    replicaCrcQueue=xQueueCreate(64,sizeof(GreyRecord));replicaRenderQueue=xQueueCreate(1,sizeof(GreyImage));
    if(!replicaFree || !replicaInput || !replicaCrcQueue || !replicaRenderQueue)fatal("Replica queue allocation");
    for(auto &b:replicaBinaryPool) {ReplicaBinary *p=&b;xQueueSend(replicaFree,&p,0);}
#ifdef DMD_CONTINUOUS
    contInit();
#endif
    if(xTaskCreatePinnedToCore(replicaDecoderTask,"gray-decode",6144,nullptr,2,nullptr,0)!=pdPASS ||
#ifndef DMD_FREEZY_EXPORT
       xTaskCreatePinnedToCore(replicaSerialTask,"gray-serial",3072,nullptr,1,nullptr,0)!=pdPASS ||
#endif
       xTaskCreatePinnedToCore(replicaRendererTask,"gray-lcd",6144,nullptr,1,nullptr,0)!=pdPASS)fatal("Replica task allocation");
#ifdef DMD_FREEZY_EXPORT
    freezyInit();
#endif
    Serial.printf("DMD REPLICA enabled: captureHost=%d SPI2; TFT SPI_PORT=%d VSPI/SPI3; ST7789 240x135 rotation=1; origin=56,51 scale=1,1; TFT_DMA=0\n",int(HOST),int(SPI_PORT));
    Serial.println("GRAY CONFIG group=3 equal-refresh-sum intensity=0..3 row-major-MSB-first crc=CRC32_IEEE_4096_bytes phase=UNVERIFIED; queues binary=12 crc=64 lcd=1");
}
void replicaFinish() {
    const uint32_t start=millis();
    while(uint32_t(millis()-start)<3000) {
        if(replicaConsumed==replicaOffered && !replicaDecoderBusy &&
#ifndef DMD_FREEZY_EXPORT
           !uxQueueMessagesWaiting(replicaCrcQueue) && replicaCrcEmitted+replicaCrcLost==replicaBuilt &&
#endif
           !uxQueueMessagesWaiting(replicaRenderQueue) && !replicaRendererBusy)break;
        delay(1);
    }
#ifdef DMD_FREEZY_EXPORT
    freezyDrain(replicaBuilt);
#endif
    delay(120); // allow status fields to reflect the final counters, outside acquisition.
}
void replicaReport() {
#ifdef DMD_FREEZY_EXPORT
    freezyReport(replicaBuilt,replicaBinaryDrops);
#endif
    Serial.printf("REPLICA SUMMARY grayBuilt=%lu crcEmitted=%lu crcLost=%lu binaryDrops=%lu rendererDrops=%lu rendered=%lu Errors=%lu Recoveries=%lu FilteredRD=%lu injected=%lu partialDiscarded=%lu partialRemaining=%u allFourImages=%lu maxStableRun=%lu\n",
        (unsigned long)replicaBuilt,(unsigned long)replicaCrcEmitted,(unsigned long)replicaCrcLost,
        (unsigned long)replicaBinaryDrops,(unsigned long)replicaRenderDrops,(unsigned long)replicaRendered,
        (unsigned long)replicaErrors,(unsigned long)replicaRecoveries,(unsigned long)replicaFilteredRD,
        (unsigned long)replicaInjected,(unsigned long)replicaDecoder->partialDiscards,replicaDecoder->partial,
        (unsigned long)replicaAllShades,(unsigned long)replicaMaxStableRun);
    Serial.printf("LCD STATUS Errors=%lu Recoveries=%lu FilteredRD=%lu image=%lu imageCRC=%08lX latestCRC=%08lX\n",
        (unsigned long)replicaStatusErrors,(unsigned long)replicaStatusRecoveries,(unsigned long)replicaStatusFiltered,
        (unsigned long)replicaLcdN,(unsigned long)replicaLcdCRC,(unsigned long)replicaLatestCRC);
    for(unsigned i=0;i<recoveryCount;++i) {
        const auto &h=replicaHolds[i];
        Serial.printf("GRAY HOLD id=%u detectedUs=%lu heldN=%lu heldCRC=%08lX lastCompleteUs=%lu firstNewN=%lu drawStartUs=%lu drawEndUs=%lu holdToStartUs=%lu holdToCompleteUs=%lu detectionToDrawUs=%lu\n",
            i,(unsigned long)h.detectedUs,(unsigned long)h.heldN,(unsigned long)h.heldCRC,(unsigned long)h.lastCompleteUs,
            (unsigned long)h.firstNewN,(unsigned long)h.drawStartUs,(unsigned long)h.drawEndUs,
            (unsigned long)(h.drawStartUs-h.lastCompleteUs),(unsigned long)(h.drawEndUs-h.lastCompleteUs),
            (unsigned long)(h.drawStartUs-h.detectedUs));
    }
    const unsigned lags[4]={1,2,3,6};
    for(unsigned i=0;i<4;++i)Serial.printf("GRAY PERIOD lag=%u samples=%lu changedBits=%llu\n",lags[i],(unsigned long)replicaLagSamples[i],(unsigned long long)replicaLagBits[i]);
    Serial.printf("GRAY TAIL n=%lu first=%lu last=%lu epoch=%lu crc=%08lX values=",
        (unsigned long)replicaLastRecord.n,(unsigned long)replicaLastRecord.first,(unsigned long)replicaLastRecord.last,
        (unsigned long)replicaLastRecord.epoch,(unsigned long)replicaLastRecord.crc);
    // One ASCII digit per intensity (0..3); this is evidence, not CRC input encoding.
    for(unsigned i=0;i<4096;++i)Serial.write('0'+replicaDecoder->image[i]);
    Serial.println();Serial.println("REPLICA END");
}
