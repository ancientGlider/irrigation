#include "soil_sensor.h"

SoilSensor::SoilSensor(uint8_t pinInput, uint8_t pinVCC, uint16_t checkPeriod = SOIL_SENSOR_DEFAULT_CHECK_PERIOD) :
    _pinInput(pinInput),
    _pinVCC(pinVCC),
    _checkPeriod(checkPeriod) 
{}

void SoilSensor::begin() {
    pinMode(_pinInput, INPUT);
    pinMode(_pinVCC, OUTPUT);
    _state = SOIL_SENSOR_BEGIN_READING;
    _checkSensor(true);    
}

uint8_t SoilSensor::getSensorData(bool forceCheck = false) {
    _checkSensor(forceCheck || _timer.isReady());

    int32_t relativeHumidity = (static_cast<int32_t>(_humidity) - _minHumidity) * 100 / (_maxHumidity - _minHumidity);

    if (relativeHumidity <= 0) return 0;
    else if (relativeHumidity >= 100) return 100;
    return static_cast<uint8_t>(relativeHumidity);
}

int SoilSensor::getRawSensorData(bool forceCheck = false) {
    _checkSensor(forceCheck  || _timer.isReady());
    return _humidity;
}

void SoilSensor::calibrate(int minHumidity, int maxHumidity) {
    if (minHumidity < 0) minHumidity = 0;
    else if (minHumidity > 1023) minHumidity = 1023;
    if (maxHumidity < 0) maxHumidity = 0;
    else if (maxHumidity > 1023) maxHumidity = 1023;
    if (minHumidity == maxHumidity) return;
    _minHumidity = minHumidity;
    _maxHumidity = maxHumidity;
}

void SoilSensor::_checkSensor(bool forceCheck) {
    if (forceCheck && _state != SOIL_SENSOR_UNINITIALIZED) _state = SOIL_SENSOR_BEGIN_READING;
    switch (_state) {
        case SOIL_SENSOR_UNINITIALIZED:
            return;
        case SOIL_SENSOR_BEGIN_READING:
            digitalWrite(_pinVCC, HIGH);
            _timer.setPeriod(SOIL_SENSOR_STAB_PAUSE);
            _state = SOIL_SENSOR_READ_DATA;
            if (forceCheck) {
                while (!_timer.isReady());
                _timer.setPeriod(0);
            }
            else break;
        case SOIL_SENSOR_READ_DATA:
            if (_timer.isReady()) {
                _humidity = analogRead(_pinInput);
                digitalWrite(_pinVCC, LOW);
                _timer.setPeriod(_checkPeriod);
                _state = SOIL_SENSOR_IDLE;
            }
            break;
        case SOIL_SENSOR_IDLE:
            if (_timer.isReady()) _state = SOIL_SENSOR_BEGIN_READING;
    }
}