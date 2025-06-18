#ifndef UI_SET_H
#define UI_SET_H

#include <Arduino.h>
#include "disp_cfg.h"

class UISet {
public:
    static void begin(LGFX* tft);
    static bool isMenuVisible();
    static void update();
};

#endif
