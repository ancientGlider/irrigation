/*
 * Модуль таймера на базе часов реального времени (RTC) DS1302
 * 
 * Реализует таймер на базе часов реального времени DS1302.
 * Работает в секундах (не в миллисекундах).
 * Работает в течение 100 лет, затем циклически продолжает с 0.
 * Значение секунд варьируется от 0 до 3155759999 (100 лет с начала дня 01.01.00 до конца дня 31.12.99)
 * 
 * Требуется библиотека <Ds1302.h>
 */

#pragma once
#include <Arduino.h>
#include <Ds1302.h>
#include "../timer/timer.h"

#define MAXIMUM_YEAR_VALUE 136                    // Максимальный год в таймере
#define DEFAULT_RTC_POLLING_PERIOD 600000UL       // Синхронизация с RTC один раз в 10 минут (600000 миллисекунд)
#define MAX_SECONDS 3155760000UL                  // Максимальное значение секунд (100 лет)

class TimerRTC {
public:
    /**
     * Конструктор таймера RTC
     * 
     * @param period Период срабатывания таймера в секундах.
     *               Через этот период isReady() вернет true и сбросит таймер.
     */
    TimerRTC(unsigned long period = 0);
    
    /**
     * Инициализация RTC модуля
     * 
     * @param pin_ena Пин ENA (enable) для DS1302
     * @param pin_clk Пин CLK (clock) для DS1302
     * @param pin_dat Пин DAT (data) для DS1302
     */
    static void begin(uint8_t pin_ena, uint8_t pin_clk, uint8_t pin_dat);
    
    /**
     * Завершение работы с RTC модулем
     * Освобождает память, выделенную для RTC
     */
    static void end();
    
    /**
     * Возвращает количество секунд, прошедших с момента
     * последнего сброса таймера (через drop() или при создании).
     * 
     * @return Время в секундах
     * 
     * Примечание: Автоматически синхронизируется с RTC периодически.
     * Корректно обрабатывает переполнение счетчика секунд благодаря модульной арифметике.
     */
    unsigned long getTime();
    
    /**
     * Устанавливает текущее время в секундах
     * 
     * @param seconds Время в секундах (0 - MAX_SECONDS)
     * @param dropTimer Если true, таймер сбрасывается после установки времени
     */
    void setTime(unsigned long seconds, bool dropTimer = false);
    
    /**
     * Возвращает значение initTime (для сохранения в EEPROM)
     * 
     * @return Значение initTime в секундах
     */
    unsigned long getInitTime() const;
    
    /**
     * Устанавливает значение initTime (для восстановления из EEPROM)
     * 
     * @param initTime Значение initTime в секундах
     */
    void setInitTime(unsigned long initTime);
    
    /**
     * Проверяет, истек ли заданный период.
     * Если период истек, автоматически сбрасывает таймер.
     * 
     * @return true если период истек, false в противном случае
     */
    bool isReady();
    
    /**
     * Устанавливает новый период срабатывания таймера.
     * 
     * @param period Новый период в секундах
     * @param dropTimer Если true, таймер сбрасывается после установки периода
     */
    void setPeriod(unsigned long period, bool dropTimer = false);
    
    /**
     * Возвращает текущий установленный период таймера.
     * 
     * @return Период в секундах
     */
    inline unsigned long getPeriod() const {
        return _period;
    }
    
    /**
     * Сбрасывает таймер (осуществляет его перезапуск).
     * Время отсчета начинается заново с текущего момента.
     */
    void drop();

private:
    bool _initialized;
    unsigned long _initTime;
    unsigned long _period;
    
    // Статические переменные для работы с RTC (общие для всех экземпляров)
    static Ds1302::DateTime _dateTime;   // Буфер для чтения даты/времени из RTC
    static unsigned long _seconds;       // Текущее время в секундах с начала «эпохи» (01.01.00)
    static Ds1302* _RTC;                 // Указатель на аппаратный RTC (создаётся лениво)
    static Timer _timer;                 // Таймер периодической синхронизации с RTC
    static const uint16_t _daysBefore[12]; // Накопленные дни до начала каждого месяца
    
    /**
     * Обновляет время из RTC
     */
    void _updateTime();
    
    /**
     * Инициализирует таймер (вызывается автоматически при первом использовании)
     */
    void _initialize();
    
    /**
     * Проверяет, является ли год високосным
     * 
     * @param year Год (0-136)
     * @return true если год високосный
     */
    inline static bool _isLeap(uint8_t year) {
        return year % 4 == 0;
    }
    
    /**
     * Возвращает количество високосных лет до указанного года
     * 
     * @param year Год (0-136)
     * @return Количество високосных лет
     */
    inline static uint16_t _leapsBefore(uint8_t year) {
        return (year + 3) / 4;
    }
    
    /**
     * Возвращает ссылку на объект RTC
     * 
     * @return Ссылка на Ds1302
     */
    inline static Ds1302& RTC() {
        return *_RTC;
    }
};

