
#include <Arduino.h>
#include "driver/pcnt.h"

constexpr int PIN_RCLK = 25;

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("V1.2a - RCLK HARDWARE COUNTER");

    pinMode(PIN_RCLK, INPUT);

    pcnt_config_t config = {};
    config.pulse_gpio_num = PIN_RCLK;
    config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    config.unit = PCNT_UNIT_0;
    config.channel = PCNT_CHANNEL_0;

    config.pos_mode = PCNT_COUNT_INC;
    config.neg_mode = PCNT_COUNT_DIS;
    config.lctrl_mode = PCNT_MODE_KEEP;
    config.hctrl_mode = PCNT_MODE_KEEP;

    config.counter_h_lim = 10000;
    config.counter_l_lim = 0;

    esp_err_t result = pcnt_unit_config(&config);

    if (result != ESP_OK) {
        Serial.printf(
            "PCNT configuration failed: %d\n",
            result
        );
        return;
    }

    // Disable the glitch filter initially.
    pcnt_filter_disable(PCNT_UNIT_0);

    pcnt_counter_pause(PCNT_UNIT_0);
    pcnt_counter_clear(PCNT_UNIT_0);
    pcnt_counter_resume(PCNT_UNIT_0);
}

void loop() {
    pcnt_counter_clear(PCNT_UNIT_0);

    delay(100);

    int16_t count = 0;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);

    Serial.printf(
        "RCLK GPIO25: %d rising edges / 100 ms\n",
        count
    );

    delay(900);
}
