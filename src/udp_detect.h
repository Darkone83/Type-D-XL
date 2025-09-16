#pragma once
#include <Arduino.h>
#include "xbox_status.h"

namespace UDPDetect {

    // Default UDP ports used by Type-D XL
    static constexpr uint16_t kPortCore = 50504;   // fan/cpu/ambient/app
    static constexpr uint16_t kPortExp  = 50505;   // expansion binary status
    static constexpr uint16_t kPortEE   = 50506;   // EEPROM broadcasts

    // Which channel has new data (optional selector for hasPacket)
    enum class Channel : uint8_t {
        Any       = 0,
        Core      = 1,
        Expansion = 2,
        EEPROM    = 3,
    };

    // --- Initialization ---
    // Backward-compatible: binds to kPortCore/kPortExp/kPortEE
    void begin();

    // New: explicitly choose ports (useful if firmware changes)
    void begin(uint16_t corePort, uint16_t expPort, uint16_t eePort);

    // --- Pump / status ---
    void loop();

    // Backward-compatible: true if any channel received an update
    bool hasPacket();

    // New: check for a specific channel
    bool hasPacket(Channel ch);

    // Retrieve the latest aggregate status (core + expansion + EE fields)
    const XboxStatus& getLatest();

    // Clear the “new packet” flags (all channels)
    void acknowledge();

    // New: clear only one channel’s “new packet” flag (optional)
    void acknowledge(Channel ch);

} // namespace UDPDetect
