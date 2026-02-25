/*
Данный класс реализует таймер на базе часов (RTC) DS1302, В СЕКУНДАХ
Работает в течение 100 лет, затем циклически продолжает с 0
Значение секунд варьируется от 0 до 3155759999 (100 лет с начала дня 01.01.00 до конца дня 31.12.99)
Требуется библиотека <Ds1302.h>
*/

#pragma once
#include <Arduino.h>
#include <Ds1302.h>
#include "timer.h"

#define MAXIMUM_YEAR_VALUE 136            // максимальный год в таймере
#define DEFAULT_RTC_POLLING_PERIOD 6000//00 // синхронизация с RTC один раз в 10 минут (600000 миллисекунд)

class TimerRTC {
    public:

        TimerRTC(unsigned long period = 0); 
        /*
        period - период срабатывания таймера, через который isReady() 
            становится true и сбрасывает таймер */
        
        static void begin(uint8_t pin_ena, uint8_t pin_clk, uint8_t pin_dat);
        
        static void end();

        unsigned long getTime();               // время, прошедшее с момента инициализации или сброса таймера (с)
        void setTime(unsigned long seconds, bool dropTimer = false);   // устанавливает текущее время
        unsigned long getInitTime();               // возвращает значение _initTime, например, для сохранения в EEPROM
        void setInitTime(unsigned long initTime);   // устанавливает значение _initTime
        bool isReady();                        // возвращает true и сбрасывает таймер, если с момента инициализации или сброса прошел period
        void setPeriod(unsigned long period, bool dropTimer = false);  // устанавливает period, таймер НЕ сбрасывает
        unsigned long getPeriod();             // возвращает заданный period
        void drop();                            // сбрасывает таймер

//        Ds1302::DateTime getDateTime();

    private:
        bool _initialized = false;
        inline static Ds1302::DateTime _dateTime = {0, 1, 1, 0, 0, 0, 1};
        unsigned long _initTime;
        unsigned long _period = 0;
        inline static unsigned long _seconds = 0;
        inline static Ds1302* _RTC = nullptr;
        inline static Timer _timer = Timer(DEFAULT_RTC_POLLING_PERIOD);

        inline static const uint16_t _daysBefore[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

        void _updateTime();
//        void _convertTimeFromSeconds();
        void _initialize();
        inline bool _isLeap(uint8_t year);
        inline uint16_t _leapsBefore(uint8_t year);
        inline static Ds1302& RTC();
};