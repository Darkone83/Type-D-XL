#include "Touch_CST820.h"
#include "ui.h"
#include "disp_cfg.h"
#include "imagedisplay.h"
#include "ui_set.h"
#include "ui_about.h"
#include "Touch_CST820.h"

// --- UI/Menu Variables (for 480x480) ---
static LGFX* _tft = nullptr;
static bool menuVisible = false;

const char* menuItems[] = {"Settings", "About"};
const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);
static const int screenCenterX = 240;
static const int screenCenterY = 240;
static const int menuW = 320;
static const int menuH = 60;
static const int menuX = 80;
static const int menuY0 = 160;
static const int itemHeight = 72;

// --- UI Initialization ---
void UI::begin(LGFX* tft) {
    _tft = tft;
}

bool UI::isMenuVisible() { return menuVisible; }

void UI::showMenu() {
    menuVisible = true;
    drawMenu();
    ImageDisplay::setPaused(true);
}

void UI::drawMenu() {
    _tft->setRotation(0);
    _tft->setTextDatum(middle_center);
    _tft->setTextFont(1);
    _tft->setTextSize(2);
    _tft->fillScreen(TFT_BLACK);
    _tft->setTextColor(TFT_GREEN, TFT_BLACK);
    _tft->setTextSize(4);
    _tft->drawString("Type D XL Menu", screenCenterX, 96);

    _tft->setTextColor(TFT_WHITE, TFT_BLACK);
    for (int i = 0; i < menuCount; ++i) {
        int y = menuY0 + i * itemHeight;
        _tft->fillRoundRect(menuX, y, menuW, menuH, 20, TFT_DARKGREEN);
        _tft->drawRoundRect(menuX, y, menuW, menuH, 20, TFT_GREEN);
        _tft->setTextSize(3);
        _tft->setTextColor(TFT_GREEN, TFT_DARKGREEN);
        _tft->drawString(menuItems[i], screenCenterX, y + menuH / 2);
    }
    int exitY = menuY0 + menuCount * itemHeight;
    _tft->fillRoundRect(menuX, exitY, menuW, menuH, 20, TFT_BLACK);
    _tft->drawRoundRect(menuX, exitY, menuW, menuH, 20, TFT_GREEN);
    _tft->setTextSize(3);
    _tft->setTextColor(TFT_GREEN, TFT_BLACK);
    _tft->drawString("Exit", screenCenterX, exitY + menuH / 2);
}

void UI::update() {

    // --- Menu open: long press ---
    if (!menuVisible && touch_data.gesture == LONG_PRESS) {
        menuVisible = true;
        ImageDisplay::setPaused(true);
        drawMenu();
        Serial.println("[UI] Menu opened (long press)");
        touch_data.gesture = NONE; // <-- Clear gesture after processing
        return;
    }

    // --- Menu select: single tap ---
    if (menuVisible && touch_data.gesture == SINGLE_CLICK) {
        int tx = touch_data.x;
        int ty = touch_data.y;

        for (int i = 0; i < menuCount; ++i) {
            int y = menuY0 + i * itemHeight;
            if (tx >= menuX && tx <= menuX + menuW && ty >= y && ty <= y + menuH) {
                menuVisible = false;
                if (i == 0) UISet::begin(_tft);
                if (i == 1) ui_about_open();
                Serial.printf("[UI] Menu item %d selected\n", i);
                touch_data.gesture = NONE; // <-- Clear gesture after processing
                return;
            }
        }
        int exitY = menuY0 + menuCount * itemHeight;
        if (tx >= menuX && tx <= menuX + menuW && ty >= exitY && ty <= exitY + menuH) {
            menuVisible = false;
            ImageDisplay::setPaused(false);
            Serial.println("[UI] Menu closed");
            touch_data.gesture = NONE; // <-- Clear gesture after processing
            return;
        }
    }
}
