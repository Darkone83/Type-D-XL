#include "ui_set.h"
#include "ui.h"
#include "ui_bright.h"
#include "imagedisplay.h"
#include "ui_winfo.h"
#include "wifimgr.h"
#include "Touch_CST820.h" // <-- DO NOT FORGET THIS!

static LGFX* _tft = nullptr;
static bool menuVisible = false;

static const char* menuItems[] = {
    "Brightness",
    "WiFi Info",
    "Forget WiFi",
    "Back"
};
static const int menuCount = sizeof(menuItems) / sizeof(menuItems[0]);

void drawSettingsMenu() {
    _tft->setRotation(0);
    _tft->setTextDatum(middle_center);
    _tft->setTextFont(1);
    _tft->setTextSize(1);
    _tft->fillScreen(TFT_BLACK);

    // Title
    _tft->setTextDatum(middle_center);
    _tft->setTextSize(4);
    _tft->setTextColor(TFT_GREEN, TFT_BLACK);
    _tft->drawString("Type D XL Menu", _tft->width()/2, 72);

    // Draw settings items
    int btnW = 320, btnH = 64;
    int btnX = (_tft->width() - btnW) / 2;
    int btnYBase = 128; // shift up from 160 -> 128
    int itemGap = 16;   // tighten up gap

    for (int i = 0; i < menuCount; ++i) {
        int itemY = btnYBase + i * (btnH + itemGap);

        if (i == 2) {
            _tft->fillRoundRect(btnX, itemY, btnW, btnH, 18, TFT_RED);
            _tft->drawRoundRect(btnX, itemY, btnW, btnH, 18, TFT_WHITE);
            _tft->setTextColor(TFT_WHITE, TFT_RED);
        } else if (i == menuCount - 1) {
            _tft->fillRoundRect(btnX, itemY, btnW, btnH, 18, TFT_DARKGREEN);
            _tft->drawRoundRect(btnX, itemY, btnW, btnH, 18, TFT_GREEN);
            _tft->setTextColor(TFT_GREEN, TFT_DARKGREEN);
        } else {
            _tft->fillRoundRect(btnX, itemY, btnW, btnH, 18, TFT_DARKGREEN);
            _tft->drawRoundRect(btnX, itemY, btnW, btnH, 18, TFT_GREEN);
            _tft->setTextColor(TFT_GREEN, TFT_DARKGREEN);
        }
        _tft->setTextSize(3);
        _tft->drawString(menuItems[i], btnX + btnW/2, itemY + btnH/2);
    }
}

void UISet::begin(LGFX* tft) {
    _tft = tft;
    menuVisible = true;
    _tft->fillScreen(TFT_BLACK);
    drawSettingsMenu();
}

bool UISet::isMenuVisible() {
    return menuVisible;
}

void UISet::update() {
    if (!menuVisible) return;

    uint8_t gesture = touch_data.gesture;
    int tx = touch_data.x;
    int ty = touch_data.y;

    if (gesture == SINGLE_CLICK || gesture == LONG_PRESS) {
        int btnW = 320, btnH = 64, btnX = (_tft->width() - btnW)/2, btnYBase = 160, itemGap = 24;

        for (int i = 0; i < menuCount; ++i) {
            int itemY = btnYBase + i * (btnH + itemGap);
            if (tx >= btnX && tx <= btnX + btnW && ty >= itemY && ty <= itemY + btnH) {
                if (i == 0 && gesture == SINGLE_CLICK) {
                    Serial.println("[UISet] Triggering ui_bright_open()");
                    menuVisible = false;
                    ui_bright_open();
                    touch_data.gesture = NONE;
                    return;
                } else if (i == 1 && gesture == SINGLE_CLICK) {
                    Serial.println("[UISet] Triggered ui_winfo_open()");
                    menuVisible = false;
                    ui_winfo_open();
                    touch_data.gesture = NONE;
                    return;
                } else if (i == 2 && gesture == LONG_PRESS) {
                    Serial.println("[UISet] Forget WiFi pressed");
                    WiFiMgr::forgetWiFi();
                    menuVisible = false;
                    touch_data.gesture = NONE;
                    return;
                } else if (i == 2 && gesture == SINGLE_CLICK) {
                    Serial.println("[UISet] Forget WiFi: long press required");
                    touch_data.gesture = NONE;
                    return;
                } else if (i == menuCount - 1 && gesture == SINGLE_CLICK) {
                    menuVisible = false;
                    _tft->fillScreen(TFT_BLACK);
                    ImageDisplay::setPaused(false);
                    UI::showMenu();
                    Serial.println("[UISet] Settings menu closed (Back)");
                    touch_data.gesture = NONE;
                    return;
                }
            }
        }
    }
}
