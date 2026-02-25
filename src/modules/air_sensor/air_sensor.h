/*
 * Модуль датчика температуры и влажности воздуха (DHT11/DHT22)
 *
 * Реализует асинхронное чтение данных с датчиков DHT11/DHT22 при помощи
 * конечного автомата (FSM) и системного таймера без использования delay().
 * Поддерживает принудительное обновление данных и защиту критичных участков
 * от прерываний для корректного считывания таймингов протокола DHT.
 */

#pragma once
#include <Arduino.h>
#include "../timer/timer.h"

// Состояния конечного автомата
#define AIR_SENSOR_UNINITIALIZED          0
#define AIR_SENSOR_READY                  1
#define AIR_SENSOR_BEGIN_READING          2
#define AIR_SENSOR_READ_DATA              3
#define AIR_SENSOR_IDLE                   4

// Константы (в миллисекундах, если не указано иное)
#define AIR_SENSOR_DEFAULT_CHECK_PERIOD   2000UL  // Период опроса по умолчанию (2 секунды)
#define AIR_SENSOR_TIME_TO_SYNC_READING   300UL   // Время, в течение которого ожидаем завершение принудительного чтения
#define AIR_SENSOR_PULLUP_TIME            100UL   // Время удержания высокого уровня перед началом чтения
#define AIR_SENSOR_START_SIGNAL_DURATION  20UL    // Длительность стартового низкого уровня
#define AIR_SENSOR_TIME_TO_BEGIN_READING  5UL     // Время ожидания ответа датчика после стартового сигнала

#define AIR_SENSOR_WAITING_SIGNAL_TIME    100U    // Максимальное время ожидания одного импульса (микросекунды)

class AirSensor {
public:
    /**
     * @param pin Цифровой пин Arduino, к которому подключен датчик DHT
     * @param checkPeriod Периодичность опроса датчика (мс)
     */
    AirSensor(uint8_t pin, unsigned long checkPeriod = AIR_SENSOR_DEFAULT_CHECK_PERIOD);

    /**
     * Подготавливает пин и выполняет первичное чтение для инициализации датчика.
     */
    void begin();

    /**
     * Возвращает температуру и влажность воздуха (значения в десятых долях).
     *
     * @param temperature Указатель для записи температуры (в десятых градуса Цельсия)
     * @param humidity Указатель для записи влажности (в десятых процента)
     * @param forceCheck Если true — выполняет немедленное чтение, иначе использует FSM
     * @return true, если получены новые валидные данные, иначе false
     */
    bool getSensorData(int* temperature, int* humidity, bool forceCheck = false);

private:
    uint8_t _state;                 // Текущее состояние FSM
    Timer _timer;                   // Таймер для внутренних задержек датчика
    uint8_t _data[5];               // Буфер из 5 байт (4 данных + checksum)
    const uint8_t _pin;             // Пин, к которому подключён датчик
    unsigned long _checkPeriod;     // Период опроса (в миллисекундах)
    uint16_t _maxWaitingSignalCycles; // Лимит количества чтений, соответствующий 100 мкс

    int _getTemperature() const;    // Возвращает температуру в десятых градуса
    int _getHumidity() const;       // Возвращает влажность в десятых процента

    bool _forceFSM();               // Выполняет FSM синхронно (forceCheck)
    bool _processFSM();             // Крутит FSM в обычном режиме (неблокирующе)

    inline void _beginReading() {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
    }

    bool _checkSensor();            // Полный цикл чтения 40 бит из датчика
    uint16_t _waitSignal(bool level) const; // Ожидание уровня с учётом тайм-аутов
};

class ManageInterrupts {
public:
    ManageInterrupts() {
        noInterrupts();
    }

    ~ManageInterrupts() {
        interrupts();
    }
};
