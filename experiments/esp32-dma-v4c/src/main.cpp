#include <Arduino.h>
#include <cstring>
#include "driver/spi_slave.h"
#include "driver/pcnt.h"
#include "esp_heap_caps.h"
#ifdef DMD_CONTINUOUS
#include "esp_system.h"
#endif
#include "soc/gpio_struct.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#ifdef DMD_RD_FILTER_COMPARE
#include "soc/pcnt_struct.h"
#include "esp32-hal-cpu.h"
#if !defined(DMD_RD_TRACE) || !defined(DMD_ATTRACT_SEQUENCE)
#error "RD filter comparison requires RD trace and attract sequence"
#endif
#endif

// Original ESP32 TTGO T-Display. All J603 inputs AFTER level shifters.
constexpr int PIN_RD=26, PIN_RCLK=25, PIN_DCLK=33, PIN_SDATA=13;
constexpr int PIN_CS_OUT=27, PIN_CS_IN=32; // Retain local 27 -> 32 jumper.
constexpr spi_host_device_t HOST=SPI2_HOST;
constexpr pcnt_unit_t ROW_PCNT=PCNT_UNIT_0, DOT_PCNT=PCNT_UNIT_1,
                      RD_PCNT=PCNT_UNIT_2;
constexpr int PCNT_LIMIT=30000;
#ifdef DMD_RD_FILTER_COMPARE
constexpr pcnt_unit_t FILTER_RD_PCNT=PCNT_UNIT_3;
constexpr uint16_t RD_FILTER_CYCLES=80;
static_assert(FILTER_RD_PCNT!=ROW_PCNT && FILTER_RD_PCNT!=DOT_PCNT &&
              FILTER_RD_PCNT!=RD_PCNT,"Comparison PCNT must be independent");
static uint32_t filterSettleCpuCycles=0;
#endif
constexpr size_t FRAME_BYTES=512, BUFFER_BYTES=576;
constexpr int BUFFER_COUNT=8;
constexpr uint32_t TARGET_FRAMES=4096; // About 33.55 s of reception.
constexpr bool EXPECT_STATIC=false; // Paused-pattern run first.
constexpr uint32_t HANDOVER_LIMIT_US=100; // Measured quiet interval ~128 us.

struct Buffer {
    spi_slave_transaction_t trans;
    uint8_t *data;
    uint32_t sequence;
    uint32_t endUs;
#ifdef DMD_AUTO_RECOVERY
    uint32_t captureId,validatedRows;
#ifdef DMD_REPLICA
    uint32_t replicaEpoch;
#endif
#endif
};
static Buffer buffers[BUFFER_COUNT];
static QueueHandle_t completedQueue=nullptr;
static TaskHandle_t acquisitionTask=nullptr;
static spi_slave_transaction_t * volatile armed=nullptr;
static volatile uint32_t armGeneration=0, lastRowCycles=0, armCallbackCycles=0;
static volatile uint32_t processed=0, pixelDifferences=0, workerError=0;
static volatile uint32_t firstCRC=0, lastCRC=0;
static uint32_t cyclesPerUs=240;
static const char *lastOutcome="Acquisition has not started";
static char finalSummary[768]={};

// Startup-only logging. Never call from an ISR or a capture quiet window.
void stage(const char *message) {
    Serial.printf("V4c STARTUP [%lu ms] %s\n", (unsigned long)millis(), message);
    Serial.flush();
}

static inline __attribute__((always_inline)) uint32_t cycles() {
    uint32_t value; asm volatile("rsr.ccount %0" : "=a"(value)); return value;
}
void IRAM_ATTR onSetup(spi_slave_transaction_t *t) {
    const uint32_t now=cycles();
    armed=t;armCallbackCycles=now;
    asm volatile("memw" ::: "memory");
    ++armGeneration;
}
void IRAM_ATTR onRow() {
    lastRowCycles=cycles();
    BaseType_t wake=pdFALSE;
    if (acquisitionTask) vTaskNotifyGiveFromISR(acquisitionTask, &wake);
    if (wake) portYIELD_FROM_ISR();
}
uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc=0xffffffffu;
    for (size_t i=0;i<length;++i) {
        crc^=data[i];
        for (int bit=0;bit<8;++bit)
            crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
[[noreturn]] void fatal(const char *what, esp_err_t err=ESP_OK) {
    GPIO.out_w1ts=1u<<PIN_CS_OUT;
    while(true) {
        Serial.printf("V4c STOP: %s (%s)\n",what,esp_err_to_name(err));
        Serial.flush();delay(2000);
    }
}
void check(esp_err_t e,const char *what) { if(e!=ESP_OK) fatal(what,e); }
void configureCounter(pcnt_unit_t unit,int pin) {
    pcnt_config_t c={};
    c.pulse_gpio_num=pin;c.ctrl_gpio_num=PCNT_PIN_NOT_USED;
    c.unit=unit;c.channel=PCNT_CHANNEL_0;
    c.pos_mode=PCNT_COUNT_INC;c.neg_mode=PCNT_COUNT_DIS;
    c.lctrl_mode=PCNT_MODE_KEEP;c.hctrl_mode=PCNT_MODE_KEEP;
    c.counter_h_lim=PCNT_LIMIT;c.counter_l_lim=0;
    check(pcnt_unit_config(&c),"PCNT setup");
    check(pcnt_filter_disable(unit),"PCNT filter");
    check(pcnt_counter_pause(unit),"PCNT pause");
    check(pcnt_counter_clear(unit),"PCNT clear");
    check(pcnt_counter_resume(unit),"PCNT resume");
}
struct Counts {
    int16_t rows,dots,markers; bool rd;
#ifdef DMD_RD_FILTER_COMPARE
    int16_t filtered,filteredAfter,rawAfter;
    uint32_t rawStart,rawEnd,levelEnd,filteredStart,filteredEnd,settledStart,settledEnd,rawAfterEnd;
#endif
};
bool snapshot(Counts &c) {
#ifdef DMD_RD_FILTER_COMPARE
    if(pcnt_get_counter_value(ROW_PCNT,&c.rows)!=ESP_OK ||
       pcnt_get_counter_value(DOT_PCNT,&c.dots)!=ESP_OK) return false;
    c.rawStart=cycles();
    if(pcnt_get_counter_value(RD_PCNT,&c.markers)!=ESP_OK) return false;
    c.rawEnd=cycles();
    // Keep GPIO sampling ahead of ALL filtered reads; no extra driver read
    // or filter settling delay is inserted before this original level sample.
    c.rd=digitalRead(PIN_RD);c.levelEnd=cycles();
    c.filteredStart=cycles();
    if(pcnt_get_counter_value(FILTER_RD_PCNT,&c.filtered)!=ESP_OK) return false;
    c.filteredEnd=cycles();
    // A second filtered read after >= one filter interval bounds a pending
    // filter response to an edge already observed by the first raw read.
    while(uint32_t(cycles()-c.rawEnd)<filterSettleCpuCycles) {}
    c.settledStart=cycles();
    if(pcnt_get_counter_value(FILTER_RD_PCNT,&c.filteredAfter)!=ESP_OK) return false;
    c.settledEnd=cycles();
    if(pcnt_get_counter_value(RD_PCNT,&c.rawAfter)!=ESP_OK) return false;
    c.rawAfterEnd=cycles();
    return true;
#else
    return pcnt_get_counter_value(ROW_PCNT,&c.rows)==ESP_OK &&
           pcnt_get_counter_value(DOT_PCNT,&c.dots)==ESP_OK &&
           pcnt_get_counter_value(RD_PCNT,&c.markers)==ESP_OK &&
           (c.rd=digitalRead(PIN_RD),true);
#endif
}
#ifdef DMD_RD_TRACE
// Single writer: the acquisition task. Print only after capture has stopped.
constexpr unsigned RD_TRACE_SIZE=64;
static_assert((RD_TRACE_SIZE&(RD_TRACE_SIZE-1))==0,"History size must be a power of two");
struct RdTraceRow {
    Counts counts;
    uint32_t row,notifications,isrCycles,sampleEndCycles;
#ifdef DMD_RD_FILTER_COMPARE
    int rawDelta,filteredDelta;
    bool comparisonAmbiguous;
#endif
};
static RdTraceRow rdHistory[RD_TRACE_SIZE];
static uint32_t rdHistoryWritten=0;

#ifdef DMD_RD_FILTER_COMPARE
void recordFilterRow(RdTraceRow &row);
void printFilterSample(const char *label,const RdTraceRow &row);
#endif
void recordRdRow(const RdTraceRow &row) {
#ifdef DMD_RD_FILTER_COMPARE
    RdTraceRow saved=row;
    recordFilterRow(saved);
    rdHistory[rdHistoryWritten&(RD_TRACE_SIZE-1)]=saved;
#else
    rdHistory[rdHistoryWritten&(RD_TRACE_SIZE-1)]=row;
#endif
    ++rdHistoryWritten;
}

void printRdHistory(const Counts &anchor,bool hasFailure) {
    const uint32_t count=rdHistoryWritten<RD_TRACE_SIZE ? rdHistoryWritten : RD_TRACE_SIZE;
    const uint32_t start=rdHistoryWritten-count;
    Serial.printf("RD TRACE rows=%lu failedRowIncluded=%u cpuMHz=%lu\n",
        (unsigned long)count,unsigned(hasFailure),(unsigned long)cyclesPerUs);
    Serial.printf("RD TRACE anchor RCLK=%d DCLK=%d RD=%d level=%u\n",
        anchor.rows,anchor.dots,anchor.markers,unsigned(anchor.rd));
#ifdef DMD_RD_FILTER_COMPARE
    RdTraceRow anchorRow={anchor,UINT32_MAX,0,0,anchor.levelEnd};
    printFilterSample("ANCHOR",anchorRow);
#endif
    for(uint32_t i=start;i<rdHistoryWritten;++i) {
        const RdTraceRow &r=rdHistory[i&(RD_TRACE_SIZE-1)];
        const bool failed=hasFailure && i+1==rdHistoryWritten;
        Serial.printf("RD ROW index=%lu slot=%lu expected=%u RCLK=%d DCLK=%d RD=%d level=%u "
                      "notify=%lu isrCycles=%lu sampleEndAfterISR=%lu failed=%u\n",
            (unsigned long)r.row,(unsigned long)(r.row%32),unsigned(r.row%32==31),
            r.counts.rows,r.counts.dots,r.counts.markers,unsigned(r.counts.rd),
            (unsigned long)r.notifications,(unsigned long)r.isrCycles,
            (unsigned long)(r.sampleEndCycles-r.isrCycles),unsigned(failed));
#ifdef DMD_RD_FILTER_COMPARE
        printFilterSample("ROW",r);
#endif
    }
}
#endif

// PCNT resets to zero on reaching its configured high limit. Sample at each
// row so at most one wrap is possible; never clear running counters in-stream.
static inline __attribute__((always_inline)) int delta(int current,int previous) {
    return current>=previous ? current-previous : current+PCNT_LIMIT-previous;
}

#ifdef DMD_RD_FILTER_COMPARE
// Diagnostic only. Raw level/edge checks and fastReopen remain authoritative.
static Counts filterPrevious={};
static RdTraceRow filterLast={};
struct FilterTotals {
    uint32_t rows=0,markers=0,rawMarkerOK=0,filteredMarkerOK=0,bothMarkerOK=0;
    uint32_t markerDisagree=0,disagree=0,ambiguous=0,ambiguousMarkers=0;
    uint32_t comparableDisagree=0;
};
static FilterTotals filterTotals;
constexpr unsigned FILTER_EVENTS=16;
struct FilterEvent { RdTraceRow previous,current; };
static FilterEvent filterEvents[FILTER_EVENTS];
static uint32_t filterEventCount=0;

bool sampleMoved(const Counts &c) {
    return delta(c.rawAfter,c.markers)!=0 || delta(c.filteredAfter,c.filtered)!=0;
}
#ifdef DMD_REPLICA
void replicaRawFiltered(unsigned count);
#endif
void recordFilterRow(RdTraceRow &row) {
    row.rawDelta=delta(row.counts.markers,filterPrevious.markers);
    row.filteredDelta=delta(row.counts.filtered,filterPrevious.filtered);
    row.comparisonAmbiguous=sampleMoved(row.counts) || sampleMoved(filterPrevious) ||
        row.notifications!=1 || delta(row.counts.rows,filterPrevious.rows)!=1;
    const bool marker=row.row%32==31;
    const bool disagree=row.rawDelta!=row.filteredDelta;
#ifdef DMD_REPLICA
    if(row.rawDelta>row.filteredDelta && !row.comparisonAmbiguous)replicaRawFiltered(unsigned(row.rawDelta-row.filteredDelta));
#endif
    ++filterTotals.rows;
    filterTotals.ambiguous+=row.comparisonAmbiguous;
    filterTotals.disagree+=disagree;
    filterTotals.comparableDisagree+=disagree && !row.comparisonAmbiguous;
    if(marker) {
        ++filterTotals.markers;
        filterTotals.rawMarkerOK+=row.rawDelta==1;
        filterTotals.filteredMarkerOK+=row.filteredDelta==1;
        filterTotals.bothMarkerOK+=row.rawDelta==1 && row.filteredDelta==1;
        filterTotals.markerDisagree+=disagree;
        filterTotals.ambiguousMarkers+=row.comparisonAmbiguous;
    }
    // Retain early disagreements even when they leave the 64-row tail.
    if(disagree) {
        if(filterEventCount<FILTER_EVENTS) filterEvents[filterEventCount]={filterLast,row};
        ++filterEventCount;
    }
    filterPrevious=row.counts;filterLast=row;
}
void printFilterSample(const char *label,const RdTraceRow &r) {
    const Counts &c=r.counts;
    Serial.printf("RD FILTER %s index=%lu anchor=%u raw=%d filtered=%d rawAfter=%d filteredAfter=%d "
                  "rawDelta=%d filteredDelta=%d expected=%u level=%u ambiguous=%u "
                  "rawStart=%lu rawEnd=%lu levelEnd=%lu filteredStart=%lu filteredEnd=%lu "
                  "settledStart=%lu settledEnd=%lu rawAfterEnd=%lu isrCycles=%lu\n",
        label,(unsigned long)r.row,unsigned(r.row==UINT32_MAX),c.markers,c.filtered,c.rawAfter,c.filteredAfter,
        r.rawDelta,r.filteredDelta,unsigned(r.row!=UINT32_MAX && r.row%32==31),unsigned(c.rd),
        unsigned(r.row==UINT32_MAX ? sampleMoved(c) : r.comparisonAmbiguous),
        (unsigned long)c.rawStart,(unsigned long)c.rawEnd,(unsigned long)c.levelEnd,
        (unsigned long)c.filteredStart,(unsigned long)c.filteredEnd,
        (unsigned long)c.settledStart,(unsigned long)c.settledEnd,(unsigned long)c.rawAfterEnd,
        (unsigned long)r.isrCycles);
}
void printFilterSummary() {
    const auto &t=filterTotals;
    Serial.printf("RD FILTER SUMMARY rows=%lu markers=%lu rawMarkerOK=%lu filteredMarkerOK=%lu bothMarkerOK=%lu "
                  "markerDisagree=%lu disagree=%lu comparableDisagree=%lu ambiguous=%lu ambiguousMarkers=%lu eventsSaved=%lu eventsTotal=%lu\n",
        (unsigned long)t.rows,(unsigned long)t.markers,(unsigned long)t.rawMarkerOK,
        (unsigned long)t.filteredMarkerOK,(unsigned long)t.bothMarkerOK,(unsigned long)t.markerDisagree,
        (unsigned long)t.disagree,(unsigned long)t.comparableDisagree,(unsigned long)t.ambiguous,
        (unsigned long)t.ambiguousMarkers,(unsigned long)(filterEventCount<FILTER_EVENTS ? filterEventCount : FILTER_EVENTS),
        (unsigned long)filterEventCount);
    for(uint32_t i=0;i<filterEventCount && i<FILTER_EVENTS;++i) {
        printFilterSample("EVENT_PREV",filterEvents[i].previous);
        printFilterSample("EVENT",filterEvents[i].current);
#ifdef DMD_FILTER_VALIDATE
        const auto &event=filterEvents[i].current;
        if(event.rawDelta>0 && event.filteredDelta==0)
            Serial.printf("RD RAW_ONLY row=%lu slot=%lu frame=%lu rawDelta=%d filteredDelta=%d ambiguous=%u\n",
                (unsigned long)event.row,(unsigned long)(event.row%32),
                (unsigned long)(event.row/32+1),event.rawDelta,event.filteredDelta,unsigned(event.comparisonAmbiguous));
#endif
    }
    Serial.println("RD FILTER END");
}
void configureFilteredRD() {
    // Source audit: only units 0/1/2 are used elsewhere; original ESP32 only.
    configureCounter(FILTER_RD_PCNT,PIN_RD);
    check(pcnt_counter_pause(FILTER_RD_PCNT),"Filtered RD pause");
    check(pcnt_set_filter_value(FILTER_RD_PCNT,RD_FILTER_CYCLES),"Filtered RD threshold");
    check(pcnt_filter_enable(FILTER_RD_PCNT),"Filtered RD enable");
    uint16_t readback=0;
    check(pcnt_get_filter_value(FILTER_RD_PCNT,&readback),"Filtered RD readback");
    const bool enabled=PCNT.conf_unit[FILTER_RD_PCNT].conf0.filter_en;
    const bool rawEnabled=PCNT.conf_unit[RD_PCNT].conf0.filter_en;
    const uint32_t apb=getApbFrequency();
    if(readback!=RD_FILTER_CYCLES || !enabled || rawEnabled || !apb) fatal("RD filter readback mismatch");
    filterSettleCpuCycles=(uint64_t(RD_FILTER_CYCLES)*getCpuFrequencyMhz()*1000000u+apb-1)/apb;
    check(pcnt_counter_clear(FILTER_RD_PCNT),"Filtered RD clear");
    check(pcnt_counter_resume(FILTER_RD_PCNT),"Filtered RD resume");
    Serial.printf("RD FILTER COMPARE enabled: rawUnit=2 filteredUnit=3 pin=26 limit=%d cycles=%u readback=%u enabled=%u rawFilterEnabled=%u APB_Hz=%lu settleCpuCycles=%lu\n",
        PCNT_LIMIT,unsigned(RD_FILTER_CYCLES),unsigned(readback),unsigned(enabled),unsigned(rawEnabled),
        (unsigned long)apb,(unsigned long)filterSettleCpuCycles);
    Serial.println("RD FILTER ORDER rows,dots,rawStart,raw,rawEnd,GPIO,levelEnd,filteredStart,filtered,filteredEnd,settle,settledStart,filteredAfter,settledEnd,rawAfter,rawAfterEnd; timestamps=CPU_cycles_mod_2^32");
    Serial.println("RD FILTER AMBIGUITY current-or-previous raw/filtered bracket movement or nonunit row/notification; stable brackets exclude observed read-window movement, not unknown waveform shape");
}
#endif

#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "V4c direct PCNT read is for the original ESP32 only"
#endif
static_assert(DOT_PCNT==PCNT_UNIT_1,"Fast read address must match PCNT unit 1");
// Original ESP32 TRM: PCNT_U1_CNT_REG = 0x3FF57064, bits 15:0 count.
// Read-only MMIO. Driver still configures/owns PCNT; no count writes here.
static inline __attribute__((always_inline)) int16_t fastDotCount() {
    return static_cast<int16_t>(*reinterpret_cast<volatile const uint32_t *>(0x3FF57064u)&0xffffu);
}

// Validate the hardware read against bracketing driver reads during startup.
// The counter can advance or wrap between reads, so equality is not required.
bool verifyFastDotCount() {
    for(int i=0;i<16;++i) {
        int16_t before=0,after=0;
        if(pcnt_get_counter_value(DOT_PCNT,&before)!=ESP_OK) return false;
        const int16_t raw=fastDotCount();
        if(pcnt_get_counter_value(DOT_PCNT,&after)!=ESP_OK) return false;
        if(delta(raw,before)>delta(after,before)) return false;
    }
    return true;
}

enum class ReopenError { NONE, NOT_READY, CLOCKS, DEADLINE, LATE_WRITE };
struct ReopenResult {
    spi_slave_transaction_t *next;
    uint32_t generation,callbackOffset,observedOffset,readCycles,reopenOffset,csHighCycles;
    int dotDelta;
    bool callbackSeen,reopened;
#ifdef DMD_CONTINUOUS
    uint32_t deadlineStartCycles;
#endif
};

// Entire readiness -> direct hardware audit -> CS write path lives in IRAM.
// No driver calls, serial, division, queue operations, or pixel processing.
ReopenError IRAM_ATTR fastReopen(uint32_t oldGeneration,int16_t expectedDots,
                               uint32_t budgetCycles,uint32_t closedCycles,
                               bool needNext,ReopenResult &r) {
    const uint32_t epoch=lastRowCycles;
#ifdef DMD_CONTINUOUS
    r.deadlineStartCycles=epoch;
#endif
    r.dotDelta=-1;r.reopened=false;r.callbackSeen=false;
    r.next=nullptr;r.generation=oldGeneration;
    r.reopenOffset=0;r.readCycles=0;r.csHighCycles=0;r.callbackOffset=0;
#ifdef DMD_CONTINUOUS
    uint32_t generation;
    while((generation=armGeneration)==oldGeneration && uint32_t(cycles()-epoch)<budgetCycles) {}
    // Read once more at timeout to detect readiness published at the boundary.
    if(generation==oldGeneration)generation=armGeneration;
    const uint32_t observed=cycles();
    r.generation=generation;
#else
    while(armGeneration==oldGeneration && uint32_t(cycles()-epoch)<budgetCycles) {}
    const uint32_t observed=cycles();
    r.generation=armGeneration;
#endif
    r.observedOffset=observed-epoch;
    r.callbackSeen=r.generation!=oldGeneration;
    if(r.callbackSeen) r.callbackOffset=armCallbackCycles-epoch;
    r.next=armed;
    if(!needNext) return ReopenError::NONE;
    if(!r.callbackSeen || !r.next || r.observedOffset>=budgetCycles)
        return ReopenError::NOT_READY;
    const uint32_t readStart=cycles();
    const int16_t dots=fastDotCount();
    r.readCycles=cycles()-readStart;
    r.dotDelta=delta(dots,expectedDots);
    if(r.dotDelta!=0) {
        r.reopenOffset=readStart+r.readCycles-epoch;
        return ReopenError::CLOCKS;
    }
    const uint32_t beforeWrite=cycles();
    r.reopenOffset=beforeWrite-epoch;
    if(r.reopenOffset>=budgetCycles) return ReopenError::DEADLINE;
    GPIO.out_w1tc=1u<<PIN_CS_OUT;
    const uint32_t afterWrite=cycles();
    r.reopened=true;r.reopenOffset=afterWrite-epoch;
    r.csHighCycles=afterWrite-closedCycles;
    if(r.reopenOffset>=budgetCycles) return ReopenError::LATE_WRITE;
    return ReopenError::NONE;
}

bool initialMarker(Counts &c) {
    Counts previous;
    if(!snapshot(previous)) return false;
    bool sawLow=false;
    const uint32_t start=micros();
    while(uint32_t(micros()-start)<30000) {
        if(!ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(10))) return false;
        delayMicroseconds(10);
        if(!snapshot(c)) return false;
        if(delta(c.rows,previous.rows)!=1) sawLow=false;
        else {
#ifdef DMD_FILTER_VALIDATE
            const int filteredDelta=delta(c.filtered,previous.filtered);
            if(filteredDelta==0) sawLow=true;
            if(filteredDelta==1 && sawLow && c.filtered==c.filteredAfter) return true;
#else
            if(!c.rd) sawLow=true;
            if(c.rd && sawLow) return true;
#endif
        }
        previous=c;
    }
    return false;
}

#ifdef DMD_ATTRACT_SEQUENCE
// The worker records signatures already used by V4c. The acquisition task
// reads the history only once all queued frames have been published.
constexpr uint32_t SEQUENCE_TAIL_FRAMES=32;
static uint32_t sequenceCrc[TARGET_FRAMES];
static uint8_t sequencePixels[SEQUENCE_TAIL_FRAMES][FRAME_BYTES];
static uint32_t sequencePublished=0;

void printSequence(uint32_t frames) {
    if(__atomic_load_n(&sequencePublished,__ATOMIC_ACQUIRE)!=frames) {
        Serial.println("SEQ UNAVAILABLE: frame history did not drain");
        return;
    }
    Serial.printf("SEQ BEGIN frames=%u\n",unsigned(frames));
    constexpr uint32_t CRC_PER_LINE=16;
    for(uint32_t i=0;i<frames;i+=CRC_PER_LINE) {
        Serial.printf("SEQ CRC %u",unsigned(i+1));
        for(uint32_t j=i;j<frames && j<i+CRC_PER_LINE;++j)
            Serial.printf(" %08lX",(unsigned long)sequenceCrc[j]);
        Serial.println();
    }
    const uint32_t first=frames>SEQUENCE_TAIL_FRAMES ? frames-SEQUENCE_TAIL_FRAMES : 0;
    constexpr char HEX_DIGITS[]="0123456789ABCDEF";
    char hex[FRAME_BYTES*2+1];
    hex[FRAME_BYTES*2]='\0';
    for(uint32_t i=first;i<frames;++i) {
        const uint8_t *pixels=sequencePixels[i%SEQUENCE_TAIL_FRAMES];
        for(size_t j=0;j<FRAME_BYTES;++j) {
            hex[2*j]=HEX_DIGITS[pixels[j]>>4];
            hex[2*j+1]=HEX_DIGITS[pixels[j]&0xf];
        }
        Serial.printf("SEQ PIXELS %u %s\n",unsigned(i+1),hex);
    }
    Serial.printf("SEQ END frames=%u\n",unsigned(frames));
}
#endif

#ifdef DMD_AUTO_RECOVERY
#include "recovery_state.h"
#endif
#ifdef DMD_REPLICA
#include "replica_pipeline.h"
void replicaRawFiltered(unsigned count) {replicaFilteredRD+=count;}
#endif
// Only core 0 touches completed pixel buffers. SPI ownership has been returned
// before a Buffer pointer is sent here. Requeue only after CRC/memcmp finish.
void worker(void *) {
    uint8_t reference[FRAME_BYTES];
    bool haveReference=false;
    uint32_t lastSequence=0,lastReportMs=millis();
    while(true) {
        Buffer *b=nullptr;
        if(xQueueReceive(completedQueue,&b,portMAX_DELAY)!=pdTRUE) continue;
        if(b->sequence!=lastSequence+1 || b->trans.trans_len!=FRAME_BYTES*8) {
            workerError=1; continue;
        }
#ifdef DMD_AUTO_RECOVERY
        bool suspect=b->validatedRows!=32;
#ifndef DMD_CONTINUOUS
        for(uint32_t i=0;i<recoveryCount;++i) if(b->captureId==recoveryEvents[i].candidate)suspect=true;
#endif
        if(suspect) {workerError=3;continue;}
#ifndef DMD_CONTINUOUS
        publicationCandidates[b->sequence-1]=b->captureId;
#endif
#endif
#ifdef DMD_REPLICA
        replicaOffer(b->data,b->sequence,b->captureId,b->replicaEpoch,b->endUs);
#endif
        const uint32_t signature=crc32(b->data,FRAME_BYTES);
        if(!haveReference) {
            memcpy(reference,b->data,FRAME_BYTES);
            firstCRC=signature;haveReference=true;
        } else if(EXPECT_STATIC && memcmp(reference,b->data,FRAME_BYTES)!=0) {
            ++pixelDifferences;
        }
        lastCRC=signature;lastSequence=b->sequence;
        const uint32_t completedEndUs=b->endUs;
#if defined(DMD_ATTRACT_SEQUENCE) && !defined(DMD_CONTINUOUS)
        const uint32_t index=lastSequence-1;
        sequenceCrc[index]=signature;
        memcpy(sequencePixels[index%SEQUENCE_TAIL_FRAMES],b->data,FRAME_BYTES);
#endif
        // Rearm before serial reporting so the receiver queue stays supplied.
        memset(b->data,0xa5,BUFFER_BYTES);
        memset(&b->trans,0,sizeof(b->trans));
        b->trans.length=BUFFER_BYTES*8;b->trans.rx_buffer=b->data;b->trans.user=b;
        if(spi_slave_queue_trans(HOST,&b->trans,0)!=ESP_OK) workerError=2;
#if defined(DMD_ATTRACT_SEQUENCE) && !defined(DMD_CONTINUOUS)
#ifdef DMD_AUTO_RECOVERY
        publicationTimes[lastSequence-1]=micros();
#endif
        __atomic_store_n(&sequencePublished,lastSequence,__ATOMIC_RELEASE);
#endif
        asm volatile("memw" ::: "memory");
        processed=lastSequence;
#ifndef DMD_CONTINUOUS
        if(lastSequence==1 || uint32_t(millis()-lastReportMs)>=1000 || lastSequence==TARGET_FRAMES) {
            Serial.printf("V4c processed=%lu/%lu staticDiff=%lu firstCRC=%08lX lastCRC=%08lX elapsed=%lu us\n",
                (unsigned long)lastSequence,(unsigned long)TARGET_FRAMES,
                (unsigned long)pixelDifferences,(unsigned long)firstCRC,
                (unsigned long)lastCRC,(unsigned long)completedEndUs);
            lastReportMs=millis();
        }
#endif
    }
}

void setup() {
#ifdef DMD_FREEZY_EXPORT
    Serial.begin(460800);
#else
    Serial.begin(115200);
#endif
    delay(500);
    stage("01 setup entered; Serial ready; build V4c startup trace");
    digitalWrite(PIN_CS_OUT,HIGH);pinMode(PIN_CS_OUT,OUTPUT);
    pinMode(PIN_CS_IN,INPUT_PULLUP);
    pinMode(PIN_RD,INPUT);pinMode(PIN_RCLK,INPUT);
    pinMode(PIN_DCLK,INPUT);pinMode(PIN_SDATA,INPUT);
    Serial.println("\nESP32 DMA V4c - startup trace and sustained handover");
#ifdef DMD_RD_TRACE
    Serial.println("RD TRACE enabled: last 64 row snapshots; unfiltered counters");
#endif
#if defined(DMD_ATTRACT_SEQUENCE) && !defined(DMD_CONTINUOUS)
    Serial.println("ATTRACT SEQUENCE enabled: full CRC timeline and 32 pixel refreshes");
#endif
#ifdef DMD_FILTER_VALIDATE
    Serial.println("FILTERED VALIDATION enabled: alignment/edges=PCNT3 filtered80; raw PCNT2 and GPIO=observers");
#ifdef DMD_AUTO_RECOVERY
#if !defined(DMD_REPLICA) || defined(DMD_REPLICA_FAULT_TEST)
    Serial.printf("AUTO RECOVERY enabled: INJECTED fault once after 256 validated refreshes at slot=%u; output gaps audited\n",unsigned(DMD_INJECT_SLOT));
#else
    Serial.println("AUTO RECOVERY enabled: genuine faults only; injection disabled; output gaps audited");
#endif
#else
    Serial.println("FILTERED VALIDATION A: no recovery; any validated mismatch stops");
#endif
#endif
#ifdef DMD_CONTINUOUS
    Serial.printf("CONTINUOUS REPLICA enabled: no refresh limit; mode=MOVING core=%d CPU=%u MHz resetReason=%d\n",xPortGetCoreID(),getCpuFrequencyMhz(),int(esp_reset_reason()));
#else
    Serial.printf("mode=%s target=%lu refreshes core=%d CPU=%u MHz\n",
        EXPECT_STATIC?"STATIC":"MOVING",(unsigned long)TARGET_FRAMES,
        xPortGetCoreID(),getCpuFrequencyMhz());
#endif
    Serial.println("RD=26 RCLK=25 DCLK=33 SDATA=13; jumper 27 -> 32 unchanged.");
    stage("02 GPIO setup and banner complete");
    if(xPortGetCoreID()!=1) fatal("Use standard Arduino loop/setup on core 1");
    cyclesPerUs=getCpuFrequencyMhz();
    const uint8_t checkText[]={'1','2','3','4','5','6','7','8','9'};
    if(crc32(checkText,sizeof(checkText))!=0xcbf43926u) fatal("CRC self-check");
#ifdef DMD_REPLICA
    replicaInit();
#endif
    stage("03 core and CRC self-check passed; testing CS jumper");
    digitalWrite(PIN_CS_OUT,LOW);delayMicroseconds(10);
    bool low=digitalRead(PIN_CS_IN)==LOW;
    digitalWrite(PIN_CS_OUT,HIGH);delayMicroseconds(10);
    if(!low || !digitalRead(PIN_CS_IN)) fatal("CS jumper test");
    stage("04 CS jumper passed; preparing SPI configuration");
    spi_bus_config_t bus={};
    bus.mosi_io_num=PIN_SDATA;bus.miso_io_num=-1;bus.sclk_io_num=PIN_DCLK;
    bus.quadwp_io_num=-1;bus.quadhd_io_num=-1;bus.max_transfer_sz=BUFFER_BYTES;
    spi_slave_interface_config_t slave={};
    slave.spics_io_num=PIN_CS_IN;slave.mode=1;slave.queue_size=BUFFER_COUNT;
    slave.post_setup_cb=onSetup;
    stage("05 entering spi_slave_initialize");
    check(spi_slave_initialize(HOST,&bus,&slave,SPI_DMA_CH_AUTO),"SPI init");
    stage("06 SPI init returned; configuring RCLK PCNT");
    configureCounter(ROW_PCNT,PIN_RCLK);
    stage("07 RCLK PCNT ready; configuring DCLK PCNT");
    configureCounter(DOT_PCNT,PIN_DCLK);
    stage("08 DCLK PCNT ready; configuring RD PCNT");
    configureCounter(RD_PCNT,PIN_RD);
#ifdef DMD_RD_FILTER_COMPARE
    configureFilteredRD();
#endif
    stage("09 all counters ready; creating completed-buffer queue");
    stage("09a checking direct DCLK counter read against driver reads");
    if(!verifyFastDotCount()) fatal("Direct DCLK counter self-check failed");
    stage("09b direct DCLK counter check passed");
    completedQueue=xQueueCreate(BUFFER_COUNT,sizeof(Buffer *));
    if(!completedQueue) fatal("Completed queue allocation");
    stage("10 completed queue ready; allocating/queuing eight DMA buffers");
    unsigned bufferIndex=0;
    for(auto &b:buffers) {
        Serial.printf("V4c BUFFER %u/8 allocate begin\n",bufferIndex);
        Serial.flush();
        b.data=static_cast<uint8_t *>(heap_caps_malloc(BUFFER_BYTES,MALLOC_CAP_DMA));
        if(!b.data) fatal("DMA allocation");
        memset(b.data,0xa5,BUFFER_BYTES);
        b.trans={};b.trans.length=BUFFER_BYTES*8;
        b.trans.rx_buffer=b.data;b.trans.user=&b;
        Serial.printf("V4c BUFFER %u/8 allocated %u bytes; queue begin\n",
                      bufferIndex,unsigned(BUFFER_BYTES));
        Serial.flush();
        check(spi_slave_queue_trans(HOST,&b.trans,pdMS_TO_TICKS(100)),"Initial queue");
        Serial.printf("V4c BUFFER %u/8 queue returned\n",bufferIndex);
        Serial.flush();++bufferIndex;
    }
    stage("11 all buffers queued; creating core-0 worker");
#ifdef DMD_REPLICA
    constexpr unsigned workerPriority=3;
#else
    constexpr unsigned workerPriority=1;
#endif
    if(xTaskCreatePinnedToCore(worker,"dmd-check",4096,nullptr,workerPriority,nullptr,0)!=pdPASS)
        fatal("Worker task creation");
#ifdef DMD_WS_EXPORT
    wsInit();
#endif
    stage("12 worker creation returned; setup complete");
#ifdef DMD_CONTINUOUS
    Serial.println("CONTINUOUS START; waiting is normal with J603 off; no automatic ESP32 reset");
#else
    Serial.println("Pause a known pattern. Starting one ~33.55 s run; no per-frame printing.");
#endif
    Serial.flush();
}

#ifdef DMD_CONTINUOUS
#include "continuous_loop.h"
#elif defined(DMD_AUTO_RECOVERY)
#include "recovery_loop.h"
#else
void loop() {
    static bool ran=false;
    if(ran) {
        Serial.printf("V4c IDLE: %s; reset ESP32 to repeat\n",lastOutcome);
        if(finalSummary[0]) Serial.println(finalSummary);
        Serial.flush();delay(2000);return;
    }
    ran=true;
    stage("20 loop entered; acquisition preparing");
    stage("21 priority change and RCLK interrupt attachment begin");
    const UBaseType_t savedPriority=uxTaskPriorityGet(nullptr);
    vTaskPrioritySet(nullptr,configMAX_PRIORITIES-4);
    acquisitionTask=xTaskGetCurrentTaskHandle();
    attachInterrupt(digitalPinToInterrupt(PIN_RCLK),onRow,RISING);
    stage("22 RCLK interrupt attached; acquisition starting at next clean RD marker");
    // Logging ends here. Do not print between finding RD and opening CS.
    Counts previous={};
    const char *issue=nullptr;
    if(!initialMarker(previous)) issue="Initial RD marker timeout";
#ifdef DMD_RD_TRACE
    const Counts rdAnchor=previous;
#ifdef DMD_RD_FILTER_COMPARE
    filterPrevious=previous;
    filterLast={previous,UINT32_MAX,0,0,previous.levelEnd};
#endif
    bool rdFailedRow=false;
#endif
    uint32_t frames=0,rows=0,markers=0,dotEdges=0,maxHandover=0,maxCsHigh=0;
    uint32_t maxReopenUs=0,lastArmUs=0,lastReopenUs=0;
    int lastHandoverDots=-1;
    bool lastReopened=false,lastCallbackSeen=false;
    uint32_t lastCallbackUs=0,maxCallbackUs=0,lastReadCycles=0,maxReadCycles=0;
    uint32_t lastCsClosedCycles=0;
    const uint32_t handoverBudgetCycles=HANDOVER_LIMIT_US*cyclesPerUs;
    uint32_t opened=0,lastClosed=0;
    int lastRowDelta=0,lastDotDelta=0,lastMarkerDelta=0;
    spi_slave_transaction_t *current=nullptr;
    uint32_t generation=0;
    if(!issue) {
        current=armed;generation=armGeneration;
        Counts c;
        if(!current || !snapshot(c) || c.rows!=previous.rows || c.dots!=previous.dots)
            issue="Missed initial quiet interval";
        else if(uint32_t(cycles()-lastRowCycles)/cyclesPerUs>=HANDOVER_LIMIT_US)
            issue="Initial start deadline";
        else {
            opened=micros();GPIO.out_w1tc=1u<<PIN_CS_OUT;
        }
    }
    while(!issue && frames<TARGET_FRAMES) {
        // Blocking on RCLK notification lets the idle task run between rows.
        // No global interrupt masking; SPI completion ISR stays enabled.
        const uint32_t notifications=ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(10));
        if(!notifications) {issue="RCLK notification timeout";break;}
        delayMicroseconds(10);
        const bool endFrame=(rows%32==31);
        if(endFrame) {GPIO.out_w1ts=1u<<PIN_CS_OUT;lastCsClosedCycles=cycles();lastClosed=micros();}
#ifdef DMD_RD_TRACE
        const uint32_t rowIsrCycles=lastRowCycles;
#endif
        Counts c;
        if(!snapshot(c)) {issue="Counter read error";break;}
#ifdef DMD_RD_TRACE
        const RdTraceRow traceRow={c,rows,notifications,rowIsrCycles,
#ifdef DMD_RD_FILTER_COMPARE
            c.levelEnd
#else
            cycles()
#endif
        };
#endif
        lastRowDelta=delta(c.rows,previous.rows);
        lastDotDelta=delta(c.dots,previous.dots);
#ifdef DMD_FILTER_VALIDATE
        lastMarkerDelta=delta(c.filtered,previous.filtered);
#else
        lastMarkerDelta=delta(c.markers,previous.markers);
#endif
        if(notifications!=1 || lastRowDelta!=1 || lastDotDelta!=128) {
#ifdef DMD_RD_TRACE
            GPIO.out_w1ts=1u<<PIN_CS_OUT;
            recordRdRow(traceRow);rdFailedRow=true;
#endif
            issue="Row continuity mismatch";break;
        }
#ifdef DMD_FILTER_VALIDATE
        if(lastMarkerDelta!=(endFrame?1:0) || c.filtered!=c.filteredAfter) {
#else
        if(c.rd!=endFrame || lastMarkerDelta!=(endFrame?1:0)) {
#endif
#ifdef DMD_RD_TRACE
            GPIO.out_w1ts=1u<<PIN_CS_OUT;
            recordRdRow(traceRow);rdFailedRow=true;
#endif
            issue="RD phase mismatch";break;
        }
        ++rows;dotEdges+=lastDotDelta;markers+=lastMarkerDelta;
        previous=c;
        if(!endFrame) {
#ifdef DMD_RD_TRACE
            recordRdRow(traceRow);
#endif
            continue;
        }
        // Keep the old descriptor identity while selecting the next receiver.
        spi_slave_transaction_t *completed=current;
        const bool needNext=(frames+1<TARGET_FRAMES);
        const char *handoverIssue=nullptr;
        ReopenResult timing;
        const ReopenError reopenError=fastReopen(generation,previous.dots,
            handoverBudgetCycles,lastCsClosedCycles,needNext,timing);
#ifdef DMD_RD_TRACE
        // Do not write the history ring before opening the next transaction.
        recordRdRow(traceRow);
#endif
        // All formatting and unit conversion happen AFTER the CS write.
        lastArmUs=timing.observedOffset/cyclesPerUs;
        lastCallbackSeen=timing.callbackSeen;
        lastCallbackUs=timing.callbackSeen ? timing.callbackOffset/cyclesPerUs : 0;
        lastReadCycles=timing.readCycles;
        lastReopenUs=timing.reopenOffset/cyclesPerUs;
        lastHandoverDots=timing.dotDelta;
        lastReopened=timing.reopened;
        if(lastArmUs>maxHandover) maxHandover=lastArmUs;
        if(lastCallbackSeen && lastCallbackUs>maxCallbackUs) maxCallbackUs=lastCallbackUs;
        if(lastReadCycles>maxReadCycles) maxReadCycles=lastReadCycles;
        if(timing.reopened) {
            current=timing.next;generation=timing.generation;
            if(lastReopenUs>maxReopenUs) maxReopenUs=lastReopenUs;
            const uint32_t csHigh=timing.csHighCycles/cyclesPerUs;
            if(csHigh>maxCsHigh) maxCsHigh=csHigh;
        }
        switch(reopenError) {
            case ReopenError::NONE: break;
            case ReopenError::NOT_READY: handoverIssue="Next DMA buffer missed quiet deadline";break;
            case ReopenError::CLOCKS: handoverIssue="DCLK edges while CS high";break;
            case ReopenError::DEADLINE: handoverIssue="CS reopen deadline";break;
            case ReopenError::LATE_WRITE: handoverIssue="CS reopened after deadline";break;
        }
        // CS is already low for the next refresh when handover succeeded.
        // Independent row/DCLK/RD deltas continue from the SAME snapshot on
        // the next notification. Counters are never cleared during the run.
        spi_slave_transaction_t *finished=nullptr;
        const esp_err_t result=spi_slave_get_trans_result(HOST,&finished,0);
        if(result!=ESP_OK || finished!=completed) {
            issue="SPI completion/ownership error";break;
        }
        if(finished->trans_len!=FRAME_BYTES*8) {issue="SPI bit length mismatch";break;}
        Buffer *b=static_cast<Buffer *>(finished->user);
        b->sequence=++frames;b->endUs=lastClosed-opened;
        if(xQueueSend(completedQueue,&b,0)!=pdTRUE) {issue="Completed buffer queue full";break;}
        if(handoverIssue) {issue=handoverIssue;break;}
        if(workerError) {issue="Worker queue or sequence error";break;}
        if(EXPECT_STATIC && pixelDifferences) {issue="Paused pixel data changed";break;}

    }
    GPIO.out_w1ts=1u<<PIN_CS_OUT;
    const uint32_t stopped=micros();
    detachInterrupt(digitalPinToInterrupt(PIN_RCLK));acquisitionTask=nullptr;
    vTaskPrioritySet(nullptr,savedPriority);
    const uint32_t drainStart=millis();
    while(processed<frames && !workerError && uint32_t(millis()-drainStart)<2000) delay(1);
    Serial.printf("\nV4c FINAL frames=%lu/%lu processed=%lu rows=%lu DCLK=%lu RDedges=%lu elapsed=%lu us maxArmWait=%lu us maxCSHigh=%lu us\n",
        (unsigned long)frames,(unsigned long)TARGET_FRAMES,(unsigned long)processed,
        (unsigned long)rows,(unsigned long)dotEdges,(unsigned long)markers,
        (unsigned long)(opened ? stopped-opened : 0),(unsigned long)maxHandover,
        (unsigned long)maxCsHigh);
    Serial.printf("Handover: lastArm=%lu us lastReopen=%lu us maxReopen=%lu us reopened=%d DCLKwhileHigh=%d deadline=%lu us\n",
        (unsigned long)lastArmUs,(unsigned long)lastReopenUs,
        (unsigned long)maxReopenUs,lastReopened,lastHandoverDots,
        (unsigned long)HANDOVER_LIMIT_US);
    Serial.printf("Timing: callbackSeen=%d callback=%lu us maxCallback=%lu us observedReady=%lu us directRead=%lu cycles maxDirectRead=%lu cycles\n",
        lastCallbackSeen,(unsigned long)lastCallbackUs,(unsigned long)maxCallbackUs,
        (unsigned long)lastArmUs,(unsigned long)lastReadCycles,(unsigned long)maxReadCycles);
    Serial.printf("Last deltas: RCLK=%d DCLK=%d RD=%d | staticDiff=%lu workerError=%lu\n",
        lastRowDelta,lastDotDelta,lastMarkerDelta,(unsigned long)pixelDifferences,
        (unsigned long)workerError);
    if(!issue && (frames!=TARGET_FRAMES || rows!=TARGET_FRAMES*32 ||
                  dotEdges!=TARGET_FRAMES*4096 || markers!=TARGET_FRAMES))
        issue="Final continuity totals failed";
    if(!issue && (processed!=frames || workerError || (EXPECT_STATIC && pixelDifferences)))
        issue="Worker validation failed";
    snprintf(finalSummary,sizeof(finalSummary),
        "Saved final: frames=%lu/%lu processed=%lu rows=%lu DCLK=%lu RD=%lu\n"
        "Saved handover: arm=%lu us reopen=%lu us maxReopen=%lu us reopened=%d DCLKwhileHigh=%d\n"
        "Saved timing: callbackSeen=%d callback=%lu us observed=%lu us directRead=%lu cycles maxDirectRead=%lu cycles\n"
        "Saved deltas: RCLK=%d DCLK=%d RD=%d staticDiff=%lu workerError=%lu",
        (unsigned long)frames,(unsigned long)TARGET_FRAMES,(unsigned long)processed,
        (unsigned long)rows,(unsigned long)dotEdges,(unsigned long)markers,
        (unsigned long)lastArmUs,(unsigned long)lastReopenUs,
        (unsigned long)maxReopenUs,lastReopened,lastHandoverDots,
        lastCallbackSeen,(unsigned long)lastCallbackUs,(unsigned long)lastArmUs,
        (unsigned long)lastReadCycles,(unsigned long)maxReadCycles,
        lastRowDelta,lastDotDelta,lastMarkerDelta,
        (unsigned long)pixelDifferences,(unsigned long)workerError);
    lastOutcome=issue ? issue : "STREAM PASS";
    if(issue) Serial.printf("STOP: %s\n",issue);
    else Serial.printf("RESULT: %s STREAM PASS - all %lu buffer transactions checked\n",
        EXPECT_STATIC?"STATIC":"MOVING COUNTS/RD",(unsigned long)frames);
#ifdef DMD_RD_TRACE
    if(rdHistoryWritten) printRdHistory(rdAnchor,rdFailedRow);
#endif
#ifdef DMD_RD_FILTER_COMPARE
    printFilterSummary();
#endif
#ifdef DMD_ATTRACT_SEQUENCE
    printSequence(frames);
#endif
    Serial.println("Run ended. Reset for another run. No automatic resync hides discontinuities.");
    Serial.flush();
}
#endif // DMD_AUTO_RECOVERY
