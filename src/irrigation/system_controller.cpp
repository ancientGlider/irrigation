/*
 * System Controller implementation
 */

#include "system_controller.h"
#include "../settings/settings.h"

namespace Irrigation {

// Статические переменные
SystemState SystemController::_state;
bool SystemController::_initialized = false;
SoilSensor* SystemController::_soilSensor = nullptr;
AirSensor* SystemController::_airSensor = nullptr;

void SystemController::begin(
    uint8_t pinPump, bool pumpHighIsOn,
    uint8_t pinLight, bool lightHighIsOn,
    uint8_t pinSoilSensor, uint8_t pinSoilVCC,
    uint8_t pinAirSensor,
    TimerRTC& rtc
) {
    // 1. Инициализация настроек (загрузка из EEPROM)
    // Settings::Manager не требует явной инициализации (ленивая загрузка)

    // 2. Инициализация GrowingCycle
    GrowingCycle::begin(rtc);

    // 3. Инициализация LightControl
    LightControl::begin(pinLight, lightHighIsOn);

    // 4. Инициализация Watering
    Watering::begin(pinPump, pumpHighIsOn);

    // 5. Инициализация SoilSensor
    uint32_t checkPeriod = Settings::Manager::get(Settings::Key::SensorCheckPeriodSec) * 1000UL;
    _soilSensor = new SoilSensor(pinSoilSensor, pinSoilVCC, checkPeriod);
    _soilSensor->begin();
    
    // Загрузка калибровки из настроек
    uint16_t calDry = Settings::Manager::get(Settings::Key::SoilCalibrationDry);
    uint16_t calWet = Settings::Manager::get(Settings::Key::SoilCalibrationWet);
    _soilSensor->calibrate(calDry, calWet);

    // 6. Инициализация AirSensor
    _airSensor = new AirSensor(pinAirSensor, 2000UL); // 2 секунды по умолчанию
    _airSensor->begin();

    _initialized = true;
}

void SystemController::process() {
    if (!_initialized) {
        return;
    }

    // 1. Обновление датчиков (FSM внутри модулей)
    _updateSensors();

    // 2. Получение влажности почвы для полива
    uint8_t soilHumidity = _state.sensors.soilHumidity;

    // 3. Обработка полива (FSM)
    Watering::process(soilHumidity);

    // 4. Обновление освещения
    LightControl::update();

    // 5. Агрегация состояния
    _aggregateState();

    // 6. Обновление времени последнего обновления
    _state.lastUpdateMillis = millis();
}

void SystemController::_updateSensors() {
    if (!_soilSensor || !_airSensor) {
        return;
    }

    // Обновление датчика почвы (FSM внутри)
    _state.sensors.soilHumidity = _soilSensor->getSensorData(false);

    // Обновление датчика воздуха (FSM внутри)
    int temp = 0;
    int humidity = 0;
    _state.sensors.sensorsValid = _airSensor->getSensorData(&temp, &humidity, false);
    
    if (_state.sensors.sensorsValid) {
        _state.sensors.airTemperature = temp;
        _state.sensors.airHumidity = humidity;
    }
}

void SystemController::_aggregateState() {
    // Состояние полива
    _state.wateringState = Watering::getState();
    _state.pumpActive = Watering::isPumpOn();
    _state.wateringRemainingSeconds = Watering::getRemainingSeconds();

    // Состояние цикла выращивания
    _state.period = GrowingCycle::getPeriod();
    _state.currentDay = GrowingCycle::getCurrentDay();
    _state.totalDays = static_cast<uint16_t>(
        Settings::Manager::get(Settings::Key::CycleLengthDays)
    );
    _state.currentHour = GrowingCycle::getCurrentHour();
    _state.currentMinute = GrowingCycle::getCurrentMinute();

    // Состояние освещения
    _state.lightOn = LightControl::isLightOn();

    // Системный режим
    uint32_t modeValue = Settings::Manager::get(Settings::Key::SystemMode);
    _state.systemMode = static_cast<Settings::SystemMode>(modeValue);
}

const SystemState* SystemController::getSystemState() {
    return &_state;
}

void SystemController::setManualWatering() {
    if (!_initialized) return;
    Watering::setManualWatering();
}

void SystemController::setManualCleaning() {
    if (!_initialized) return;
    Watering::setManualCleaning();
}

void SystemController::setTraining() {
    if (!_initialized) return;
    Watering::setTraining();
}

void SystemController::stopWatering() {
    if (!_initialized) return;
    Watering::stop();
}

void SystemController::forceSensorCheck() {
    if (!_initialized || !_soilSensor || !_airSensor) return;
    
    // Принудительная проверка обоих датчиков
    _soilSensor->getSensorData(true);
    
    int temp = 0;
    int humidity = 0;
    _airSensor->getSensorData(&temp, &humidity, true);
    
    // Обновляем состояние сразу после принудительной проверки
    _updateSensors();
}

int16_t SystemController::getSoilRawValue(bool forceCheck) {
    if (!_initialized || !_soilSensor) {
        return 0;
    }
    return static_cast<int16_t>(_soilSensor->getRawSensorData(forceCheck));
}

} // namespace Irrigation
