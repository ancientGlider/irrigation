#include "button.h"

Button::Button(uint8_t pin) : _pin(pin), _btState(BUTTON_UNPRESSED), _timer(0) {}

void Button::begin() {
    pinMode(_pin, INPUT_PULLUP);

    // Сразу определяем фактическое состояние кнопки.
    // LOW соответствует нажатию (pull-up), HIGH — отпусканию.
    _btState = (digitalRead(_pin) == LOW) ? BUTTON_PRESSED : BUTTON_UNPRESSED;
    _timer = millis();
}

void Button::check() {
    bool isPressed = (digitalRead(_pin) == LOW);
    unsigned long now = millis();

    // --- Антидребезг ---
    // Используем одно сравнение: состояние меняем только когда логическая
    // фиксация (pressed/unpressed) стабильна BUTTON_DEBOUNCE_TIME миллисекунд.
    // В исходном FSM значение `_btState` содержит актуальное логическое
    // состояние. Сравнение (_btState > BUTTON_UNPRESSED) эквивалентно
    // «кнопка сейчас считается нажатой».
    if (((_btState > BUTTON_UNPRESSED) != isPressed) && (now - _timer >= BUTTON_DEBOUNCE_TIME)) {
        _timer = now;
        _btState = isPressed ? BUTTON_PRESSED : BUTTON_UNPRESSED;
    }

    // --- Длинное нажатие ---
    // Когда кнопка находится в состоянии BUTTON_PRESSED достаточное время,
    // переводим FSM в BUTTON_LONGPRESSED и обнуляем таймер, чтобы длинное
    // нажатие срабатывало один раз за цикл удержания.
    if ((_btState == BUTTON_PRESSED) && (now - _timer >= BUTTON_LONGPRESS_TIME)) {
        _timer = now;
        _btState = BUTTON_LONGPRESSED;
    }
}

uint8_t Button::getState() const {
    return _btState;
}
