// ui.h
#ifndef UI_H
#define UI_H

#include <Arduino.h>
#include "disp_cfg.h"

namespace UI {
    void begin(LGFX* tft);
    void update();
    bool isMenuVisible();
    void showMenu();
    void drawMenu();
    uint8_t getLastGesture(); // Returns gesture enum (see GESTURE)
    int getTouchX();      // Returns last X (0 if none)
    int getTouchY();      // Returns last Y (0 if none)
}

#endif
