#pragma once
#include "driver/uart.h"

struct FreezyPacket {
    GreyRecord frame;
    uint32_t acquiredUs;
    uint8_t pixels[1024];
};

static QueueHandle_t freezyQueue;
static volatile uint32_t freezySent=0,freezyDropped=0,freezyTxErrors=0;

static void freezyOffer(const GreyImage &image,uint32_t acquiredUs) {
    FreezyPacket packet;
    packet.frame=image.record;
    packet.acquiredUs=acquiredUs;
    for(unsigned i=0;i<sizeof(packet.pixels);++i) {
        const uint8_t *p=image.values+4*i;
        packet.pixels[i]=(p[0]<<6)|(p[1]<<4)|(p[2]<<2)|p[3];
    }
    if(xQueueSend(freezyQueue,&packet,0)!=pdTRUE)++freezyDropped;
}

static void freezySerialTask(void *) {
    static FreezyPacket packet;
    static uint8_t line[1152];
    while(true) {
        if(xQueueReceive(freezyQueue,&packet,portMAX_DELAY)!=pdTRUE)continue;
        const auto &f=packet.frame;
        const int prefix=snprintf(reinterpret_cast<char *>(line),sizeof(line),
            "FREEZY2 %lu %lu %lu %lu %lu %08lX ",
            (unsigned long)f.n,(unsigned long)packet.acquiredUs,(unsigned long)f.epoch,
            (unsigned long)f.first,(unsigned long)f.last,(unsigned long)f.crc);
        memcpy(line+prefix,packet.pixels,sizeof(packet.pixels));
        const int length=prefix+sizeof(packet.pixels)+1;
        line[length-1]='\n';
        // Use IDF directly to observe send errors. UART0 is the only serial
        // writer during capture in this variant, apart from initial setup.
        if(uart_write_bytes(UART_NUM_0,line,length)==length)++freezySent;
        else ++freezyTxErrors;
    }
}

static void freezyInit() {
    freezyQueue=xQueueCreate(8,sizeof(FreezyPacket));
    if(!freezyQueue ||
       xTaskCreatePinnedToCore(freezySerialTask,"freezy-serial",4096,nullptr,1,nullptr,0)!=pdPASS)
        fatal("Freezy export queue/task allocation");
    Serial.println("FREEZY2 START");
}

static void freezyDrain(uint32_t built) {
    const uint32_t start=millis();
    while(freezySent+freezyDropped+freezyTxErrors<built &&
          uint32_t(millis()-start)<1000)delay(1);
}

static void freezyReport(uint32_t built,uint32_t binaryDrops) {
    const uint32_t sent=freezySent,dropped=freezyDropped,errors=freezyTxErrors;
    Serial.printf("FREEZY2 END built=%lu sent=%lu dropped=%lu txErrors=%lu inputDrops=%lu\n",
        (unsigned long)built,(unsigned long)sent,(unsigned long)dropped,
        (unsigned long)errors,(unsigned long)binaryDrops);
}
