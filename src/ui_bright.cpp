#include "ui_bright.h"
#include "disp_cfg.h"
#include <Arduino.h>
#include <Preferences.h>
#include "ui_set.h"
#include "ui.h" // central UI/touch interface
#include "Touch_CST820.h"

extern LGFX tft;

static bool menuVisible = false;
static Preferences prefs;

#define BRIGHTNESS_PREF_KEY "brightness"
#define BRIGHTNESS_PREF_NS "type_d"

enum BrightnessLevel { BRIGHT_HIGH, BRIGHT_MED_HIGH, BRIGHT_MED, BRIGHT_MED_LOW, BRIGHT_LOW };
static BrightnessLevel currLevel = BRIGHT_HIGH;

const int brightPercents[5] = {100, 75, 50, 25, 5};
const char* brightLabels[5] = {"High", "Med-High", "Med","Med-Low", "Low"};

static int percent_to_hw(int percent) {
    if (percent < 5) percent = 5;
    if (percent > 100) percent = 100;
    return ((percent * 255) / 100);
}

static void apply_brightness(BrightnessLevel level) {
    int percent = brightPercents[level];
    int hwval = percent_to_hw(percent);
    Serial.printf("[ui_bright_update] setBrightness(%d)\n", hwval);
    tft.setBrightness(hwval);
    prefs.begin(BRIGHTNESS_PREF_NS, false); // read-write
    prefs.putUInt(BRIGHTNESS_PREF_KEY, percent);
    prefs.end();
}

static void drawBrightnessMenu() {
    tft.setRotation(0);
    tft.setTextDatum(middle_center);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);

    // Title
    tft.setTextDatum(middle_center);
    tft.setTextSize(4);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Brightness", tft.width()/2, 70);

    // Large brightness button (centered)
    int btnW = 340, btnH = 112, btnX = (tft.width() - btnW)/2, btnY = 140, radius = 36;
    tft.fillRoundRect(btnX, btnY, btnW, btnH, radius, TFT_DARKGREEN);
    tft.drawRoundRect(btnX, btnY, btnW, btnH, radius, TFT_GREEN);
    tft.setTextDatum(middle_center);
    tft.setTextSize(5);
    tft.setTextColor(TFT_GREEN, TFT_DARKGREEN);
    tft.drawString(brightLabels[currLevel], btnX + btnW / 2, btnY + btnH / 2);

    // Back button (centered below)
    int backW = 220, backH = 76, backX = (tft.width() - backW)/2, backY = btnY + btnH + 48;
    tft.setTextSize(4);
    tft.fillRoundRect(backX, backY, backW, backH, 18, TFT_DARKGREEN);
    tft.drawRoundRect(backX, backY, backW, backH, 18, TFT_GREEN);
    tft.setTextColor(TFT_GREEN, TFT_DARKGREEN);
    tft.drawString("Back", backX + backW / 2, backY + backH / 2);

    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
}

void ui_bright_open() {
    prefs.begin(BRIGHTNESS_PREF_NS, true); // read-only
    int lastPercent = prefs.getUInt(BRIGHTNESS_PREF_KEY, 100);
    prefs.end();
    if (lastPercent >= 90)      currLevel = BRIGHT_HIGH;
    else if (lastPercent >= 65) currLevel = BRIGHT_MED_HIGH;
    else if (lastPercent >= 40) currLevel = BRIGHT_MED;
    else if (lastPercent >= 15) currLevel = BRIGHT_MED_LOW;
    else                        currLevel = BRIGHT_LOW;

    menuVisible = true;
    apply_brightness(currLevel);
    drawBrightnessMenu();
}

void ui_bright_exit() {
    menuVisible = false;
    tft.fillScreen(TFT_BLACK);
}

bool ui_bright_isVisible() {
    return menuVisible;
}

void ui_bright_update() {
    if (!menuVisible) return;

    // Only act if there’s a gesture, and clear it immediately after!
    if (touch_data.gesture == SINGLE_CLICK) {
        int tx = touch_data.x;
        int ty = touch_data.y;

        int btnW = 340, btnH = 112, btnX = (tft.width() - btnW)/2, btnY = 140;
        int backW = 220, backH = 76, backX = (tft.width() - backW)/2, backY = btnY + btnH + 48;

        if (tx >= btnX && tx < btnX + btnW && ty >= btnY && ty < btnY + btnH) {
            Serial.println("[ui_bright_update] Brightness button pressed");
            currLevel = (BrightnessLevel)((currLevel + 1) % 5);
            apply_brightness(currLevel);
            drawBrightnessMenu();
            touch_data.gesture = NONE; // Clear after handling
        }
        else if (tx >= backX && tx < backX + backW && ty >= backY && ty < backY + backH) {
            Serial.println("[ui_bright_update] Back button pressed");
            menuVisible = false;
            UISet::begin(&tft);
            touch_data.gesture = NONE; // Clear after handling
        }
    }
}
