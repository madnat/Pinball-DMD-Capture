// Separate opt-in loop. No failed refresh enters completedQueue.
static Buffer *ownedBuffer(spi_slave_transaction_t *t) {
    for(auto &b:buffers) if(&b.trans==t) return &b;
    return nullptr;
}
static bool openAligned(Counts &previous,spi_slave_transaction_t *&current,
                        uint32_t &generation,uint32_t &maxStartUs) {
    current=armed;generation=armGeneration;
    Counts now;
    if(!current || !ownedBuffer(current) || !snapshot(now) ||
       now.rows!=previous.rows || now.dots!=previous.dots ||
       now.filtered!=previous.filtered || now.filtered!=now.filteredAfter) return false;
    const uint32_t before=cycles();
    if(uint32_t(before-lastRowCycles)>=HANDOVER_LIMIT_US*cyclesPerUs) return false;
    GPIO.out_w1tc=1u<<PIN_CS_OUT;
    const uint32_t elapsed=cycles()-lastRowCycles;
    if(elapsed>=HANDOVER_LIMIT_US*cyclesPerUs) {GPIO.out_w1ts=1u<<PIN_CS_OUT;return false;}
    const uint32_t us=elapsed/cyclesPerUs;
    if(us>maxStartUs)maxStartUs=us;
    return true;
}
static const char *recoverCandidate(RecoveryEvent &event,Counts &previous,
                                   spi_slave_transaction_t *&current,uint32_t &generation,
                                   uint32_t &maxStartUs) {
    // Close first; driver receives the completed partial transaction and arms next.
    GPIO.out_w1ts=1u<<PIN_CS_OUT;
    spi_slave_transaction_t *discarded=current,*finished=nullptr;
    if(spi_slave_get_trans_result(HOST,&finished,pdMS_TO_TICKS(2))!=ESP_OK || finished!=discarded)
        return "Recovery could not reclaim suspect descriptor";
    Buffer *b=ownedBuffer(finished);
    if(!b || finished->user!=b)return "Recovery descriptor identity mismatch";
    event.reclaimed=true;event.reclaimedUs=micros();event.droppedBits=finished->trans_len;
    // The descriptor and its RX data are ours only after get_trans_result.
    memset(b->data,0xa5,BUFFER_BYTES);memset(&b->trans,0,sizeof(b->trans));
    b->trans.length=BUFFER_BYTES*8;b->trans.rx_buffer=b->data;b->trans.user=b;
    if(spi_slave_queue_trans(HOST,&b->trans,0)!=ESP_OK)return "Recovery discard requeue failed";
    const uint32_t readyStart=micros();
    while(armGeneration==generation && uint32_t(micros()-readyStart)<2000) taskYIELD();
    if(armGeneration==generation || !armed || armed==discarded || !ownedBuffer(armed))
        return "Recovery next DMA ownership not established";
    event.newGeneration=armGeneration;
    // No in-stream counter clears: deliberately declare a new alignment epoch.
    ulTaskNotifyTake(pdTRUE,0);
    if(!initialMarker(previous))return "Recovery filtered alignment timeout";
    event.anchor=previous;
    filterPrevious=previous;filterLast={previous,UINT32_MAX,0,0,previous.levelEnd};
    if(!openAligned(previous,current,generation,maxStartUs))return "Recovery missed safe opening deadline";
    event.aligned=true;event.syncUs=micros();
    return nullptr;
}
void loop() {
    static bool ran=false;
    if(ran) {Serial.printf("RECOVERY IDLE outcome=%s\n",lastOutcome);Serial.flush();delay(2000);return;}
    ran=true;
    stage("20 loop entered; recovery diagnostic preparing");
    const UBaseType_t priority=uxTaskPriorityGet(nullptr);
    vTaskPrioritySet(nullptr,configMAX_PRIORITIES-4);
    acquisitionTask=xTaskGetCurrentTaskHandle();
    attachInterrupt(digitalPinToInterrupt(PIN_RCLK),onRow,RISING);
    stage("22 RCLK interrupt attached; filtered acquisition starting");
    Counts previous={},anchor={};
    const char *issue=nullptr;
    uint32_t frames=0,phase=0,candidate=1,lastGoodEndUs=0,maxReopenUs=0,maxStartUs=0;
    uint32_t lastClosed=0,startedUs=0,maxCsHigh=0,maxArm=0,lastArm=0,lastReopen=0;
    uint32_t lastCallback=0,maxCallback=0,lastRead=0,maxRead=0;
    bool lastReopened=false,lastCallbackSeen=false,injected=false;
    int lastDots=-1;
    spi_slave_transaction_t *current=nullptr;
    uint32_t generation=0;
    if(!initialMarker(previous)) issue="Initial filtered marker timeout";
    if(!issue) {
        anchor=previous;filterPrevious=previous;filterLast={previous,UINT32_MAX,0,0,previous.levelEnd};
        if(!openAligned(previous,current,generation,maxStartUs))issue="Initial start deadline/ownership";
        else startedUs=micros();
    }
    while(!issue && frames<TARGET_FRAMES) {
        const uint32_t notifications=ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(10));
        delayMicroseconds(10);
        const bool endFrame=phase==31;
        uint32_t closedCycles=0;
        if(endFrame) {GPIO.out_w1ts=1u<<PIN_CS_OUT;closedCycles=cycles();lastClosed=micros();}
        const uint32_t isr=lastRowCycles;
        Counts c={};
        if(!snapshot(c)) {issue="Counter read error";break;}
        const int dr=delta(c.rows,previous.rows),dd=delta(c.dots,previous.dots);
        const int df=delta(c.filtered,previous.filtered),raw=delta(c.markers,previous.markers);
        const RdTraceRow row={c,frames*32+phase,notifications,isr,c.levelEnd};
        const char *fault=nullptr;
        bool injectNow=false;
        if(notifications!=1 || dr!=1 || dd!=128)fault="Clock/notification validation mismatch";
        else if(df!=(endFrame?1:0) || c.filtered!=c.filteredAfter)fault="Filtered RD validation mismatch";
#if !defined(DMD_REPLICA) || defined(DMD_REPLICA_FAULT_TEST)
        else if(!injected && frames>=256 && processed==frames && phase==DMD_INJECT_SLOT) {
            fault="INJECTED validation failure";injectNow=true;injected=true;
        }
#endif
        if(fault) {
            const uint32_t detected=micros();
            GPIO.out_w1ts=1u<<PIN_CS_OUT;
            recordRdRow(row);
            if(recoveryCount>=MAX_RECOVERIES) {issue="Recovery event capacity exceeded";break;}
            RecoveryEvent &event=recoveryEvents[recoveryCount];
            event={};event.detectedUs=detected;event.lastGoodEndUs=lastGoodEndUs;
            event.publishedBefore=frames;event.candidate=candidate;event.frame=frames+1;event.slot=phase;
            event.before=previous;event.failed=c;event.rowDelta=dr;event.dotDelta=dd;
            event.filteredDelta=df;event.rawDelta=raw;event.cause=fault;event.injected=injectNow;
#ifdef DMD_REPLICA
            replicaFault(recoveryCount,detected,injectNow);
#endif
            asm volatile("memw" ::: "memory");++recoveryCount;
            issue=recoverCandidate(event,previous,current,generation,maxStartUs);
            if(issue)break;
#ifdef DMD_REPLICA
            ++replicaRecoveries;
#endif
            anchor=previous;++candidate;phase=0;
            continue;
        }
        previous=c;
        if(!endFrame) {recordRdRow(row);++phase;continue;}
        spi_slave_transaction_t *completed=current;
        ReopenResult timing;
        const ReopenError error=fastReopen(generation,previous.dots,
            HANDOVER_LIMIT_US*cyclesPerUs,closedCycles,frames+1<TARGET_FRAMES,timing);
        recordRdRow(row);
        lastArm=timing.observedOffset/cyclesPerUs;lastReopen=timing.reopenOffset/cyclesPerUs;
        lastCallback=timing.callbackOffset/cyclesPerUs;lastCallbackSeen=timing.callbackSeen;
        lastRead=timing.readCycles;lastReopened=timing.reopened;lastDots=timing.dotDelta;
        if(lastArm>maxArm)maxArm=lastArm;
        if(lastCallback>maxCallback)maxCallback=lastCallback;
        if(lastRead>maxRead)maxRead=lastRead;
        if(timing.reopened) {
            current=timing.next;generation=timing.generation;
            if(lastReopen>maxReopenUs)maxReopenUs=lastReopen;
            if(timing.csHighCycles/cyclesPerUs>maxCsHigh)maxCsHigh=timing.csHighCycles/cyclesPerUs;
        }
        // Ownership/length checks are never bypassed, even after an injected fault.
        spi_slave_transaction_t *finished=nullptr;
        if(spi_slave_get_trans_result(HOST,&finished,0)!=ESP_OK || finished!=completed) {
            issue="SPI completion/ownership error";break;
        }
        if(finished->trans_len!=FRAME_BYTES*8) {issue="SPI bit length mismatch";break;}
        Buffer *b=ownedBuffer(finished);
        if(!b || finished->user!=b) {issue="Buffer identity mismatch";break;}
        if(error!=ReopenError::NONE) {
            // No further output is published after a handover validation failure.
            issue="Handover validation/deadline failure";break;
        }
        b->captureId=candidate;b->validatedRows=32;
#ifdef DMD_REPLICA
        b->replicaEpoch=replicaEpoch;
#endif
        b->sequence=++frames;b->endUs=lastClosed-startedUs;
        const uint32_t validatedAt=micros();
        for(uint32_t i=0;i<recoveryCount;++i) if(!recoveryEvents[i].firstGoodSequence) {
            recoveryEvents[i].firstGoodSequence=frames;
            recoveryEvents[i].firstGoodEndUs=lastClosed;
            recoveryEvents[i].firstGoodValidatedUs=validatedAt;
        }
        if(xQueueSend(completedQueue,&b,0)!=pdTRUE) {issue="Completed buffer queue full";break;}
        lastGoodEndUs=lastClosed;++candidate;phase=0;
        if(workerError) {issue="Worker validation error";break;}
    }
    GPIO.out_w1ts=1u<<PIN_CS_OUT;
    const uint32_t ended=micros();
    detachInterrupt(digitalPinToInterrupt(PIN_RCLK));acquisitionTask=nullptr;
    vTaskPrioritySet(nullptr,priority);
    const uint32_t drain=millis();
    while(processed<frames && !workerError && uint32_t(millis()-drain)<2000)delay(1);
    if(!issue && (frames!=TARGET_FRAMES || processed!=frames || workerError))issue="Worker/output totals invalid";
#if !defined(DMD_REPLICA) || defined(DMD_REPLICA_FAULT_TEST)
    if(!issue && !injected)issue="Injection was not exercised";
#endif
#ifdef DMD_REPLICA
    if(issue && (!recoveryCount || recoveryEvents[recoveryCount-1].aligned))++replicaErrors;
    replicaFinish();
#endif
    Serial.printf("\nV4c FINAL frames=%lu/4096 processed=%lu rows=%lu DCLK=%lu RDedges=%lu elapsed=%lu us maxArmWait=%lu us maxCSHigh=%lu us\n",
        (unsigned long)frames,(unsigned long)processed,(unsigned long)(frames*32),
        (unsigned long)(frames*4096),(unsigned long)frames,(unsigned long)(ended-startedUs),
        (unsigned long)maxArm,(unsigned long)maxCsHigh);
    Serial.printf("Handover: lastArm=%lu us lastReopen=%lu us maxReopen=%lu us reopened=%u DCLKwhileHigh=%d deadline=100 us\n",
        (unsigned long)lastArm,(unsigned long)lastReopen,(unsigned long)maxReopenUs,unsigned(lastReopened),lastDots);
    Serial.printf("Timing: callbackSeen=%u callback=%lu us maxCallback=%lu us observedReady=%lu us directRead=%lu cycles maxDirectRead=%lu cycles\n",
        unsigned(lastCallbackSeen),(unsigned long)lastCallback,(unsigned long)maxCallback,(unsigned long)lastArm,
        (unsigned long)lastRead,(unsigned long)maxRead);
    Serial.printf("RECOVERY TOTALS validatedRefreshes=%lu discarded=%lu maxAlignmentOpen=%lu deadline=100 countersNeverCleared=1\n",
        (unsigned long)frames,(unsigned long)recoveryCount,(unsigned long)maxStartUs);
    for(uint32_t i=0;i<recoveryCount;++i) {
        const auto &e=recoveryEvents[i];
        const uint32_t first=e.firstGoodSequence;
        const uint32_t published=first && processed>=first ? publicationTimes[first-1] : 0;
        const uint32_t last=e.publishedBefore && processed>=e.publishedBefore ? publicationTimes[e.publishedBefore-1] : 0;
        Serial.printf("RECOVERY EVENT id=%lu injected=%u cause=%s candidate=%lu frame=%lu slot=%lu rawDelta=%d filteredDelta=%d RCLKdelta=%d DCLKdelta=%d level=%u reclaimed=%u droppedBits=%lu aligned=%u generation=%lu\n",
            (unsigned long)i,unsigned(e.injected),e.cause,(unsigned long)e.candidate,(unsigned long)e.frame,(unsigned long)e.slot,
            e.rawDelta,e.filteredDelta,e.rowDelta,e.dotDelta,unsigned(e.failed.rd),unsigned(e.reclaimed),
            (unsigned long)e.droppedBits,unsigned(e.aligned),(unsigned long)e.newGeneration);
        Serial.printf("RECOVERY TIMING id=%lu detectedUs=%lu reclaimedUs=%lu syncUs=%lu lastGoodEndUs=%lu firstGoodSequence=%lu firstGoodCandidate=%lu firstGoodEndUs=%lu firstGoodValidatedUs=%lu firstGoodPublishedUs=%lu lastGoodPublishedUs=%lu detectionToGoodUs=%lu goodGapUs=%lu detectionToPublishUs=%lu publicationGapUs=%lu\n",
            (unsigned long)i,(unsigned long)e.detectedUs,(unsigned long)e.reclaimedUs,(unsigned long)e.syncUs,
            (unsigned long)e.lastGoodEndUs,(unsigned long)first,(unsigned long)(first ? publicationCandidates[first-1] : 0),
            (unsigned long)e.firstGoodEndUs,(unsigned long)e.firstGoodValidatedUs,(unsigned long)published,(unsigned long)last,
            (unsigned long)(e.firstGoodEndUs-e.detectedUs),(unsigned long)(e.firstGoodEndUs-e.lastGoodEndUs),
            (unsigned long)(published-e.detectedUs),(unsigned long)(published-last));
        Serial.printf("RECOVERY ANCHOR id=%lu RCLK=%d DCLK=%d rawRD=%d filteredRD=%d level=%u\n",
            (unsigned long)i,e.anchor.rows,e.anchor.dots,e.anchor.markers,e.anchor.filtered,unsigned(e.anchor.rd));
    }
    Serial.printf("PUB BEGIN frames=%lu\n",(unsigned long)processed);
    for(uint32_t i=0;i<processed;i+=16) {
        Serial.printf("PUB IDS %lu",(unsigned long)(i+1));
        for(uint32_t j=i;j<processed && j<i+16;++j)Serial.printf(" %lu",(unsigned long)publicationCandidates[j]);
        Serial.println();
    }
    Serial.printf("PUB END frames=%lu\n",(unsigned long)processed);
    lastOutcome=issue?issue:"RECOVERY VALIDATED OUTPUT PASS";
    if(issue)Serial.printf("STOP: %s\n",issue);
#ifdef DMD_REPLICA
    else Serial.println("RESULT: REPLICA VALIDATED OUTPUT PASS - grayscale groups require consecutive validated refreshes");
#else
    else Serial.println("RESULT: RECOVERY VALIDATED OUTPUT PASS - discontinuities explicitly discarded and logged");
#endif
    if(rdHistoryWritten)printRdHistory(anchor,false);
    printFilterSummary();printSequence(frames);
#ifdef DMD_REPLICA
    replicaReport();
#endif
    Serial.println("RECOVERY END; no ESP32 reset during recovery; final counts describe validated output only");
    Serial.flush();
}
