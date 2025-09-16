#include "xbox_status.h"
#include <Arduino.h>
#include <FFat.h>
#include "disp_cfg.h"
#include <esp_heap_caps.h>   // PSRAM for JPG buffers

// ----------------- small helpers -----------------
static inline int measureTextWidth(LGFX* tft, const String& s, int font) {
  tft->setTextFont(font);
  return tft->textWidth(s);
}

static void drawShadowedText(LGFX* tft, const String& text, int x, int y, uint16_t color, uint16_t shadow, int font) {
  tft->setTextFont(font);
  tft->setTextColor(shadow, TFT_BLACK);
  tft->drawString(text, x+2, y+2);
  tft->setTextColor(color, TFT_BLACK);
  tft->drawString(text, x, y);
}

static void drawJpgFromFFat(LGFX* tft, const char* path, int x, int y, int w, int h) {
  File f = FFat.open(path, "r");
  if (!f || f.size() <= 0) { if (f) f.close(); return; }
  size_t sz = f.size();
  uint8_t* buf = (uint8_t*)heap_caps_malloc(sz, MALLOC_CAP_SPIRAM);
  if (!buf) { f.close(); return; }
  int br = f.read(buf, sz);
  f.close();
  if (br == (int)sz) tft->drawJpg(buf, sz, x, y, w, h);
  heap_caps_free(buf);
}

static void drawIconOrPlaceholder(LGFX* tft, const char* path, int x, int y, int w, int h) {
  File probe = FFat.open(path, "r");
  if (probe && probe.size() > 0) {
    probe.close();
    drawJpgFromFFat(tft, path, x, y, w, h);
  } else {
    if (probe) probe.close();
    tft->fillRoundRect(x, y, w, h, 10, TFT_DARKGREY);
    tft->drawRoundRect(x, y, w, h, 10, TFT_BLACK);
  }
}

// ----- A/V pack label (primary + fallback) -----
static String avPackString(int avVal) {
  uint8_t v = (uint8_t)avVal;
  switch (v) {
    case 0x00: return "SCART";
    case 0x01: return "HDTV (Component)";
    case 0x02: return "VGA";
    case 0x03: return "RFU";
    case 0x04: return "Advanced (S-Video)";
    case 0x05: return "Undefined";
    case 0x06: return "Standard (Composite)";
    case 0x07: return "Disconnected";
    default: break;
  }
  uint8_t k = (v & 0x0E);
  switch (k) {
    case 0x00: return "Disconnected";
    case 0x02: return "Standard (Composite)";
    case 0x06: return "Advanced (S-Video)";
    case 0x0A: return "HDTV (Component)";
    case 0x0E: return "SCART";
    default: break;
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "0x%02X", v);
  return String(buf);
}

// ----- encoder name from I2C address (or legacy 0/1/2) -----
static String encoderNameFromType(int enc) {
  switch (enc) {
    case 0x45: case 0: return "Conexant";
    case 0x6A: case 1: return "Focus";
    case 0x70: case 2: return "Xcalibur";
    default: {
      char buf[16]; snprintf(buf, sizeof(buf), "0x%02X", (enc & 0xFF));
      return String(buf);
    }
  }
}

// ----- version inference: serial first, encoder fallback -----
static String versionFromSerialOrEncoder(const XboxStatus& pkt) {
  String s = String(pkt.eeSerial); s.trim();
  if (s.length() >= 5) {
    int ywwffIdx = -1;
    for (int i = (int)s.length() - 5; i >= 0; --i) {
      bool ok = true;
      for (int k = 0; k < 5; ++k) if (!isDigit((unsigned char)s[i+k])) { ok = false; break; }
      if (ok) { ywwffIdx = i; break; }
    }
    if (ywwffIdx >= 0) {
      int Y  = s[ywwffIdx] - '0';
      int WW = (s[ywwffIdx+1]-'0')*10 + (s[ywwffIdx+2]-'0');
      int FF = (s[ywwffIdx+3]-'0')*10 + (s[ywwffIdx+4]-'0');

      if (FF == 3) return "1.0 (03)";
      if (FF == 2) { if (Y == 2 && WW < 45) return "1.0 (02)"; return "1.1 (02)"; }
      if (Y == 2) { if (WW >= 50) return "1.2"; return "1.1"; }
      if (Y == 3) { if (WW <= 10) return "1.2"; if (WW <= 20) return "1.3"; if (WW >= 31) return "1.4"; return "1.3"; }
      if (Y == 4) { if (WW <= 12) return "1.4"; if (WW >= 38) return "1.6b"; return "1.6"; }
      if (Y >= 5) return "1.6b";
    }
  }
  String enc = encoderNameFromType(pkt.encoderType);
  if      (enc.startsWith("Conexant")) return "1.0–1.3";
  else if (enc.startsWith("Focus"))    return "1.4";
  else if (enc.startsWith("Xcalibur")) return "1.6/1.6b";
  return "Unknown";
}

// ===================== UI Pager =====================
namespace xbox_status {

static const uint32_t PAGE_MS = 4000;     // rotate every 4s
static uint32_t s_lastFlip = 0;
static int      s_page = 0;               // 0..2

void show(LGFX* tft, const XboxStatus& packet) {
  // Pager
  uint32_t now = millis();
  if (now - s_lastFlip >= PAGE_MS) {
    s_lastFlip = now;
    s_page = (s_page + 1) % 3;
  }

  // Display baseline
  tft->setRotation(0);         // round 2.1" GC9A01 portrait
  tft->setTextDatum(TL_DATUM);
  tft->setTextFont(1);
  tft->setTextSize(1);
  tft->fillScreen(TFT_BLACK);

  const int W = tft->width();      // ~480
  const int H = tft->height();     // ~480
  const int CX = W / 2;
  const int CY = H / 2;

  // Inner safe area to avoid the circular edge
  const int MARGIN = 36;                 // conservative inner margin
  const int SAFE_L = MARGIN;
  const int SAFE_T = MARGIN;
  const int SAFE_R = W - MARGIN;
  const int SAFE_B = H - MARGIN;

  const uint16_t labelCol = TFT_LIGHTGREY;
  const uint16_t valueCol = 0x07E0;      // green
  const int labelFont = 2;
  const int valueFont = 2;
  const int iconSize = 64;               // bigger icons for 2.1"

  if (s_page == 0) {
    // ---------- Page 1: Fan / CPU / Ambient (original pyramid) ----------
    const int topY = max(SAFE_T + iconSize/2, CY - 120);
    const int botY = min(SAFE_B - iconSize/2, CY + 60);
    const int offX = 150;

    struct Item { const char* icon; String label; String value; int x,y; } items[] = {
      { "/resource/fan.jpg",  "Fan",     String(packet.fanSpeed) + "%",    CX,           topY },
      { "/resource/cpu.jpg",  "CPU",     String(packet.cpuTemp) + "C",     CX - offX,    botY },
      { "/resource/amb.jpg",  "Ambient", String(packet.ambientTemp) + "C", CX + offX,    botY },
    };

    for (auto& it : items) {
      int iconX = it.x - iconSize / 2;
      int iconY = it.y - iconSize / 2;
      drawIconOrPlaceholder(tft, it.icon, iconX, iconY, iconSize, iconSize);

      int labelY = iconY + iconSize + 6;
      int lw = measureTextWidth(tft, it.label, labelFont);
      drawShadowedText(tft, it.label, it.x - lw/2, labelY, labelCol, TFT_DARKGREY, labelFont);

      int valueY = labelY + 22;
      int vw = measureTextWidth(tft, it.value, valueFont);
      drawShadowedText(tft, it.value, it.x - vw/2, valueY, valueCol, TFT_DARKGREY, valueFont);
    }
    return;
  }

  if (s_page == 1) {
    // ---------- Page 2: Pyramid — App (top), Resolution (BL), A/V Pack (BR) ----------
    const int topY = max(SAFE_T + iconSize/2, CY - 120);
    const int botY = min(SAFE_B - iconSize/2, CY + 60);

    // Pull the bottom items inward and clamp to safe area:
    int spread = 120; // was 150; reduced to bring items in
    int leftX  = CX - spread;
    int rightX = CX + spread;

    // Clamp to keep icon centers away from the circular edge
    const int minCenter = SAFE_L + iconSize/2 + 8;
    const int maxCenter = SAFE_R - iconSize/2 - 8;
    if (leftX  < minCenter) leftX  = minCenter;
    if (rightX > maxCenter) rightX = maxCenter;

    struct Item { const char* icon; String label; String value; int x,y; } items[] = {
      { "/resource/app.jpg",  "App",        String(packet.currentApp),        CX,     topY },
      { "/resource/res.jpg",  "Resolution", String(packet.resolution),        leftX,  botY },
      { "/resource/av.jpg",   "A/V Pack",   avPackString(packet.avPackState), rightX, botY },
    };

    for (auto& it : items) {
      int iconX = it.x - iconSize / 2;
      int iconY = it.y - iconSize / 2;
      drawIconOrPlaceholder(tft, it.icon, iconX, iconY, iconSize, iconSize);

      int labelY = iconY + iconSize + 6;
      int lw = measureTextWidth(tft, it.label, labelFont);
      drawShadowedText(tft, it.label, it.x - lw/2, labelY, labelCol, TFT_DARKGREY, labelFont);

      int valueY = labelY + 22;
      String val = it.value.length() ? it.value : String("—");
      int vw = measureTextWidth(tft, val, valueFont);
      drawShadowedText(tft, val, it.x - vw/2, valueY, valueCol, TFT_DARKGREY, valueFont);
    }
    return;
  }

  // ---------- Page 3: 2×2 Grid — Version, Encoder, Region, MAC ----------
  const String ver = versionFromSerialOrEncoder(packet);
  const String enc = encoderNameFromType(packet.encoderType);
  const String reg = String(packet.eeRegion).length() ? String(packet.eeRegion) : String("—");
  const String mac = String(packet.eeMac).length()    ? String(packet.eeMac)    : String("—");

  struct Cell { const char* icon; String label; String value; };
  Cell cells[4] = {
    { "/resource/ver.jpg", "Version", ver },
    { "/resource/enc.jpg", "Encoder", enc },
    { "/resource/reg.jpg", "Region",  reg },
    { "/resource/mac.jpg", "MAC",     mac },
  };

  // Safe grid area (avoid edges)
  const int GRID_L = SAFE_L + 8;
  const int GRID_T = SAFE_T + 8;
  const int GRID_R = SAFE_R - 8;
  const int GRID_B = SAFE_B - 8;

  const int COL_W = (GRID_R - GRID_L) / 2;
  const int ROW_H = (GRID_B - GRID_T) / 2;

  const int cellIcon = 56;
  const int labelGap = 6;
  const int valueGap = 22;

  for (int r = 0; r < 2; ++r) {
    for (int c = 0; c < 2; ++c) {
      int idx = r * 2 + c;
      int cx = GRID_L + c * COL_W + COL_W / 2;
      int cy = GRID_T + r * ROW_H + ROW_H / 2;

      int iconX = cx - cellIcon/2;
      int iconY = cy - (cellIcon/2 + 18 + 22)/2; // bias upward to leave room for text

      drawIconOrPlaceholder(tft, cells[idx].icon, iconX, iconY, cellIcon, cellIcon);

      int labelY = iconY + cellIcon + labelGap;
      int lw = measureTextWidth(tft, cells[idx].label, labelFont);
      drawShadowedText(tft, cells[idx].label, cx - lw/2, labelY, labelCol, TFT_DARKGREY, labelFont);

      int valueY = labelY + valueGap;
      String val = cells[idx].value.length() ? cells[idx].value : String("—");
      int vw = measureTextWidth(tft, val, valueFont);
      drawShadowedText(tft, val, cx - vw/2, valueY, valueCol, TFT_DARKGREY, valueFont);
    }
  }
}

} // namespace xbox_status
