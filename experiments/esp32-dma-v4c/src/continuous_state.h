#pragma once
#include "signal_session.h"
static SignalSession contSession;
static portMUX_TYPE contMux=portMUX_INITIALIZER_UNLOCKED;
static volatile bool contWaiting=true,contStopped=false;
static volatile uint32_t contFrames=0,contMaxReopen=0,contMaxAlignment=0,contStaleBinary=0,contEventLost=0;
static uint32_t contLcdEpoch=0;
static bool contHaveLcd=false;
static QueueHandle_t contEventQueue=nullptr;
struct ContEvent {const char *kind;uint32_t us,episode,epoch,a,b,c,d;};
static void contEmit(const char *kind,uint32_t a=0,uint32_t b=0,uint32_t c=0,uint32_t d=0) {
    ContEvent e;portENTER_CRITICAL(&contMux);e={kind,micros(),contSession.episode,replicaEpoch,a,b,c,d};portEXIT_CRITICAL(&contMux);
    if(!contEventQueue || xQueueSend(contEventQueue,&e,0)!=pdTRUE)++contEventLost;
}
static void contGrayReady(const GreyRecord &record) {
    bool accepted=false,recovered=false;uint32_t detected=0,episode=0;
    portENTER_CRITICAL(&contMux);
    if(record.epoch==replicaEpoch && contSession.waiting) {
        recovered=contSession.goodGroup();accepted=true;detected=contSession.returnUs;episode=contSession.episode;
        replicaRecoveries=contSession.recoveries;
    }
    portEXIT_CRITICAL(&contMux);
    if(accepted)contEmit(recovered?"RESUMED_GRAY":"STARTED_GRAY",record.n,record.crc,
                         uint32_t(micros()-detected),episode);
}
static void contLcdReady(const GreyRecord &record,uint32_t drawEnd) {
    if(record.epoch==replicaEpoch && (!contHaveLcd || record.epoch!=contLcdEpoch)) {
        contLcdEpoch=record.epoch;contHaveLcd=true;contWaiting=false;
        contEmit("LCD_RESUMED",record.n,record.crc,drawEnd,uint32_t(drawEnd-contSession.returnUs));
    }
}
static void contTelemetryTask(void *) {
    uint32_t last=0;ContEvent e;
    while(true) {
        if(xQueueReceive(contEventQueue,&e,pdMS_TO_TICKS(50))==pdTRUE)
            Serial.printf("CONT EVENT kind=%s us=%lu episode=%lu epoch=%lu a=%lu b=%lu c=%lu d=%lu\n",
                e.kind,(unsigned long)e.us,(unsigned long)e.episode,(unsigned long)e.epoch,
                (unsigned long)e.a,(unsigned long)e.b,(unsigned long)e.c,(unsigned long)e.d);
        if(uint32_t(millis()-last)>=1000) {
            SignalSession s;portENTER_CRITICAL(&contMux);s=contSession;portEXIT_CRITICAL(&contMux);
            Serial.printf("CONT STATUS us=%lu state=%s frames=%lu gray=%lu emitted=%lu crcLost=%lu Errors=%lu Losses=%lu Recoveries=%lu FilteredRD=%lu attempts=%lu maxReopen=%lu maxAlignment=%lu binaryDrops=%lu rendererDrops=%lu staleBinary=%lu eventLost=%lu partial=%u partialDiscarded=%lu LCDn=%lu LCDcrc=%08lX\n",
                (unsigned long)micros(),contStopped?"STOPPED":(contWaiting?"WAITING":"ACTIVE"),
                (unsigned long)contFrames,(unsigned long)replicaBuilt,(unsigned long)replicaCrcEmitted,
                (unsigned long)replicaCrcLost,(unsigned long)s.errors,(unsigned long)s.losses,
                (unsigned long)s.recoveries,(unsigned long)replicaFilteredRD,(unsigned long)s.attempts,
                (unsigned long)contMaxReopen,(unsigned long)contMaxAlignment,(unsigned long)replicaBinaryDrops,
                (unsigned long)replicaRenderDrops,(unsigned long)contStaleBinary,(unsigned long)contEventLost,
                replicaDecoder->partial,(unsigned long)replicaDecoder->partialDiscards,
                (unsigned long)replicaLcdN,(unsigned long)replicaLcdCRC);
            last=millis();
        }
    }
}
static void contInit() {
    contEventQueue=xQueueCreate(32,sizeof(ContEvent));
    if(!contEventQueue || xTaskCreatePinnedToCore(contTelemetryTask,"dmd-state",4096,nullptr,1,nullptr,0)!=pdPASS)
        fatal("Continuous telemetry allocation");
    contEmit("WAITING_STARTUP");
}
