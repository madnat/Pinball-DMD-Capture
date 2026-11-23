#pragma once
static Buffer *contOwned(spi_slave_transaction_t *t) {
    for(auto &b:buffers)if(&b.trans==t)return &b;
    return nullptr;
}
static bool contOpen(Counts &previous,spi_slave_transaction_t *&current,uint32_t &generation,bool &dirty) {
    dirty=false;current=armed;generation=armGeneration;Counts c;
    if(!current || !contOwned(current) || !snapshot(c) || c.rows!=previous.rows || c.dots!=previous.dots ||
       c.filtered!=previous.filtered || c.filtered!=c.filteredAfter)return false;
    if(uint32_t(cycles()-lastRowCycles)>=HANDOVER_LIMIT_US*cyclesPerUs)return false;
    dirty=true;GPIO.out_w1tc=1u<<PIN_CS_OUT;
    const uint32_t elapsed=cycles()-lastRowCycles;
    if(elapsed>=HANDOVER_LIMIT_US*cyclesPerUs) {GPIO.out_w1ts=1u<<PIN_CS_OUT;return false;}
    const uint32_t us=elapsed/cyclesPerUs;if(us>contMaxAlignment)contMaxAlignment=us;
    dirty=false;return true;
}
// 0: still driver-owned, 1: reclaimed cleanly, -1: ownership is unsafe.
static int contDiscard(spi_slave_transaction_t *discarded,uint32_t generation,bool probe) {
    GPIO.out_w1ts=1u<<PIN_CS_OUT;
    spi_slave_transaction_t *finished=nullptr;
    esp_err_t result=spi_slave_get_trans_result(HOST,&finished,0);
    if(result!=ESP_OK && result!=ESP_ERR_TIMEOUT)return -1;
    if(result==ESP_ERR_TIMEOUT && probe) {
        // A zero-clock transaction may not finish on CS alone. After clocks return,
        // clock only the still-owned suspect descriptor until the first DCLK move.
        const int16_t before=fastDotCount();const uint32_t start=micros();
        GPIO.out_w1tc=1u<<PIN_CS_OUT;
        while(fastDotCount()==before && uint32_t(micros()-start)<1000) {}
        GPIO.out_w1ts=1u<<PIN_CS_OUT;
        result=spi_slave_get_trans_result(HOST,&finished,pdMS_TO_TICKS(2));
        contEmit("DISCARD_PROBE",uint32_t(micros()-start));
    }
    if(result==ESP_ERR_TIMEOUT)return 0;
    if(result!=ESP_OK || finished!=discarded)return -1;
    Buffer *b=contOwned(finished);
    if(!b || finished->user!=b)return -1;
    const uint32_t bits=finished->trans_len;
    // A second unseen setup generation could contain clocked, unvalidated data.
    const uint32_t ready=micros();
    while(armGeneration==generation && uint32_t(micros()-ready)<2000)taskYIELD();
    if(armGeneration-generation!=1 || !armed || armed==discarded || !contOwned(armed))return -1;
    memset(b->data,0xa5,BUFFER_BYTES);memset(&b->trans,0,sizeof(b->trans));
    b->trans.length=BUFFER_BYTES*8;b->trans.rx_buffer=b->data;b->trans.user=b;
    if(spi_slave_queue_trans(HOST,&b->trans,0)!=ESP_OK)return -1;
    contEmit("DISCARDED_DMA",bits,probe,armGeneration);return 1;
}
static void contEnterWait(bool silent,const char *reason) {
    GPIO.out_w1ts=1u<<PIN_CS_OUT;
    bool fresh=false,marked=false;
    portENTER_CRITICAL(&contMux);
    const uint32_t oldLosses=contSession.losses;
    fresh=contSession.enterWait(silent);marked=contSession.losses!=oldLosses;
    replicaErrors=contSession.errors;contWaiting=true;
    portEXIT_CRITICAL(&contMux);
    __atomic_add_fetch(&replicaEpoch,1,__ATOMIC_RELEASE);
    contEmit(fresh?"WAITING_OUTAGE":(contSession.hasImage?"WAITING_RETRY":"WAITING_STARTUP_RETRY"),processed,replicaBuilt,replicaLatestCRC,silent);
    if(marked)contEmit("SIGNAL_LOSS",contSession.losses);
    (void)reason;
}
struct ContHandoverFailure {
    uint32_t transaction, candidate, elapsedCycles, bitLength;
    int rowDelta, dotDelta, handoverDotDelta;
    esp_err_t spiResult;
    bool bitsKnown, dotSampled;
    uint32_t deadlineStart, callbackEntry, generationSample;
    bool callbackSeen;
};

static const char *contHandoverReason(ReopenError error,const ReopenResult &timing) {
    switch(error) {
        case ReopenError::NOT_READY:
            if(!timing.callbackSeen)return "SPI setup callback not observed by deadline";
            if(!timing.next)return "SPI setup callback has no armed descriptor";
            return "SPI setup callback observed too late";
        case ReopenError::CLOCKS:return "DCLK advanced while CS was high";
        case ReopenError::DEADLINE:return "deadline exceeded before CS reopen";
        case ReopenError::LATE_WRITE:return "deadline exceeded at CS reopen";
        case ReopenError::NONE:return nullptr;
    }
    return "unknown handover error";
}

void loop() {
    static bool ran=false;if(ran){delay(1000);return;}ran=true;
    stage("20 continuous replica entered; no refresh limit");
    const UBaseType_t saved=uxTaskPriorityGet(nullptr);
    vTaskPrioritySet(nullptr,configMAX_PRIORITIES-4);
    acquisitionTask=xTaskGetCurrentTaskHandle();attachInterrupt(digitalPinToInterrupt(PIN_RCLK),onRow,RISING);
    stage("22 continuous signal monitor attached; startup silence is waiting");
    uint32_t frames=0,candidate=1,phase=0,generation=0;
    bool active=false,pending=false;
    spi_slave_transaction_t *current=nullptr,*discarded=nullptr;
    uint32_t discardedGeneration=0;
    Counts previous={},monitor={};snapshot(monitor);
    uint32_t lastActivity=micros();const char *fatalReason=nullptr;
    ContHandoverFailure handoverFailure={};
    while(!fatalReason) {
        if(!active) {
            if(pending) {
                const int result=contDiscard(discarded,discardedGeneration,false);
                if(result<0){fatalReason="DMA ownership unsafe while waiting";break;}
                if(result>0)pending=false;
            }
            vTaskDelay(pdMS_TO_TICKS(5));Counts now={};
            if(!snapshot(now)){fatalReason="Counter read error while waiting";break;}
            const int dr=delta(now.rows,monitor.rows),dd=delta(now.dots,monitor.dots);
            monitor=now;
            if(!dr || dd<128) {
                if(uint32_t(micros()-lastActivity)>10000) {
                    bool marked=false;portENTER_CRITICAL(&contMux);marked=contSession.markSilent();portEXIT_CRITICAL(&contMux);
                    if(marked)contEmit("SIGNAL_LOSS",contSession.losses);
                }
                continue;
            }
            lastActivity=micros();
            portENTER_CRITICAL(&contMux);contSession.returned(lastActivity);portEXIT_CRITICAL(&contMux);
            contEmit("SIGNAL_RETURN",dr,dd,replicaBuilt,contSession.attempts);
            if(pending) {
                const int result=contDiscard(discarded,discardedGeneration,true);
                if(result<0){fatalReason="DMA ownership unsafe after signal return";break;}
                if(!result)continue;
                pending=false;
            }
            ulTaskNotifyTake(pdTRUE,0);
            if(!initialMarker(previous)){contEmit("ALIGN_RETRY",1);snapshot(monitor);continue;}
            bool dirty=false;
            if(!contOpen(previous,current,generation,dirty)) {
                if(dirty){pending=true;discarded=current;discardedGeneration=generation;++candidate;}
                contEmit("ALIGN_RETRY",2);snapshot(monitor);continue;
            }
            filterPrevious=previous;filterLast={previous,UINT32_MAX,0,0,previous.levelEnd};
            phase=0;active=true;
            contEmit("ALIGNED",previous.rows,previous.dots,previous.filtered,contMaxAlignment);
            continue;
        }
        const uint32_t notifications=ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(10));
        if(!notifications) {
            contEnterWait(true,"RCLK timeout");pending=true;discarded=current;discardedGeneration=generation;
            ++candidate;active=false;snapshot(monitor);lastActivity=micros();continue;
        }
        delayMicroseconds(10);const bool endFrame=phase==31;
        uint32_t closedCycles=0,closedUs=0;
        if(endFrame){GPIO.out_w1ts=1u<<PIN_CS_OUT;closedCycles=cycles();closedUs=micros();}
        const uint32_t isr=lastRowCycles;Counts c;
        if(!snapshot(c)){fatalReason="Counter read error";break;}
        const int dr=delta(c.rows,previous.rows),dd=delta(c.dots,previous.dots),df=delta(c.filtered,previous.filtered);
        const RdTraceRow row={c,frames*32+phase,notifications,isr,c.levelEnd};
        if(notifications!=1 || dr!=1 || dd!=128 || df!=(endFrame?1:0) || c.filtered!=c.filteredAfter) {
            contEnterWait(false,"Clock or filtered RD validation mismatch");
            contEmit("VALIDATION_FAULT",phase,dr,dd,df);recordRdRow(row);
            pending=true;discarded=current;discardedGeneration=generation;++candidate;active=false;
            snapshot(monitor);lastActivity=micros();continue;
        }
        previous=c;
        if(!endFrame){recordRdRow(row);++phase;continue;}
        spi_slave_transaction_t *completed=current;ReopenResult timing;
        const ReopenError error=fastReopen(generation,previous.dots,HANDOVER_LIMIT_US*cyclesPerUs,closedCycles,true,timing);
        recordRdRow(row);
        if(timing.reopened){current=timing.next;generation=timing.generation;
            const uint32_t us=timing.reopenOffset/cyclesPerUs;if(us>contMaxReopen)contMaxReopen=us;}
        spi_slave_transaction_t *finished=nullptr;
        const esp_err_t spiResult=spi_slave_get_trans_result(HOST,&finished,0);
        const bool bitsKnown=spiResult==ESP_OK && finished==completed && contOwned(finished);
        if(!bitsKnown || finished->trans_len!=FRAME_BYTES*8 || error!=ReopenError::NONE) {
            if(spiResult!=ESP_OK)fatalReason="SPI completion result unavailable";
            else if(!bitsKnown)fatalReason="SPI completion descriptor mismatch";
            else if(finished->trans_len!=FRAME_BYTES*8)fatalReason="SPI transaction bit length mismatch";
            else fatalReason=contHandoverReason(error,timing);
            const bool dotSampled=error!=ReopenError::NOT_READY;
            const uint32_t elapsed=dotSampled?timing.reopenOffset:timing.observedOffset;
            handoverFailure={frames+1,candidate,elapsed,
                bitsKnown?uint32_t(finished->trans_len):0,dr,dd,timing.dotDelta,spiResult,bitsKnown,dotSampled,
                timing.deadlineStartCycles,
                timing.callbackSeen?timing.deadlineStartCycles+timing.callbackOffset:0,
                timing.deadlineStartCycles+timing.observedOffset,timing.callbackSeen};
            break;
        }
        Buffer *b=contOwned(finished);b->captureId=candidate;b->validatedRows=32;b->replicaEpoch=replicaEpoch;
        b->sequence=++frames;b->endUs=closedUs;
        if(xQueueSend(completedQueue,&b,0)!=pdTRUE){fatalReason="Completed buffer queue full";break;}
        contFrames=frames;++candidate;phase=0;
        if(workerError){fatalReason="Worker validation error";break;}
    }
    GPIO.out_w1ts=1u<<PIN_CS_OUT;detachInterrupt(digitalPinToInterrupt(PIN_RCLK));acquisitionTask=nullptr;
    vTaskPrioritySet(nullptr,saved);contStopped=true;
    portENTER_CRITICAL(&contMux);contSession.enterWait(false);replicaErrors=contSession.errors;contWaiting=true;portEXIT_CRITICAL(&contMux);
    if(handoverFailure.transaction) {
        const auto &f=handoverFailure;
        Serial.printf("CONT HANDOVER_FAIL transaction=%lu candidate=%lu reason=\"%s\" "
                      "handoverCycles=%lu cyclesPerUs=%lu limitUs=%lu "
                      "rowExpected=1 rowObserved=%d dotExpected=128 dotObserved=%d "
                      "csHighDotExpected=0 csHighDotObserved=%d csHighDotSampled=%u "
                      "bitsExpected=%u bitsObserved=%lu bitsKnown=%u spiResult=%d "
                      "deadlineStartCycle=%lu callbackEntryCycle=%lu generationSampleCycle=%lu callbackSeen=%u\n",
                      (unsigned long)f.transaction,(unsigned long)f.candidate,fatalReason,
                      (unsigned long)f.elapsedCycles,(unsigned long)cyclesPerUs,(unsigned long)HANDOVER_LIMIT_US,
                      f.rowDelta,f.dotDelta,f.handoverDotDelta,unsigned(f.dotSampled),
                      unsigned(FRAME_BYTES*8),(unsigned long)f.bitLength,unsigned(f.bitsKnown),int(f.spiResult),
                      (unsigned long)f.deadlineStart,(unsigned long)f.callbackEntry,
                      (unsigned long)f.generationSample,unsigned(f.callbackSeen));
    }
    contEmit("FATAL",0);Serial.printf("CONT FATAL reason=%s\n",fatalReason);Serial.flush();
}
