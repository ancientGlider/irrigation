/*
 * Button module
 *
 * Provides debounced button handling with short/long press detection
 * without using blocking delays. The class relies on the projects
 * `Timer` module to track debounce and long-press periods.
 */

#pragma once
#include <Arduino.h>

// Public button states reported through getState()
#define BUTTON_UNPRESSED      0  // Button released or idle
#define BUTTON_PRESSED        1  // Button pressed (short press)
#define BUTTON_LONGPRESSED    2  // Button pressed longer than BUTTON_LONGPRESS_TIME

// Timing constants (milliseconds)
#define BUTTON_DEBOUNCE_TIME  25   // Debounce filter window
#define BUTTON_LONGPRESS_TIME 500  // Minimum press duration to trigger long press

class Button {
public:
    /**
     * @param pin Digital pin configured with INPUT_PULLUP connected to button
     */
    explicit Button(uint8_t pin);

    /**
     * Initializes the hardware pin and priming state machine.
     * Call once in setup().
     */
    void begin();

    /**
     * Performs one iteration of the button FSM.
     * Call from loop() as often as possible.
     */
    void check();

    /**
     * Returns the current logical state (unpressed/pressed/long pressed).
     */
    uint8_t getState() const;

private:
    const uint8_t _pin;      // Button pin (INPUT_PULLUP)
    uint8_t _btState;        // Current state reported to the user
    unsigned long _timer;    // Timestamp of last state change
};
