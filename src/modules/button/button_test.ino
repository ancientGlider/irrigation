/*
 * Button Module Test
 *
 * Wiring (per project specification):
 *  - Button OK     -> pin 2  (to GND, INPUT_PULLUP enabled automatically)
 *  - Button UP     -> pin 5
 *  - Button DOWN   -> pin 6
 *  - Button CANCEL -> pin 7
 *
 * Usage:
 * 1. Upload the sketch to Arduino Nano
 * 2. Open Serial Monitor at 9600 baud
 * 3. Test each button: short press, long press (>500 ms), release
 */

#include <Arduino.h>
#include "button.h"

#define BUTTON_PIN_OK     2
#define BUTTON_PIN_UP     5
#define BUTTON_PIN_DOWN   6
#define BUTTON_PIN_CANCEL 7

static Button buttons[] = {
    Button(BUTTON_PIN_OK),
    Button(BUTTON_PIN_UP),
    Button(BUTTON_PIN_DOWN),
    Button(BUTTON_PIN_CANCEL)
};

static const char* BUTTON_NAMES[] = {
    "OK",
    "UP",
    "DOWN",
    "CANCEL"
};

static uint8_t lastStates[sizeof(buttons) / sizeof(buttons[0])] = {
    BUTTON_UNPRESSED,
    BUTTON_UNPRESSED,
    BUTTON_UNPRESSED,
    BUTTON_UNPRESSED
};

static void printState(uint8_t index, uint8_t state) {
    Serial.print(BUTTON_NAMES[index]);
    Serial.print(F(": "));
    switch (state) {
        case BUTTON_UNPRESSED:
            Serial.println(F("UNPRESSED"));
            break;
        case BUTTON_PRESSED:
            Serial.println(F("PRESSED"));
            break;
        case BUTTON_LONGPRESSED:
            Serial.println(F("LONGPRESSED"));
            break;
        default:
            Serial.println(F("UNKNOWN"));
            break;
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Button Module Test ==="));
    Serial.println();

    const size_t buttonCount = sizeof(buttons) / sizeof(buttons[0]);
    for (size_t i = 0; i < buttonCount; ++i) {
        buttons[i].begin();
    }

    Serial.println(F("Buttons initialized."));
    Serial.println(F("Test steps for each button (OK, UP, DOWN, CANCEL):"));
    Serial.println(F("  1) Tap quickly"));
    Serial.println(F("  2) Hold for >500 ms"));
    Serial.println(F("  3) Release"));
    Serial.println();
}

void loop() {
    const size_t buttonCount = sizeof(buttons) / sizeof(buttons[0]);

    for (size_t i = 0; i < buttonCount; ++i) {
        buttons[i].check();
        uint8_t state = buttons[i].getState();
        if (state != lastStates[i]) {
            printState(static_cast<uint8_t>(i), state);
            lastStates[i] = state;
        }
    }
}
