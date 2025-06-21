#include "ui_about.h"
#include "ui.h"
#include "disp_cfg.h"
#include <FFat.h>
#include "imagedisplay.h"

extern LGFX tft;

// ---- State Variables ----
static bool aboutActive = false;
static int aboutStep = 0;
static unsigned long aboutStepTime = 0;
static bool finishedAnimation = false;

static constexpr uint16_t COLOR_GREEN  = TFT_GREEN;
static constexpr uint16_t COLOR_WHITE  = TFT_WHITE;
static constexpr uint16_t COLOR_YELLOW = 0xFFE0; // Yellow
static constexpr uint16_t COLOR_RED    = 0xF800; // Red
static constexpr uint16_t COLOR_PURPLE = 0x780F; // Purple

// ---- Minimal JPEG dimension parser ----
bool decodeJpegSize(const uint8_t* jpg, size_t len, uint16_t* w, uint16_t* h) {
    for (size_t i = 0; i + 9 < len; ++i) {
        if (jpg[i] == 0xFF && jpg[i+1] == 0xC0) {
            *h = (jpg[i+5] << 8) | jpg[i+6];
            *w = (jpg[i+7] << 8) | jpg[i+8];
            return true;
        }
    }
    return false;
}

// ---- Fade-to-black transition ----
void about_fadeToBlack(int steps = 12, int delayMs = 18) {
    for (int i = 0; i < steps; ++i) {
        uint8_t shade = 255 - (i * (255 / steps));
        tft.fillRect(0, 0, tft.width(), tft.height(), tft.color565(shade, shade, shade));
        delay(delayMs);
    }
    tft.fillScreen(TFT_BLACK);
}

// ---- Draw and center JPG from FFat ----
void about_drawImageCentered(const char* path) {
    File jpgFile = FFat.open(path, "r");
    if (jpgFile && jpgFile.size() > 0) {
        size_t jpgSize = jpgFile.size();
        uint8_t* jpgBuffer = (uint8_t*)heap_caps_malloc(jpgSize, MALLOC_CAP_SPIRAM);
        if (jpgBuffer) {
            int bytesRead = jpgFile.read(jpgBuffer, jpgSize);
            jpgFile.close();
            if ((size_t)bytesRead == jpgSize) {
                uint16_t w = 0, h = 0;
                if (decodeJpegSize(jpgBuffer, jpgSize, &w, &h)) {
                    int x = (tft.width()  - w) / 2;
                    int y = (tft.height() - h) / 2;
                    tft.drawJpg(jpgBuffer, jpgSize, x, y);
                } else {
                    tft.drawJpg(jpgBuffer, jpgSize, 0, 0);
                }
            }
            heap_caps_free(jpgBuffer);
        } else {
            jpgFile.close();
            Serial.println("[About] PSRAM alloc failed!");
        }
    } else {
        Serial.println("[About] JPG file open or size failed!");
        if (jpgFile) jpgFile.close();
    }
}

void ui_about_open() {
    aboutActive = true;
    aboutStep = 0;
    aboutStepTime = millis();
    finishedAnimation = false;
}

bool ui_about_isActive() {
    return aboutActive;
}

void ui_about_update() {
    if (!aboutActive) return;

    unsigned long now = millis();

    switch (aboutStep) {
        case 0:
            tft.setRotation(0);
            tft.setTextDatum(middle_center);
            tft.setTextFont(1);
            about_fadeToBlack();
            // Type D + Version
            tft.setTextSize(6);
            tft.setTextColor(COLOR_GREEN, TFT_BLACK);
            tft.drawString("Type D XL", tft.width() / 2, tft.height() / 2 - 68);
            tft.setTextColor(COLOR_WHITE, TFT_BLACK);
            tft.setTextSize(4);
            tft.drawString(VERSION_TEXT, tft.width() / 2, tft.height() / 2 + 16);
            aboutStepTime = now;
            aboutStep++;
            break;
        case 1: {
            if (now - aboutStepTime < 1500) return;
            tft.setTextSize(2);
            about_fadeToBlack();
            tft.setTextColor(COLOR_WHITE, TFT_BLACK);
            tft.setTextSize(4);
            tft.drawString("Concept by:", tft.width() / 2, tft.height() / 2 - 32);

            tft.setTextSize(5);
            // Use textWidth to center multicolored name
            int wAndr = tft.textWidth("Andr");
            int w0    = tft.textWidth("0");
            int totalW = wAndr + w0;
            int baseX = tft.width() / 2 - totalW / 2;
            int y = tft.height() / 2 + 46;

            tft.setTextColor(COLOR_YELLOW, TFT_BLACK);
            tft.setTextDatum(top_left);
            tft.drawString("Andr", baseX, y);
            tft.setTextColor(COLOR_RED, TFT_BLACK);
            tft.drawString("0", baseX + wAndr, y);

            tft.setTextDatum(middle_center); // restore for safety
            aboutStepTime = now;
            aboutStep++;
            break;
        }
        case 2: {
            if (now - aboutStepTime < 1500) return;
            tft.setTextSize(2);
            about_fadeToBlack();
            tft.setTextColor(COLOR_WHITE, TFT_BLACK);
            tft.setTextSize(4);
            tft.drawString("Coded by:", tft.width() / 2, tft.height() / 2 - 32);

            tft.setTextSize(5);
            int wDarkone = tft.textWidth("Darkone");
            int w83      = tft.textWidth("83");
            int totalW = wDarkone + w83;
            int baseX = tft.width() / 2 - totalW / 2;
            int y = tft.height() / 2 + 46;

            tft.setTextColor(COLOR_PURPLE, TFT_BLACK);
            tft.setTextDatum(top_left);
            tft.drawString("Darkone", baseX, y);
            tft.setTextColor(COLOR_GREEN, TFT_BLACK);
            tft.drawString("83", baseX + wDarkone, y);

            tft.setTextDatum(middle_center); // restore for safety
            aboutStepTime = now;
            aboutStep++;
            break;
        }
        case 3:
            if (now - aboutStepTime < 1500) return;
            tft.setTextSize(2);
            about_fadeToBlack();
            about_drawImageCentered("/resource/TR.jpg");
            aboutStepTime = now;
            aboutStep++;
            break;
        case 4: {
            if (now - aboutStepTime < 1500) return;
            about_fadeToBlack();
            about_drawImageCentered("/resource/XBS.jpg");
            tft.setTextColor(COLOR_WHITE, TFT_BLACK);
            tft.setTextSize(3);
            tft.setTextDatum(middle_center);
            // Draw just below the image: assume image height ≈ 240, screen center Y = 240
            int yBelow = tft.height() / 2 + 130; // Try 130px below center; adjust if needed
            if (yBelow > tft.height() - 32) yBelow = tft.height() - 32; // Clip for safety
            tft.drawString("XBOX-scene.info", tft.width() / 2, yBelow);
            aboutStepTime = now;
            aboutStep++;
            break;
        }
        case 5: {
            if (now - aboutStepTime < 1500) return;
            about_fadeToBlack();
            about_drawImageCentered("/resource/DC.jpg");
            tft.setTextColor(COLOR_WHITE, TFT_BLACK);
            tft.setTextSize(3);
            tft.setTextDatum(middle_center);
            int yBelow = tft.height() / 2 + 130;
            if (yBelow > tft.height() - 32) yBelow = tft.height() - 32;
            tft.drawString("darkonecustoms.com", tft.width() / 2, yBelow);
            aboutStepTime = now;
            aboutStep++;
            break;
        }
        case 6:
            if (now - aboutStepTime < 2000) return;
            about_fadeToBlack();
            about_drawImageCentered("/resource/TD.jpg");
            tft.setTextColor(COLOR_WHITE, TFT_BLACK);
            aboutStepTime = now;
            aboutStep++;
            break;
        case 7:
            if (now - aboutStepTime < 4000) return;
            about_fadeToBlack();
            // Handoff only ONCE
            aboutActive = false;        // Disable further updates
            aboutStep = 0;
            aboutStepTime = 0;
            if (!finishedAnimation) {
                finishedAnimation = true;   // Signal handoff (only once)
                UI::showMenu();
            }
            break;
        default:
            break;
    }
}