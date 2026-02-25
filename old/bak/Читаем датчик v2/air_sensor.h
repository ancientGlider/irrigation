#pragma once
#include <Arduino.h>
#include "timer.h"

#define AIR_SENSOR_UNINITIALIZED          0
#define AIR_SENSOR_READY                  1
#define AIR_SENSOR_BEGIN_READING          2
#define AIR_SENSOR_READ_DATA              3
#define AIR_SENSOR_IDLE                   4

#define AIR_SENSOR_DEFAULT_CHECK_PERIOD   2000  // время между опросами датчика по умолчанию, мс
#define AIR_SENSOR_TIME_TO_SYNC_READING   300   // время на синхронное чтение датчика (с блокировкой), мс
#define AIR_SENSOR_PULLUP_TIME            100   // время перед началом чтения, мс
#define AIR_SENSOR_START_SIGNAL_DURATION  20    // продолжительность стартового сигнала (низкого уровня), мс
#define AIR_SENSOR_TIME_TO_BEGIN_READING  5     // время на ответ сенсора после стартового сигнала, мс

#define AIR_SENSOR_WAITING_SIGNAL_TIME    100   // время ожидания импульса датчика в микросекундах

class AirSensor {
public:
    AirSensor(uint8_t pin, uint32_t checkPeriod = AIR_SENSOR_DEFAULT_CHECK_PERIOD);
    void begin();
    bool getSensorData(int *temperature, int *humidity);
    bool getSyncSensorData(int *temperature, int *humidity);

private:
    uint8_t _state = AIR_SENSOR_UNINITIALIZED;
    bool _initialized = false;
    Timer _timer;
    uint8_t data[5];
    const uint8_t _pin;
    uint32_t _checkPeriod;
    uint16_t _maxWaitingSignalCycles;

    int _getTemperature() const;
    int _getHumidity() const;

    bool _forceFSM();
    bool _processFSM();

    inline void _beginReading();
    bool _checkSensor();

    uint16_t _waitSignal(bool level) const;
};

class ManageInterrupts {
    public:
        ManageInterrupts();
        ~ManageInterrupts();
};