#include "ui_winfo.h"
#include "disp_cfg.h"
#include "ui_set.h"
#include <WiFi.h>
#include "ui.h"
#include "Touch_CST820.h"

extern LGFX tft;

static bool menuVisible = false;

static void drawWiFiInfoMenu() {
    tft.setRotation(0);
    tft.setTextDatum(middle_center);
    tft.setTextFont(1);
    tft.setTextSize(1);
    tft.fillScreen(TFT_BLACK);

    // Title
    tft.setTextDatum(middle_center);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(4);
    tft.drawString("WiFi Info", tft.width() / 2, 84);

    // SSID (centered, white)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(3);
    String ssid = WiFi.SSID();
    tft.drawString(ssid.length() > 0 ? ssid : "(none)", tft.width() / 2, 184);

    // IP (centered, white, just below SSID)
    IPAddress ip = WiFi.localIP();
    String ipStr = ip.toString();
    tft.drawString(ipStr, tft.width() / 2, 254);

    // Back button (large and centered below)
    int backW = 220, backH = 76, backX = (tft.width() - backW) / 2, backY = 350;
    tft.setTextSize(4);
    tft.fillRoundRect(backX, backY, backW, backH, 18, TFT_DARKGREEN);
    tft.drawRoundRect(backX, backY, backW, backH, 18, TFT_GREEN);
    tft.setTextColor(TFT_GREEN, TFT_DARKGREEN);
    tft.drawString("Back", backX + backW / 2, backY + backH / 2);

    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
}

void ui_winfo_open() {
    menuVisible = true;
    drawWiFiInfoMenu();
}

void ui_winfo_exit() {
    menuVisible = false;
    tft.fillScreen(TFT_BLACK);
    UISet::begin(&tft);
}

bool ui_winfo_isVisible() {
    return menuVisible;
}

void ui_winfo_update() {
    if (!menuVisible) return;

    uint8_t gesture = touch_data.gesture;
    int tx = touch_data.x;
    int ty = touch_data.y;

    int backW = 220, backH = 76, backX = (tft.width() - backW) / 2, backY = 350;

    if (gesture == SINGLE_CLICK) {
        if (tx >= backX && tx < backX + backW && ty >= backY && ty < backY + backH) {
            Serial.println("[ui_winfo_update] Back button pressed");
            menuVisible = false;
            UISet::begin(&tft);
            touch_data.gesture = NONE;
            return;
        }
    }
}
