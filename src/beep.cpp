#include "beep.h"
#include "TCA9554PWR.h"

static int _buzzerPin = -1;

// Helper: ensure buzzer off
static void exio8_noTone() {
    Set_EXIO(8, 0);
}

// Helper: beep ON for duration_ms, then off
static void exio8_beep(int duration_ms) {
    Set_EXIO(8, 1);
    delay(duration_ms);
    Set_EXIO(8, 0);
}

namespace Beep {

void begin(int pin) {
    _buzzerPin = pin;
    // Optionally: Mode_EXIO(8, 0); // Set as output, if not already
}

void playMorseXBOX() {
    if (_buzzerPin < 0) return;

    const int DOT = 120;
    const int DASH = 360;
    const int PAUSE = 120;
    const int LTR_PAUSE = 400;
    const int WORD_PAUSE = 1000;

    // X: –··–
    exio8_beep(DASH);   delay(PAUSE);
    exio8_beep(DOT);    delay(PAUSE);
    exio8_beep(DOT);    delay(PAUSE);
    exio8_beep(DASH);   delay(LTR_PAUSE);

    // B: –···
    exio8_beep(DASH);   delay(PAUSE);
    exio8_beep(DOT);    delay(PAUSE);
    exio8_beep(DOT);    delay(PAUSE);
    exio8_beep(DOT);    delay(LTR_PAUSE);

    // O: – – –
    exio8_beep(DASH);   delay(PAUSE);
    exio8_beep(DASH);   delay(PAUSE);
    exio8_beep(DASH);   delay(LTR_PAUSE);

    // X: –··–
    exio8_beep(DASH);   delay(PAUSE);
    exio8_beep(DOT);    delay(PAUSE);
    exio8_beep(DOT);    delay(PAUSE);
    exio8_beep(DASH);   delay(WORD_PAUSE);

    exio8_noTone();
}

// Placeholder for interface; does nothing for Morse
void update() {}

} // end namespace Beep
