#include "udp_detect.h"
#include <WiFiUdp.h>
#include "xbox_status.h"
#include <string.h>
#include <ctype.h>

// ---- UDP ports ----
#define UDP_PORT_CORE   50504  // core (fan/cpu/ambient/app)
#define UDP_PORT_EXP    50505  // expansion status (7 x int32_t)
#define UDP_PORT_EE     50506  // EEPROM ASCII frames

static WiFiUDP udpCore;
static WiFiUDP udpExp;
static WiFiUDP udpEE;

static XboxStatus lastStatus;
static bool gotPacket = false;

// -------------------- Core wire format (50504) --------------------
struct CorePacket {
  int32_t fanSpeed;
  int32_t cpuTemp;
  int32_t ambientTemp;
  char    currentApp[32];
};

// -------------------- helpers --------------------
static inline void safe_copy(char* dst, size_t dstsz, const char* src) {
  if (!dst || !dstsz) return;
  if (!src) { dst[0] = '\0'; return; }
  strncpy(dst, src, dstsz - 1);
  dst[dstsz - 1] = '\0';
}

static const char* encoderNameFromType(int enc) {
  // Expansion sends I2C addresses: 0x45 (Conexant), 0x6A (Focus), 0x70 (Xcalibur).
  // Be tolerant if some builds used 0/1/2.
  switch (enc) {
    case 0x45: case 0: return "Conexant";
    case 0x6A: case 1: return "Focus";
    case 0x70: case 2: return "Xcalibur";
    default: break;
  }
  static char buf[16];
  snprintf(buf, sizeof(buf), "0x%02X", (enc & 0xFF));
  return buf;
}

// AV pack label (primary + fallback, mirrors PC viewer table)
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

static bool avIsPAL(int avVal) {
  uint8_t v = (uint8_t)avVal;
  if (v == 0x00) return true;          // SCART primary
  if ((v & 0x0E) == 0x0E) return true; // SCART fallback nibble
  return false;
}

static bool avIsHDTV(int avVal) {
  uint8_t v = (uint8_t)avVal;
  if (v == 0x01) return true;          // explicit HDTV pack
  if ((v & 0x0E) == 0x0A) return true; // fallback nibble bucket
  return false;
}

static inline bool approx(int v, int target, int tol) {
  return (v >= target - tol) && (v <= target + tol);
}

// Pretty resolution string with mode, tolerant to SD active-area variants.
static void formatResolution(int w, int h, int avVal, char *out, size_t outSize) {
  if (!out || outSize == 0) return;

  const bool pal     = avIsPAL(avVal);
  const bool isHDTV  = avIsHDTV(avVal);

  // 720p / 1080i with height tolerance
  if (approx(h, 720, 8) && approx(w, 1280, 32)) {
    snprintf(out, outSize, "%dx%d (720p)", w, h);
    return;
  }
  if (approx(h, 1080, 16) && approx(w, 1920, 64)) {
    snprintf(out, outSize, "%dx%d (1080i)", w, h);
    return;
  }

  // SD: treat widths ~640/704/720 as the same class, decide 480/576 by height
  if (approx(h, 480, 16)) {
    const char* mode = isHDTV ? "480p" : "480i";
    snprintf(out, outSize, "%dx%d (%s)", w, h, mode);
    return;
  }
  if (approx(h, 576, 16)) {
    const char* mode = isHDTV ? "576p" : "576i";
    snprintf(out, outSize, "%dx%d (%s)", w, h, mode);
    return;
  }

  // Fallback: just show WxH
  if (w > 0 && h > 0) {
    snprintf(out, outSize, "%dx%d", w, h);
  } else {
    snprintf(out, outSize, "—");
  }
}

// ---- tiny Base64 decoder (for EE:RAW) ----
static int b64val(int c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  if (c == '=') return -2;  // padding
  return -1;                // skip
}
static int base64_decode(const char* in, uint8_t* out, int outCap) {
  int o = 0, q[4], qi = 0;
  for (const char* p = in; *p; ++p) {
    int v = b64val((unsigned char)*p);
    if (v < 0 && v != -2) continue; // ignore non-b64
    q[qi++] = v;
    if (qi == 4) {
      int pad = (q[2] == -2) + (q[3] == -2);
      uint32_t b = ((uint32_t)(q[0] & 63) << 18)
                 | ((uint32_t)(q[1] & 63) << 12)
                 | ((uint32_t)((q[2] < 0 ? 0 : q[2]) & 63) << 6)
                 | ((uint32_t)((q[3] < 0 ? 0 : q[3]) & 63));
      if (pad < 3 && o < outCap) out[o++] = (b >> 16) & 0xFF;
      if (pad < 2 && o < outCap) out[o++] = (b >> 8)  & 0xFF;
      if (pad < 1 && o < outCap) out[o++] = (b >> 0)  & 0xFF;
      qi = 0;
      if (pad) break;
    }
  }
  return o;
}

static char* trim_inplace(char* s) {
  while (*s == ' ' || *s == '\t') ++s;
  char* e = s + strlen(s);
  while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) --e;
  *e = 0;
  return s;
}

// ==================== EXP (50505) ====================
// Binary Status packet (actual order from expansion):
// trayState, avPackState, picVersion, xboxVersion, videoWidth, videoHeight, encoderType
static void parseExpansionBinary(const uint8_t* buf, int n) {
  if (n != 28) return;
  const int32_t* f = (const int32_t*)buf;

  lastStatus.trayState   = f[0];
  lastStatus.avPackState = f[1];
  lastStatus.picVersion  = f[2];
  lastStatus.xboxVersion = f[3];
  lastStatus.videoWidth  = f[4];
  lastStatus.videoHeight = f[5];
  lastStatus.encoderType = f[6];  // I2C addr: 0x45/0x6A/0x70

  formatResolution(lastStatus.videoWidth, lastStatus.videoHeight,
                   lastStatus.avPackState, lastStatus.resolution, sizeof(lastStatus.resolution));

  Serial.printf("[UDPDetect] EXP/BIN: Tray=%d AV=0x%02X (%s) PIC=%d XboxVer=%d Enc=%s %dx%d (%s)\n",
                lastStatus.trayState,
                (lastStatus.avPackState & 0xFF),
                avPackString(lastStatus.avPackState).c_str(),
                lastStatus.picVersion,
                lastStatus.xboxVersion,
                encoderNameFromType(lastStatus.encoderType),
                lastStatus.videoWidth, lastStatus.videoHeight,
                lastStatus.resolution);
  gotPacket = true;
}

// Legacy ASCII on 50505 (optional)
static void parseExpansionAscii(char* buf, int n) {
  buf[n] = 0;
  char* save = nullptr;
  for (char* tok = strtok_r(buf, ";", &save); tok; tok = strtok_r(nullptr, ";", &save)) {
    char* eq = strchr(tok, '=');
    if (!eq) continue;
    *eq = 0;
    char* key = trim_inplace(tok);
    char* val = trim_inplace(eq + 1);
    for (char* p = key; *p; ++p) *p = (char)toupper((unsigned char)*p);

    if (!strcmp(key, "APP")) {
      safe_copy(lastStatus.currentApp, sizeof(lastStatus.currentApp), val);
    } else if (!strcmp(key, "RES")) {
      int w=0,h=0;
      if (sscanf(val, "%dx%d", &w, &h) == 2) {
        lastStatus.videoWidth=w; lastStatus.videoHeight=h;
        formatResolution(w,h,lastStatus.avPackState,lastStatus.resolution,sizeof(lastStatus.resolution));
      } else {
        safe_copy(lastStatus.resolution, sizeof(lastStatus.resolution), val);
      }
    } else if (!strcmp(key, "WIDTH")) {
      lastStatus.videoWidth = atoi(val);
      formatResolution(lastStatus.videoWidth,lastStatus.videoHeight,lastStatus.avPackState,lastStatus.resolution,sizeof(lastStatus.resolution));
    } else if (!strcmp(key, "HEIGHT")) {
      lastStatus.videoHeight = atoi(val);
      formatResolution(lastStatus.videoWidth,lastStatus.videoHeight,lastStatus.avPackState,lastStatus.resolution,sizeof(lastStatus.resolution));
    } else if (!strcmp(key, "ENCODER")) {
      // accept either address as hex or numeric bucket
      if (val[0]=='0' && (val[1]=='x'||val[1]=='X')) {
        lastStatus.encoderType = (int)strtol(val, nullptr, 16);
      } else {
        lastStatus.encoderType = atoi(val);
      }
    } else if (!strcmp(key, "AV") || !strcmp(key, "AVPACK") || !strcmp(key, "AVSTATE")) {
      lastStatus.avPackState = atoi(val);
      formatResolution(lastStatus.videoWidth,lastStatus.videoHeight,lastStatus.avPackState,lastStatus.resolution,sizeof(lastStatus.resolution));
    } else if (!strcmp(key, "PIC")) {
      lastStatus.picVersion = atoi(val);
    } else if (!strcmp(key, "XBOXVER") || !strcmp(key, "XBOXVERSION")) {
      lastStatus.xboxVersion = atoi(val);
    } else if (!strcmp(key, "TRAY")) {
      lastStatus.trayState = atoi(val);
    }
  }
  Serial.printf("[UDPDetect] EXP/TXT: App='%s' Res=%s W=%d H=%d Enc=%s AV=0x%02X (%s) PIC=%d XboxVer=%d Tray=%d\n",
                lastStatus.currentApp, lastStatus.resolution,
                lastStatus.videoWidth, lastStatus.videoHeight,
                encoderNameFromType(lastStatus.encoderType),
                (lastStatus.avPackState & 0xFF), avPackString(lastStatus.avPackState).c_str(),
                lastStatus.picVersion, lastStatus.xboxVersion, lastStatus.trayState);
  gotPacket = true;
}

// ==================== EEPROM (50506) ====================
static void parseEE_line(const char* line) {
  if (!strncmp(line, "EE:RAW=", 7)) {
    const char* b64 = line + 7;
    lastStatus.eeRawLen = base64_decode(b64, lastStatus.eeRaw, (int)sizeof(lastStatus.eeRaw));
    Serial.printf("[UDPDetect] EE RAW decoded: %d bytes\n", lastStatus.eeRawLen);
    gotPacket = true;
    return;
  }
  if (!strncmp(line, "EE:HDD=", 7)) {
    const char* hex = line + 7;
    safe_copy(lastStatus.eeHddHex, sizeof(lastStatus.eeHddHex), hex);
    Serial.printf("[UDPDetect] EE HDD: %s\n", lastStatus.eeHddHex);
    gotPacket = true;
    return;
  }
  if (!strncmp(line, "EE:SN=", 6)) {
    char tmp[1024];
    size_t L = strlen(line);
    if (L >= sizeof(tmp)) L = sizeof(tmp) - 1;
    memcpy(tmp, line + 3, L - 3); // skip "EE:"
    tmp[L - 3] = 0;

    char* save = nullptr;
    for (char* tok = strtok_r(tmp, "|", &save); tok; tok = strtok_r(nullptr, "|", &save)) {
      char* eq = strchr(tok, '=');
      if (!eq) continue;
      *eq = 0;
      char* key = trim_inplace(tok);
      char* val = trim_inplace(eq + 1);
      for (char* p = key; *p; ++p) *p = (char)toupper((unsigned char)*p);

      if      (!strcmp(key, "SN"))  { safe_copy(lastStatus.eeSerial, sizeof(lastStatus.eeSerial), val); }
      else if (!strcmp(key, "MAC")) { safe_copy(lastStatus.eeMac,    sizeof(lastStatus.eeMac),    val); }
      else if (!strcmp(key, "REG")) { safe_copy(lastStatus.eeRegion, sizeof(lastStatus.eeRegion), val); }
      else if (!strcmp(key, "HDD")) { safe_copy(lastStatus.eeHddHex, sizeof(lastStatus.eeHddHex), val); }
      else if (!strcmp(key, "RAW")) { lastStatus.eeRawLen = base64_decode(val, lastStatus.eeRaw, (int)sizeof(lastStatus.eeRaw)); }
    }
    Serial.printf("[UDPDetect] EE LBL: SN=%s MAC=%s REG=%s HDD=%s RAW=%dB\n",
                  lastStatus.eeSerial, lastStatus.eeMac, lastStatus.eeRegion,
                  lastStatus.eeHddHex, lastStatus.eeRawLen);
    gotPacket = true;
    return;
  }
}

// -------------------- public API --------------------
void UDPDetect::begin() {
  udpCore.begin(UDP_PORT_CORE);
  udpExp.begin(UDP_PORT_EXP);
  udpEE.begin(UDP_PORT_EE);
  gotPacket = false;

  memset(&lastStatus, 0, sizeof(lastStatus));
  lastStatus.fanSpeed    = -1;
  lastStatus.cpuTemp     = -1000;
  lastStatus.ambientTemp = -1000;
  lastStatus.trayState = lastStatus.avPackState = lastStatus.picVersion = -1;
  lastStatus.xboxVersion = lastStatus.encoderType = -1;
  lastStatus.videoWidth = lastStatus.videoHeight = -1;
  lastStatus.eeRawLen = 0;

  Serial.printf("[UDPDetect] Listening on core=%u, exp=%u, ee=%u\n",
                UDP_PORT_CORE, UDP_PORT_EXP, UDP_PORT_EE);
}

void UDPDetect::loop() {
  // --- CORE (50504): Fan/CPU/Ambient/App ---
  int sz = udpCore.parsePacket();
  if (sz == (int)sizeof(CorePacket)) {
    CorePacket cp;
    int n = udpCore.read(reinterpret_cast<char*>(&cp), sizeof(cp));
    if (n == (int)sizeof(cp)) {
      lastStatus.fanSpeed    = cp.fanSpeed;
      lastStatus.cpuTemp     = cp.cpuTemp;
      lastStatus.ambientTemp = cp.ambientTemp;
      safe_copy(lastStatus.currentApp, sizeof(lastStatus.currentApp), cp.currentApp);
      gotPacket = true;
      Serial.printf("[UDPDetect] CORE: Fan=%d CPU=%d Amb=%d App='%s'\n",
                    lastStatus.fanSpeed, lastStatus.cpuTemp,
                    lastStatus.ambientTemp, lastStatus.currentApp);
    } else {
      uint8_t tmp[256]; if (sz > (int)sizeof(tmp)) sz = sizeof(tmp);
      udpCore.read(tmp, sz);
    }
  } else if (sz > 0) {
    uint8_t tmp[256]; if (sz > (int)sizeof(tmp)) sz = sizeof(tmp);
    udpCore.read(tmp, sz);
  }

  // --- EXPANSION (50505): binary status (or legacy ASCII) ---
  sz = udpExp.parsePacket();
  if (sz > 0) {
    if (sz == 28) {
      uint8_t buf[28];
      int n = udpExp.read(buf, sizeof(buf));
      if (n == 28) parseExpansionBinary(buf, n);
    } else {
      char buf[256];
      if (sz > (int)sizeof(buf) - 1) sz = sizeof(buf) - 1;
      int n = udpExp.read(buf, sz);
      if (n > 0) parseExpansionAscii(buf, n);
    }
  }

  // --- EEPROM (50506): ASCII frames ---
  sz = udpEE.parsePacket();
  if (sz > 0) {
    char buf[1024];
    if (sz > (int)sizeof(buf) - 1) sz = sizeof(buf) - 1;
    int n = udpEE.read(buf, sz);
    if (n > 0) {
      buf[n] = 0;
      parseEE_line(buf);
    }
  }
}

bool UDPDetect::hasPacket() { return gotPacket; }
void UDPDetect::acknowledge() { gotPacket = false; }
const XboxStatus& UDPDetect::getLatest() { return lastStatus; }
