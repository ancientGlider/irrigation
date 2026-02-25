#include "air_sensor.h"

/*
 Конструктор сохраняет пин и устанавливает FSM в статус "не инициализировано"
 */
AirSensor::AirSensor(uint8_t pin, uint32_t checkPeriod = 2000) : _pin(pin), _checkPeriod(checkPeriod) {
    _state = AIR_SENSOR_UNINITIALIZED;

}

/*
 Устанавливает режим пина, сбрасывает таймер и запускает FSM
*/
void AirSensor::begin() {
    uint16_t count = 0;
    
    pinMode(_pin, INPUT_PULLUP);
    
    uint32_t timer = micros();

    while (true) {
        digitalRead(_pin);
        if (count++ >= 1000) break;
    }

    timer = micros() - timer;

    _maxWaitingSignalCycles = static_cast<uint16_t>(AIR_SENSOR_WAITING_SIGNAL_TIME * 1000UL / timer) + 1;

    _state = AIR_SENSOR_READY;
    if (!_forceFSM()) _state = AIR_SENSOR_UNINITIALIZED;
}

bool AirSensor::getSensorData(int *temperature, int *humidity, bool forceCheck = false) {
    if (_state == AIR_SENSOR_UNINITIALIZED) return false;
    if (forceCheck) _forceFSM();
    else _processFSM();
    *temperature = _getTemperature();
    *humidity = _getHumidity();
}

int AirSensor::_getTemperature() const {
    int res;

    res = data[2];
    if (data[3] & 0x80) res = -1 - res;
    return res * 10 + (data[3] & 0x0F);
}


int AirSensor::_getHumidity() const {
    return data[0] * 10 + data[1];
}

bool AirSensor::_forceFSM() {
    Timer timer;
    
    if (_state == AIR_SENSOR_IDLE) _state = AIR_SENSOR_READY;

    timer.setPeriod(AIR_SENSOR_TIME_TO_SYNC_READING);
    while (!timer.isReady()) {
        if (_processFSM()) return true;
    }
    return false;
}

/*
 Конечный автомат для переключения статусов и вызова соответствующих фукнций
 */
bool AirSensor::_processFSM() {

    switch (_state) {
        
        case AIR_SENSOR_READY:           // сенсор свободен, можно начать измерение
            _timer.setPeriod(AIR_SENSOR_PULLUP_TIME);
            _state = AIR_SENSOR_BEGIN_READING;
            break;

        case AIR_SENSOR_BEGIN_READING:  // отправить сигнал начала чтения
            if (_timer.isReady()) {
                _beginReading();
                _timer.setPeriod(AIR_SENSOR_START_SIGNAL_DURATION);
                _state = AIR_SENSOR_READ_DATA;
            }
            break;

        case AIR_SENSOR_READ_DATA:      // осуществить чтение
            if (_timer.isReady()) {

                _state = AIR_SENSOR_IDLE;
                _timer.setPeriod(_checkPeriod);
                if (_checkSensor()) {
                    return true;
                }
            }
            break;

        case AIR_SENSOR_IDLE:       // освободить сенсор и дать ему "отдохнуть"
            if (_timer.isReady()) {
                _state = AIR_SENSOR_READY;
            }
            break;

        default:
            break;
    }

    return false;
}

void AirSensor::_beginReading() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

/* Считываем данные с датчика */
bool AirSensor::_checkSensor() {
    Timer timer;

    pinMode(_pin, INPUT_PULLUP);        // подтягиваем ножку к высокому уровню и ожидаем сигнал датчика

    timer.setPeriod(AIR_SENSOR_TIME_TO_BEGIN_READING);  // устанавливаем время ожидания
    while (digitalRead(_pin) == HIGH) {
        if (timer.isReady()) return false;
    }

    /* Данный участок кода чувствителен ко времени выполнения. Поэтому необходимо запретить
     прерывания на период его выполнения. Класс ManageInterrupts при вызове конструктора
     (определение объекта класса) запрещает прерывания, а при вызове деструктора (завершение
     участка кода либо return) разрешает их. */
    {
        volatile ManageInterrupts manageInterrupts;

        if (_waitSignal(LOW) == 0) {
            return false;
        }
        if (_waitSignal(HIGH) == 0) {
            return false;
        }

        /* Далее читаем из датчика 40 бит данных. В соответствии с документацией, каждый
           бит передаётся так:
                1. 50 мкс низкий уровень
                2. 26-28 мкс (бит 0) либо 70 мкс (бит 1) высокий уровень
           Мы измеряем продолжительность сигнала высокого уровня, следующего за низким
           уровнем. Если его продолжительность >= 46 мкс (фактически с учётом вызовов и 
           переходов ~50 мкс)  записываем "1", иначе "0"
        */

        for (uint8_t i = 0; i < 40; i++) {
            data[i / 8] <<= 1;
            uint8_t low = _waitSignal(LOW);
            if (_waitSignal(HIGH) > low) data[i / 8] |= 1;
        }
    }

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return true;
    } else {
        return false;
    }
}

/*
 Ожидает пока вход находится в указанном состоянии (HIGH / LOW)
 Возвращает количество микросекунд ожидания или ошибку (0), если
 превышено указанное в AIR_SENSOR_WAITING_SIGNAL_TIME время ожидания

 ВАЖНО: поскольку в момент вызова этого метода прерывания отключены,
 micros() не считает переполнения счётчика и изменяется только младший байт.
 То есть время ожидания технически не может превышать 252 микросекунд, а
 практически - ~200. Иначе будет переполнение и условие 
 micros() - startTime >= AIR_SENSOR_WAITING_SIGNAL_TIME
 может не сработать
 */
uint16_t AirSensor::_waitSignal(bool level) const {
    uint16_t count = 0; // Начало ожидания
    while (digitalRead(_pin) == level) {
        if (count++ > _maxWaitingSignalCycles) {
            return 0; // Вернуть 0 в случае превышения времени ожидания
        }
    }
    return count;
}

/* Класс для запрета прерываний на критичных ко времени выполнения участках кода
 В конструкторе осуществляется запрет прерываний, в деструкторе разрешение прерываний
 Таким образом при определении объекта класса ManageInterrupts в критичном ко времени
 выполнения участке кода вызывается конструктор и прерывания запрещаются, при завершении
 такого участка кода вызывается деструктор и прерывания разрешаются
 ВАЖНО: участок кода должен быть как минимум выделен {}, или оформлен в виде отдельной функции*/
ManageInterrupts::ManageInterrupts() {
    noInterrupts();
}

ManageInterrupts::~ManageInterrupts() {
    interrupts();
}
