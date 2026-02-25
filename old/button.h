/*
Данный класс реализует работу (опросы, статусы и т.д.) кнопки, подключенной к любому пину контроллера.
Предполагается, что для работы кнопки используется состояние пина INPUT_PULLUP
*/

#pragma once
#include <Arduino.h>

// статусы кнопки
#define BUTTON_UNPRESSED          0                // не нажата
#define BUTTON_PRESSED            1                // нажата
#define BUTTON_LONGPRESSED        2                // длинное нажатие

#define BUTTON_DEBOUNCE_TIME      25               // время на фиксацию состояния кнопки для устранения дребезга, мс
#define BUTTON_LONGPRESS_TIME     500              // время нажатия, приводящее к установлению статуса длинного нажатия, мс

class Button {
    public:
        Button(uint8_t pin);
        void begin();
        void check();
        uint8_t getState();

    private:
        uint8_t _pin;
        uint8_t _btState = BUTTON_UNPRESSED;
        uint32_t _timer;
};