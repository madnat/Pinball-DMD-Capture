
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

constexpr int DMD_WIDTH = 128;
constexpr int DMD_HEIGHT = 32;

enum TestPattern {
    LEFT_COLUMN,
    TOP_ROW,
    BOTTOM_ROW
};

void drawDMD(TestPattern pattern, const char* label) {
    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(8, 8);
    tft.print(label);

    // Render each DMD pixel as a 1x2 LCD rectangle.
    // 128x32 DMD becomes 128x64 on screen.
    const int x0 = 56;
    const int y0 = 38;

    tft.drawRect(x0 - 2, y0 - 2, 132, 68, TFT_DARKGREY);

    for (int y = 0; y < DMD_HEIGHT; y++) {
        for (int x = 0; x < DMD_WIDTH; x++) {
            bool lit = false;

            switch (pattern) {
                case LEFT_COLUMN:
                    lit = (x == 0);
                    break;
                case TOP_ROW:
                    lit = (y == 0);
                    break;
                case BOTTOM_ROW:
                    lit = (y == 31);
                    break;
            }

            if (lit) {
                tft.fillRect(
                    x0 + x,
                    y0 + y * 2,
                    1,
                    2,
                    TFT_ORANGE
                );
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("LCD Diagnostic V0.2");

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);

    tft.init();
    tft.setRotation(1);

    Serial.println("TFT initialised");
}

void loop() {
    tft.fillScreen(TFT_RED);
    Serial.println("RED");
    delay(1000);

    tft.fillScreen(TFT_GREEN);
    Serial.println("GREEN");
    delay(1000);

    tft.fillScreen(TFT_BLUE);
    Serial.println("BLUE");
    delay(1000);

    drawDMD(LEFT_COLUMN, "LEFT COLUMN");
    Serial.println("LEFT COLUMN");
    delay(2000);

    drawDMD(TOP_ROW, "TOP ROW");
    Serial.println("TOP ROW");
    delay(2000);

    drawDMD(BOTTOM_ROW, "BOTTOM ROW");
    Serial.println("BOTTOM ROW");
    delay(2000);
}
