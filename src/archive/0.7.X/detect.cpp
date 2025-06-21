#include <WiFi.h>
#include <WiFiUdp.h>
#include "detect.h"

// ==== CONFIGURABLES ====
#define DETECT_BROADCAST_PORT 50502
#define DETECT_BROADCAST_INTERVAL 3000 // ms between ID broadcasts
#define DETECT_ID_MSG_PREFIX "TYPE_D_ID:"

namespace Detect {

const uint8_t deviceId = 5;
uint32_t lastBroadcast = 0;
WiFiUDP udpBroadcast;

void begin() {
  udpBroadcast.begin(DETECT_BROADCAST_PORT); // Not strictly required for send
  Serial.println("[Detect] ID is statically set to 5.");
}

void broadcastId() {
  if (WiFi.status() != WL_CONNECTED) return;
  char msg[32];
  snprintf(msg, sizeof(msg), DETECT_ID_MSG_PREFIX "%d", deviceId);
  udpBroadcast.beginPacket("255.255.255.255", DETECT_BROADCAST_PORT);
  udpBroadcast.write((const uint8_t *)msg, strlen(msg));
  udpBroadcast.endPacket();
  Serial.printf("[Detect] Status broadcast: %s\n", msg);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (millis() - lastBroadcast > DETECT_BROADCAST_INTERVAL) {
    lastBroadcast = millis();
    broadcastId();
  }
}

uint8_t getId() { return deviceId; }

} // namespace Detect
