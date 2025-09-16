#pragma once
#include "disp_cfg.h"

// --- Status structure for UDP/packet sending/receiving ---
struct XboxStatus {
    // ---- Core (UDP 50504) ----
    int  fanSpeed      = -1;        // 0–100%
    int  cpuTemp       = -1000;     // Celsius
    int  ambientTemp   = -1000;     // Celsius
    char currentApp[32] = {0};      // App name

    // ---- Expansion / Video (UDP 50505) ----
    int trayState      = -1;        // Tray state
    int avPackState    = -1;        // AV pack ID / state
    int picVersion     = -1;        // PIC firmware version
    int xboxVersion    = -1;        // Xbox hardware version
    int encoderType    = -1;        // Video encoder (e.g., 0x45=Conexant, 0x6A=Focus, 0x70=Xcalibur)
    int videoWidth     = -1;        // Active width
    int videoHeight    = -1;        // Active height
    char resolution[32] = {0};      // Pretty string, e.g. "1280x720 (720p)"

    // ---- EEPROM frames (UDP 50506) ----
    // RAW (base64-decoded) contents, typically 256 bytes
    uint8_t eeRaw[256] = {0};
    int     eeRawLen    = 0;

    // Parsed / labeled fields
    char eeHddHex[33]  = {0};       // 32-hex HDD key (+NUL)
    char eeSerial[13]  = {0};       // Console serial (12 chars + NUL)
    char eeMac[18]     = {0};       // MAC address "XX:XX:XX:XX:XX:XX"
    char eeRegion[12]  = {0};       // Region code/string
};

namespace xbox_status {
    void show(LGFX* tft, const XboxStatus& packet);
}
