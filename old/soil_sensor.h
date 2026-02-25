#pragma once
#include <Arduino.h>
#include "timer.h"

#define SOIL_SENSOR_DEFAULT_CHECK_PERIOD    600000 // периодичность опроса физического датчика по умолчанию в миллисекундах (600000 мс = 10 мин)
#define SOIL_SENSOR_STAB_PAUSE              60      // время, необходимое для стабилизации показаний физического датчика, мс

#define SOIL_SENSOR_UNINITIALIZED       0
#define SOIL_SENSOR_BEGIN_READING       1
#define SOIL_SENSOR_READ_DATA           2
#define SOIL_SENSOR_IDLE                3

class SoilSensor {
    public:
        SoilSensor (uint8_t pinInput, uint8_t pinVCC, uint16_t checkPeriod = SOIL_SENSOR_DEFAULT_CHECK_PERIOD);  // pinInput - аналоговый пин контроллера, куда подключен датчик
                                                                                                     // pinVCC - пин контроллера, управляющий питанием датчика
                                                                                                     // checkPeriod - периодичность опроса физического датчика в мс

        void begin();

        uint8_t getSensorData(bool forceCheck = false);         // возвращает расчитанную относительную влажность, forceCheck = true вызывает принудительную проверку физического датчика
        int getRawSensorData(bool forceCheck = false);                               // возвращает показания физического датчика
        void calibrate(int minHumidity, int maxHumidity);     // устанавливает калибровочные значения для расчёта относительной влажности

    private:
        uint32_t _checkPeriod;
        uint8_t _state = SOIL_SENSOR_UNINITIALIZED;
        const uint8_t _pinInput, _pinVCC;                     // пины контроллера, к которым подключен датчик
        Timer _timer;                                         // таймер для обеспечения периодичности опроса датчика
        int _humidity;                                        // значение, возвращаемое физическим датчиком
        int _minHumidity = 0;                                 // калибровочное значение, соответствующее влажности 0%
        int _maxHumidity = 1023;                              // калибровочное значение, соответствующее влажности 100%

        void _checkSensor(bool forceCheck);
};