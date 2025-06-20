#include <Arduino.h>
#include "wifimgr.h"
#include <FFat.h>
#include "disp_cfg.h"
#include "detect.h"
#include "fileman.h"
#include "imagedisplay.h"
#include "boot.h"
#include <ESPAsyncWebServer.h>
#include "xbox_status.h"
#include "ui.h"
#include "ui_set.h"
#include "ui_bright.h"
#include "ui_about.h"
#include "ui_winfo.h"
#include <Preferences.h>
#include "cmd.h"
#include "diag.h"
#include "udp_detect.h"
#include "Touch_CST820.h"
#include "TCA9554PWR.h"
#include "I2C_Driver.h"

// ==========================
// CST820 PIN DEFINITIONS
// ==========================
//#define TP_SDA 15
//#define TP_SCL 7
//#define TP_INT 16
//#define TP_RST EXIO_PIN2


#define WIFI_TIMEOUT 120
#define BRIGHTNESS_PREF_KEY "brightness"
#define BRIGHTNESS_PREF_NS "type_d"

LGFX tft;

AsyncWebServer server80(80);
AsyncWebServer server8080(8080);

bool nowConnected = WiFiMgr::isConnected();
bool portalInfoShown = false;

static bool overlayPending = false;
static bool showingXboxStatus = false;
static unsigned long lastStatusDisplay = 0;

XboxStatus lastXboxStatus;

static int percent_to_hw(int percent) {
    if (percent < 5) percent = 5;
    if (percent > 100) percent = 100;
    return ((percent * 255) / 100);
}

void apply_saved_brightness() {
    Preferences prefs;
    prefs.begin(BRIGHTNESS_PREF_NS, true);
    int percent = prefs.getUInt(BRIGHTNESS_PREF_KEY, 100); // default to 100%
    prefs.end();

    if (percent < 5) percent = 5;
    if (percent > 100) percent = 100;
    int hwval = (percent * 255) / 100;
    tft.setBrightness(hwval);
}

void displayPortalInfo() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextDatum(MC_DATUM); // center text

    tft.setTextSize(4);
    tft.drawString("WiFi Portal Active", tft.width()/2, tft.height()/2 - 70);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(4);
    tft.drawString("Type D XL Setup", tft.width()/2, tft.height()/2 - 18);
    tft.drawString("IP: 192.168.4.1", tft.width()/2, tft.height()/2 + 34);
    tft.setTextSize(2);
    tft.drawString("Connect below to setup.", tft.width()/2, tft.height()/2 + 80);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("[Type D XL] Booting...");

   if (!FFat.begin()) {
        Serial.println("[Type D XL] FFat Mount Failed! Attempting to format...");
        if (FFat.format()) {
            Serial.println("[Type D XL] FFat format succeeded! Rebooting...");
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("[Type D XL] FFat format FAILED! Halting.");
            while (1) delay(100);
        }
    } else {
        Serial.println("[Type D XL] FFat Mounted OK.");
    }
    server80.serveStatic("/resource/", FFat, "/resource/");
    server8080.serveStatic("/resource/", FFat, "/resource/");

  // I2C expander & LCD              
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Set_EXIO(EXIO_PIN2, High);         
  Set_EXIO(EXIO_PIN8, Low);          
  pinMode(6, OUTPUT); digitalWrite(6, HIGH); // Backlight ON early
  pinMode(LCD_SDA_PIN, OUTPUT);
  pinMode(LCD_SCL_PIN, OUTPUT);

  LCD_RST_L(); delay(20);
  LCD_RST_H(); delay(100);
  delay(20);

  vendor_ST7701_init();
  delay(50);

  tft.begin();
  apply_saved_brightness();
    
  bootShowScreen();
  ImageDisplay::begin(&tft);
  
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(middle_center);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(6);
  tft.drawString("Type D XL", tft.width() / 2, tft.height() / 2- 48);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(4);
  tft.drawString(VERSION_TEXT, tft.width() / 2, tft.height() / 2 + 40);
  delay(1500);

    Touch_Init();            // Initialize CST820 driver, pins set in driver
    CST820_AutoSleep(false); // Keep touch responsive
    Serial.printf("[UI] CST820 Touch (C driver) initialized");

  // WiFiMgr replaces WiFiManager
  WiFiMgr::begin();
  Serial.println("[Type D XL] WiFiMgr initialized.");

  if (!WiFiMgr::isConnected()) {
    displayPortalInfo();
  }

  UDPDetect::begin();
  server8080.begin();
  FileMan::begin(server8080);
  Diag::begin(server8080);
  cmd_init(&server8080, &tft);
  UI::begin(&tft);

  Serial.printf("[Type D XL] Device ID: %d\n", Detect::getId());

  ImageDisplay::displayRandomImage();
}

void loop() {
        if (Touch_interrupts) {
        Touch_interrupts = false;
        Touch_Read_Data();
    }

    WiFiMgr::loop();

    // UI/Menu updates etc.
if      (ui_about_isActive())    { ui_about_update(); return; }
else if (ui_bright_isVisible())  { ui_bright_update(); return; }
else if (UISet::isMenuVisible()) { UISet::update(); return; }
else if (ui_winfo_isVisible())   { ui_winfo_update(); return; }
else if (UI::isMenuVisible())    { UI::update(); return; }
    UI::update();

    // 2. Run detection and UDP polling
    Detect::loop();
    UDPDetect::loop();

    // 3. Status overlay logic -- only show between images and if no UI/menu overlay is active
    bool anyUiActive = ui_about_isActive() || ui_bright_isVisible() || UISet::isMenuVisible() || UI::isMenuVisible();

    if (ImageDisplay::isDone() && UDPDetect::hasPacket() && !overlayPending && !showingXboxStatus && !anyUiActive) {
        lastXboxStatus = UDPDetect::getLatest(); // latch latest
        overlayPending = true;
        UDPDetect::acknowledge();
    }

    if (overlayPending && !anyUiActive) {
        xbox_status::show(&tft, lastXboxStatus);
        lastStatusDisplay = millis();
        showingXboxStatus = true;
        overlayPending = false;
        return; // Show overlay, block image update
    }

    // If overlay is showing, time it out after 2s, then resume slideshow
    if (showingXboxStatus && !anyUiActive) {
        if (millis() - lastStatusDisplay > 2000) {
            showingXboxStatus = false;
            ImageDisplay::displayRandomImage();
        }
        return; // Block image update while overlay active
    }

    // 4. Only update image if overlay is not showing and not in a menu
    if (!UI::isMenuVisible()) {
        ImageDisplay::update();
    }

    cmd_serial_poll();
}


