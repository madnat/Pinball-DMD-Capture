#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
// V1.1 reference: three consecutive, equal-duration binary refreshes, MSB first.
struct GreyDecoder {
    static constexpr unsigned PIXELS=4096,BYTES=512;
    uint8_t accumulation[PIXELS]={},image[PIXELS]={};
    unsigned partial=0;
    uint32_t previousCandidate=0,epoch=0,firstCandidate=0,groups=0,partialDiscards=0;
    bool havePrevious=false;
    void gap() { if(partial)++partialDiscards;partial=0;memset(accumulation,0,sizeof(accumulation));havePrevious=false; }
    bool push(const uint8_t *binary,uint32_t candidate,uint32_t currentEpoch) {
        if(havePrevious && (candidate!=previousCandidate+1 || currentEpoch!=epoch))gap();
        if(!partial) {firstCandidate=candidate;memset(accumulation,0,sizeof(accumulation));}
        for(unsigned i=0;i<PIXELS;++i)accumulation[i]+=bool(binary[i/8]&(0x80u>>(i%8)));
        previousCandidate=candidate;epoch=currentEpoch;havePrevious=true;
        if(++partial!=3)return false;
        memcpy(image,accumulation,sizeof(image));partial=0;++groups;return true;
    }
};
inline uint32_t greyCRC32(const uint8_t *values,size_t size=4096) {
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<size;++i) {
        crc^=values[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return ~crc;
}
static constexpr uint16_t GREY_PALETTE[4]={0x0000,0x52AA,0xAD55,0xFFFF};
