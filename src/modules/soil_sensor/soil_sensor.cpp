#include "soil_sensor.h"

SoilSensor::SoilSensor(uint8_t pinInput, uint8_t pinVCC, unsigned long checkPeriod) :
    _pinInput(pinInput),
    _pinVCC(pinVCC),
    _checkPeriod(checkPeriod),
    _state(SOIL_SENSOR_UNINITIALIZED),
    _humidity(0),
    _minHumidity(0),
    _maxHumidity(1023)
{
    // Конструктор инициализирует только переменные
    // begin() должен быть вызван отдельно для настройки пинов
}

void SoilSensor::begin() {
    pinMode(_pinInput, INPUT);
    pinMode(_pinVCC, OUTPUT);
    _state = SOIL_SENSOR_BEGIN_READING;
    // Первое чтение выполняем принудительно, чтобы пользователь сразу получил
    // актуальные данные, а датчик перешёл в правильное состояние FSM.
    _checkSensor(true);
}

uint8_t SoilSensor::getSensorData(bool forceCheck) {
    // При необходимости запускаем новый цикл чтения.
    _checkSensor(forceCheck || _timer.isReady());
    
    // Вычисляем относительную влажность на основе калибровочных значений
    // Используем int32_t для предотвращения переполнения при умножении
    int32_t relativeHumidity = (static_cast<int32_t>(_humidity) - _minHumidity) * 100 / (_maxHumidity - _minHumidity);
    
    // Ограничиваем результат диапазоном 0-100%
    if (relativeHumidity <= 0) return 0;
    else if (relativeHumidity >= 100) return 100;
    return static_cast<uint8_t>(relativeHumidity);
}

void SoilSensor::calibrate(int minHumidity, int maxHumidity) {
    // Ограничиваем значения диапазоном 0-1023
    if (minHumidity < 0) minHumidity = 0;
    else if (minHumidity > 1023) minHumidity = 1023;
    
    if (maxHumidity < 0) maxHumidity = 0;
    else if (maxHumidity > 1023) maxHumidity = 1023;
    
    // Предотвращаем деление на ноль: если min == max, калибровка не изменяется
    if (minHumidity == maxHumidity) return;
    
    _minHumidity = minHumidity;
    _maxHumidity = maxHumidity;
}

void SoilSensor::_checkSensor(bool forceCheck) {
    // Принудительная проверка: переходим в состояние начала чтения
    if (forceCheck && _state != SOIL_SENSOR_UNINITIALIZED) {
        _state = SOIL_SENSOR_BEGIN_READING;
    }
    
    switch (_state) {
        case SOIL_SENSOR_UNINITIALIZED:
            // Датчик не инициализирован, ничего не делаем
            return;
            
        case SOIL_SENSOR_BEGIN_READING:
            // Включаем питание датчика для защиты от коррозии
            digitalWrite(_pinVCC, HIGH);
            _timer.setPeriod(SOIL_SENSOR_STAB_PAUSE);
            _state = SOIL_SENSOR_READ_DATA;
            
            // Если требуется принудительная проверка, ждем синхронно
            // Это используется для немедленного получения данных
            if (forceCheck) {
                while (!_timer.isReady());
                _timer.setPeriod(0);
            } else {
                break;
            }
            
        case SOIL_SENSOR_READ_DATA:
            // Ждем стабилизации показаний, затем читаем данные
            if (_timer.isReady()) {
                // Сохраняем текущее показание АЦП — далее его можно конвертировать в проценты.
                _humidity = analogRead(_pinInput);
                // Выключаем питание датчика для защиты от коррозии
                digitalWrite(_pinVCC, LOW);
                _timer.setPeriod(_checkPeriod);
                _state = SOIL_SENSOR_IDLE;
            }
            break;
            
        case SOIL_SENSOR_IDLE:
            // Ожидание следующего цикла опроса
            if (_timer.isReady()) {
                _state = SOIL_SENSOR_BEGIN_READING;
            }
            break;
    }
}

