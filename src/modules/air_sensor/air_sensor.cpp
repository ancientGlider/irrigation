#include "air_sensor.h"
#include <string.h>

AirSensor::AirSensor(uint8_t pin, unsigned long checkPeriod)
    : _state(AIR_SENSOR_UNINITIALIZED),
      _data{0, 0, 0, 0, 0},
      _pin(pin),
      _checkPeriod(checkPeriod),
      _maxWaitingSignalCycles(0) {
    // Конструктор лишь сохраняет параметры; оборудование настраивается в begin().
}

void AirSensor::begin() {
    uint16_t count = 0;

    pinMode(_pin, INPUT_PULLUP);

    // Оценка времени одного цикла digitalRead для вычисления максимально допустимого ожидания
    unsigned long startMicros = micros();
    while (true) {
        (void)digitalRead(_pin);
        if (count++ >= 1000U) {
            break;
        }
    }
    // Фиксируем длительность 1000 вызовов digitalRead(). Зная это время, можно
    // приблизительно оценить количество таких вызовов, помещающихся в 100 мкс.
    unsigned long elapsed = micros() - startMicros;
    if (elapsed == 0UL) {
        elapsed = 1UL;
    }

    // Вычисляем количество циклов, соответствующее AIR_SENSOR_WAITING_SIGNAL_TIME микросекунд
    _maxWaitingSignalCycles = static_cast<uint16_t>((AIR_SENSOR_WAITING_SIGNAL_TIME * 1000UL) / elapsed) + 1U;

    _state = AIR_SENSOR_READY;
    if (!_forceFSM()) {
        _state = AIR_SENSOR_UNINITIALIZED;
    }
}

bool AirSensor::getSensorData(int* temperature, int* humidity, bool forceCheck) {
    if (_state == AIR_SENSOR_UNINITIALIZED) {
        return false;
    }

    // В обычном режиме FSM работает неблокирующе; forceCheck позволяет получить
    // данные немедленно, что полезно в тестах и для ручных обновлений.
    bool updated = forceCheck ? _forceFSM() : _processFSM();

    if (temperature != nullptr) {
        *temperature = _getTemperature();
    }
    if (humidity != nullptr) {
        *humidity = _getHumidity();
    }

    return updated;
}

int AirSensor::_getTemperature() const {
    int value = static_cast<int>(_data[2]);
    if ((_data[3] & 0x80U) != 0U) {
        value = -1 - value;
    }
    return value * 10 + static_cast<int>(_data[3] & 0x0FU);
}

int AirSensor::_getHumidity() const {
    return static_cast<int>(_data[0]) * 10 + static_cast<int>(_data[1]);
}

bool AirSensor::_forceFSM() {
    // Отдельный таймер ограничивает время синхронного ожидания, чтобы не
    // блокировать выполнение программы, даже если сенсор не отвечает.
    Timer syncTimer(AIR_SENSOR_TIME_TO_SYNC_READING);

    if (_state == AIR_SENSOR_IDLE) {
        _state = AIR_SENSOR_READY;
    }

    while (!syncTimer.isReady()) {
        if (_processFSM()) {
            return true;
        }
    }
    return false;
}

bool AirSensor::_processFSM() {
    // Пошагово выполняем операции, соответствующие текущему состоянию датчика.
    switch (_state) {
        case AIR_SENSOR_READY:
            // Датчик «отдохнул», можно начинать новый цикл измерения.
            _timer.setPeriod(AIR_SENSOR_PULLUP_TIME);
            _state = AIR_SENSOR_BEGIN_READING;
            break;

        case AIR_SENSOR_BEGIN_READING:
            if (_timer.isReady()) {
                // Формируем стартовый импульс: переводим линию в LOW на 20 мс.
                _beginReading();
                _timer.setPeriod(AIR_SENSOR_START_SIGNAL_DURATION);
                _state = AIR_SENSOR_READ_DATA;
            }
            break;

        case AIR_SENSOR_READ_DATA:
            if (_timer.isReady()) {
                // Импульс сформирован, возвращаемся в режим ожидания и запускаем чтение.
                _state = AIR_SENSOR_IDLE;
                _timer.setPeriod(_checkPeriod);
                if (_checkSensor()) {
                    return true;
                }
            }
            break;

        case AIR_SENSOR_IDLE:
            if (_timer.isReady()) {
                // Период опроса истёк — запускаем следующий цикл измерений.
                _state = AIR_SENSOR_READY;
            }
            break;

        default:
            break;
    }

    return false;
}

bool AirSensor::_checkSensor() {
    // После стартового импульса датчик должен ответить в течение нескольких миллисекунд.
    // Таймер страхует нас от зависания, если реакция не последовала.
    Timer waitTimer(AIR_SENSOR_TIME_TO_BEGIN_READING);

    pinMode(_pin, INPUT_PULLUP);

    if (_maxWaitingSignalCycles == 0U) {
        return false;
    }

    // Ожидаем переход в LOW, которым датчик подтверждает готовность передавать данные.
    while (digitalRead(_pin) == HIGH) {
        if (waitTimer.isReady()) {
            return false;
        }
    }

    {
        volatile ManageInterrupts manage;

        if (_waitSignal(LOW) == 0U) {
            return false;
        }
        if (_waitSignal(HIGH) == 0U) {
            return false;
        }

        for (uint8_t i = 0; i < 40U; ++i) {
            _data[i / 8U] <<= 1;
            uint16_t lowDuration = _waitSignal(LOW);
            if (lowDuration == 0U) {
                return false;
            }
            uint16_t highDuration = _waitSignal(HIGH);
            if (highDuration == 0U) {
                return false;
            }
            // Если высокий уровень длится дольше, чем низкий, это «1», иначе — «0».
            if (highDuration > lowDuration) {
                _data[i / 8U] |= 0x01U;
            }
        }
    }

    // Контрольная сумма — последний байт должен совпадать с суммой предыдущих четырёх.
    uint8_t checksum = static_cast<uint8_t>((_data[0] + _data[1] + _data[2] + _data[3]) & 0xFFU);
    if (checksum != _data[4]) {
        return false;
    }

    return true;
}

uint16_t AirSensor::_waitSignal(bool level) const {
    uint16_t count = 0;
    while (digitalRead(_pin) == level) {
        if (count++ > _maxWaitingSignalCycles) {
            return 0;
        }
    }
    return count;
}
