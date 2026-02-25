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
    _state = AIR_SENSOR_READY;

    pinMode(_pin, INPUT_PULLUP);
    _initialized = _forceFSM();
    
    if (_initialized) Serial.println("true");
    else Serial.println("false");
}

bool AirSensor::getSensorData(int *temperature, int *humidity) {
    if (!_initialized) return false;
    _processFSM();
    *temperature = _getTemperature();
    *humidity = _getHumidity();
}

bool AirSensor::getSyncSensorData(int *temperature, int *humidity) {
    if (!_initialized) return false;
    if (_forceFSM()) {
        *temperature = _getTemperature();
        *humidity = _getHumidity();
        return true;
    }
    return false;
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
//            Serial.println("In state 3");
            if (_timer.isReady()) {

                _state = AIR_SENSOR_IDLE;
                _timer.setPeriod(_checkPeriod);
                if (_checkSensor()) {
                    Serial.println("Read data ok");
                    return true;
                }
            }
            break;

        case AIR_SENSOR_IDLE:       // освободить сенсор и дать ему "отдохнуть"
            Serial.print("In state 4   ");
            Serial.println(millis());

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
    uint16_t signalLength;
    uint16_t SL[80];
    Timer timer;

    pinMode(_pin, INPUT_PULLUP);        // подтягиваем ножку к высокому уровню и ожидаем сигнал датчика

    timer.setPeriod(AIR_SENSOR_TIME_TO_BEGIN_READING);  // устанавливаем время ожидания
    while (digitalRead(_pin) == HIGH) {
        if (timer.isReady()) return false;
    }

Serial.println("Got response, begin read");

    data[0] = data[1] = data[2] = data[3] = data[4] = 0;
    
    /* Данный участок кода чувствителен ко времени выполнения. Поэтому необходимо запретить
     прерывания на период его выполнения. Класс ManageInterrupts при вызове конструктора
     (определение объекта класса) запрещает прерывания, а при вызове деструктора (завершение
     участка кода либо return) разрешает их. */
    {
        volatile ManageInterrupts manageInterrupts;

        if (_waitSignal(LOW) == 0) {
            Serial.println("Cannot receive 1st LOW");
            return false;
        }
        if (_waitSignal(HIGH) == 0) {
            Serial.println("Cannot receive 1st HIGH");
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

        for (int i = 0; i < 80; i += 2) {
            signalLength = _waitSignal(LOW);
            SL[i / 2] = signalLength;
            if (signalLength == 0) {
                Serial.print("FATAL: Prefix error, i = ");
                Serial.println(i);
                for (int _=0; _<5; _++) Serial.println(data[i]);
                Serial.println("Signals :");
                for (int _=0; _<=i; _+=2) {
                    Serial.print("Bit ");
                    Serial.print(_/2);
                    Serial.print(": prefix=");
                    Serial.print(SL[_/2]);
                    Serial.print("; data=");
                    Serial.println(SL[_/2+1]);
                }

                return false;
            }
            signalLength = _waitSignal(HIGH);
            SL[i / 2 + 1] = signalLength;
            if (signalLength == 0) {
                Serial.print("FATAL: Info bit error, i = ");
                Serial.println(i);
                for (int _=0; _<5; _++) Serial.println(data[i]);
                return false;
            }
            data[i / 16] <<= 1;
            if (signalLength >= 46) data[i / 16] |= 1; 
        }
        /* Конец участка кода, чувствительного ко времени */
    }

    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return true;
    } else {
        Serial.println("Incorrect CRC");
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
    uint32_t startTime = micros(); // Начало ожидания
    while (digitalRead(_pin) != level) {
        if (micros() - startTime >= AIR_SENSOR_WAITING_SIGNAL_TIME) {
            return 0; // Вернуть 0 в случае превышения времени ожидания
        }
    }
    return static_cast<uint16_t>(micros() - startTime);
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
