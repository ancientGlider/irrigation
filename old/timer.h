/*
Данный класс реализует таймер на базе встроенной функции millis();
При переопределении прерываний таймеров может работать некорректно
*/

#pragma once
#include <Arduino.h>

class Timer {
    public:
        Timer(unsigned long period = 0, bool dropTimer = true);
        /* period - период срабатывания таймера, через который isReady() 
            становится true и сбрасывает таймер */
        unsigned long getTime();               // время, прошедшее с момента инициализации или сброса таймера (мс)
        bool isReady();                        // возвращает true и сбрасывает таймер, если с момента инициализации или сброса прошел period
        void setPeriod(unsigned long period, bool dropTimer = true);  // устанавливает period, если dporTimer == true - сбрасывает таймер
        unsigned long getPeriod();             // возвращает заданный period
        void drop();                            // сбрасывает таймер

    private:
        unsigned long _initTime;
        unsigned long _period;
};