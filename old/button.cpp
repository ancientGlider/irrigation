#include "button.h"

Button::Button(uint8_t pin) : _pin(pin) {}

void Button::begin() {
    pinMode(_pin, INPUT_PULLUP);
    _btState = digitalRead(_pin) == LOW ? BUTTON_PRESSED : BUTTON_UNPRESSED;
    _timer = millis();
}

void Button::check() {
    bool currentState = digitalRead(_pin) == LOW;

    if ((_btState > BUTTON_UNPRESSED) != currentState && millis() - _timer >= BUTTON_DEBOUNCE_TIME) {
        _timer = millis();
        _btState = currentState ? BUTTON_PRESSED : BUTTON_UNPRESSED;
    }

    if ((_btState == BUTTON_PRESSED) && millis() - _timer >= BUTTON_LONGPRESS_TIME) {
        _timer = millis();
        _btState = BUTTON_LONGPRESSED;
    }
}

uint8_t Button::getState() {
    return _btState;
}