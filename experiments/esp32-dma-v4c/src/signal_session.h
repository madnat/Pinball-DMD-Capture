#pragma once
#include <stdint.h>
// No platform dependencies: one counted episode, no errors for startup silence.
struct SignalSession {
    bool hasImage=false,waiting=true,outage=false,lossCounted=false;
    uint32_t errors=0,losses=0,recoveries=0,episode=0,attempts=0,returnUs=0;
    bool enterWait(bool silent) {
        bool fresh=false;
        if(!waiting) {waiting=true;outage=true;lossCounted=false;++errors;++episode;fresh=true;}
        if(outage && silent && !lossCounted) {++losses;lossCounted=true;}
        return fresh;
    }
    bool markSilent() {
        if(!outage || lossCounted)return false;
        lossCounted=true;++losses;return true;
    }
    void returned(uint32_t now) {returnUs=now;++attempts;}
    bool goodGroup() {
        const bool recovered=outage;
        if(waiting && recovered)++recoveries;
        hasImage=true;waiting=false;outage=false;lossCounted=false;
        return recovered;
    }
};
