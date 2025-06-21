#pragma once

#include <Arduino.h>

namespace Beep {
    void begin(int pin);
    void playMorseXBOX();
    void update();
}
