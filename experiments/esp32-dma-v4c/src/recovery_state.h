#pragma once
#ifndef DMD_INJECT_SLOT
#define DMD_INJECT_SLOT 8
#endif
static_assert(DMD_INJECT_SLOT>0 && DMD_INJECT_SLOT<31,"Injection must be mid-refresh");
constexpr unsigned MAX_RECOVERIES=8;
struct RecoveryEvent {
    uint32_t detectedUs,lastGoodEndUs,publishedBefore,candidate,frame,slot;
    uint32_t reclaimedUs,droppedBits,newGeneration,syncUs,firstGoodSequence,firstGoodEndUs,firstGoodValidatedUs;
    Counts before,failed,anchor;
    int rowDelta,dotDelta,filteredDelta,rawDelta;
    const char *cause;
    bool injected,reclaimed,aligned;
};
static RecoveryEvent recoveryEvents[MAX_RECOVERIES];
static volatile uint32_t recoveryCount=0;
static uint32_t publicationCandidates[TARGET_FRAMES],publicationTimes[TARGET_FRAMES];
