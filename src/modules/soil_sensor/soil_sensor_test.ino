/*
 * Модуль датчика влажности почвы
 * 
 * Тестовый файл для проверки работы модуля SoilSensor на железе.
 * 
 * Использование:
 * 1. Подключите датчик влажности почвы:
 *    - Аналоговый выход датчика -> A0 (или другой аналоговый пин)
 *    - Питание датчика -> цифровой пин 2 (или другой цифровой пин)
 *    - GND -> GND
 * 2. Загрузите этот скетч на Arduino Nano
 * 3. Откройте Serial Monitor (9600 baud)
 * 4. Следуйте инструкциям в Serial Monitor
 * 
 * Примечание: Для тестирования калибровки можно использовать потенциометр
 * вместо датчика, подключив его к аналоговому входу.
 */

#include <Arduino.h>
#include "soil_sensor.h"

// Пины для подключения датчика (измените при необходимости)
#define PIN_SOIL_INPUT  A0
#define PIN_SOIL_VCC    2

// Создаем датчик с периодом опроса 10 секунд (для тестирования)
SoilSensor sensor(PIN_SOIL_INPUT, PIN_SOIL_VCC, 10000UL);

void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Soil Sensor Module Test ==="));
    Serial.println();
    
    sensor.begin();
    Serial.println(F("Sensor initialized"));
    Serial.println();
}

void testInitialization() {
    Serial.println(F("Test 1: Sensor initialization"));
    Serial.println(F("  Checking sensor state after begin()..."));
    
    // После begin() датчик должен начать чтение
    delay(100); // Даем время на переход в состояние READ_DATA
    
    int rawValue = sensor.getRawSensorData();
    Serial.print(F("  Raw sensor value: "));
    Serial.println(rawValue);
    
    uint8_t humidity = sensor.getSensorData();
    Serial.print(F("  Calculated humidity: "));
    Serial.print(humidity);
    Serial.println(F("%"));
    
    Serial.println();
    delay(2000);
}

void testPeriodicReading() {
    Serial.println(F("Test 2: Periodic reading"));
    Serial.println(F("  Reading sensor immediately..."));
    
    int raw1 = sensor.getRawSensorData();
    uint8_t hum1 = sensor.getSensorData();
    Serial.print(F("  First reading - Raw: "));
    Serial.print(raw1);
    Serial.print(F(", Humidity: "));
    Serial.print(hum1);
    Serial.println(F("%"));
    
    Serial.println(F("  Waiting 5 seconds (period is 10 seconds)..."));
    delay(5000);
    
    int raw2 = sensor.getRawSensorData();
    uint8_t hum2 = sensor.getSensorData();
    Serial.print(F("  Second reading - Raw: "));
    Serial.print(raw2);
    Serial.print(F(", Humidity: "));
    Serial.print(hum2);
    Serial.println(F("%"));
    Serial.println(F("  (Values should be the same - period not expired)"));
    
    Serial.println(F("  Waiting additional 6 seconds (total 11 seconds)..."));
    delay(6000);
    
    int raw3 = sensor.getRawSensorData();
    uint8_t hum3 = sensor.getSensorData();
    Serial.print(F("  Third reading - Raw: "));
    Serial.print(raw3);
    Serial.print(F(", Humidity: "));
    Serial.print(hum3);
    Serial.println(F("%"));
    Serial.println(F("  (Values should be updated - period expired)"));
    
    Serial.println();
    delay(2000);
}

void testForceCheck() {
    Serial.println(F("Test 3: Force check"));
    
    int raw1 = sensor.getRawSensorData();
    Serial.print(F("  Initial reading - Raw: "));
    Serial.println(raw1);
    
    Serial.println(F("  Forcing immediate check..."));
    delay(100); // Даем время на чтение
    int raw2 = sensor.getRawSensorData(true);
    Serial.print(F("  After force check - Raw: "));
    Serial.println(raw2);
    Serial.println(F("  (Should be updated immediately)"));
    
    Serial.println();
    delay(2000);
}

void testCalibrationResistive() {
    Serial.println(F("Test 4: Calibration - Resistive sensor"));
    Serial.println(F("  Setting calibration: min=180, max=500 (resistive)"));
    
    // Для резистивного датчика: при повышении влажности показания растут
    // min < max: показания 180 = сухо (0%), показания 500 = влажно (100%)
    sensor.calibrate(180, 500);
    
    Serial.println(F("  Reading sensor with new calibration..."));
    delay(100);
    
    int raw = sensor.getRawSensorData(true);
    uint8_t humidity = sensor.getSensorData();
    
    Serial.print(F("  Raw value: "));
    Serial.println(raw);
    Serial.print(F("  Calculated humidity: "));
    Serial.print(humidity);
    Serial.println(F("%"));
    
    Serial.println();
    delay(2000);
}

void testCalibrationCapacitive() {
    Serial.println(F("Test 5: Calibration - Capacitive sensor"));
    Serial.println(F("  Setting calibration: min=500, max=180 (capacitive)"));
    
    // Для емкостного датчика: при повышении влажности показания падают
    // min > max: показания 500 = 0% (сухо), показания 180 = 100% (влажно)
    sensor.calibrate(500, 180);
    
    Serial.println(F("  Reading sensor with new calibration..."));
    delay(100);
    
    int raw = sensor.getRawSensorData(true);
    uint8_t humidity = sensor.getSensorData();
    
    Serial.print(F("  Raw value: "));
    Serial.println(raw);
    Serial.print(F("  Calculated humidity: "));
    Serial.print(humidity);
    Serial.println(F("%"));
    
    Serial.println();
    delay(2000);
}

void testCalibrationBoundaries() {
    Serial.println(F("Test 6: Calibration boundaries"));
    
    Serial.println(F("  Testing with values below min..."));
    sensor.calibrate(300, 700);
    // Симулируем значение ниже min (если возможно)
    Serial.println(F("  (If sensor reads < 300, humidity should be 0%)"));
    
    Serial.println(F("  Testing with values above max..."));
    // Симулируем значение выше max (если возможно)
    Serial.println(F("  (If sensor reads > 700, humidity should be 100%)"));
    
    Serial.println(F("  Testing with min == max (should not change calibration)..."));
    int oldMin = 300;
    int oldMax = 700;
    sensor.calibrate(oldMin, oldMax);
    sensor.calibrate(500, 500); // min == max
    // Калибровка не должна измениться
    Serial.println(F("  (Calibration should remain unchanged)"));
    
    Serial.println();
    delay(2000);
}

void testPowerManagement() {
    Serial.println(F("Test 7: Power management"));
    Serial.println(F("  Checking that power is turned on only during reading..."));
    
    // После чтения питание должно быть выключено
    sensor.getRawSensorData(true);
    delay(200); // Даем время на завершение чтения
    
    // Проверяем состояние пина питания (должно быть LOW)
    // Примечание: В реальном тесте можно использовать мультиметр или осциллограф
    Serial.println(F("  Power pin should be LOW after reading"));
    Serial.println(F("  (Check with multimeter or oscilloscope)"));
    
    Serial.println(F("  Forcing new reading..."));
    delay(100);
    sensor.getRawSensorData(true);
    Serial.println(F("  Power pin should be HIGH during reading, then LOW"));
    
    Serial.println();
    delay(2000);
}

void testFSMTransitions() {
    Serial.println(F("Test 8: FSM state transitions"));
    Serial.println(F("  Testing state machine behavior..."));
    
    // Создаем новый датчик для чистого теста
    SoilSensor testSensor(PIN_SOIL_INPUT, PIN_SOIL_VCC, 5000UL);
    
    Serial.println(F("  Before begin(): sensor should be UNINITIALIZED"));
    // getRawSensorData() не должен работать до begin()
    
    Serial.println(F("  Calling begin()..."));
    testSensor.begin();
    Serial.println(F("  After begin(): sensor should transition to BEGIN_READING -> READ_DATA"));
    
    delay(100);
    Serial.println(F("  Reading sensor data..."));
    int raw = testSensor.getRawSensorData();
    Serial.print(F("  Raw value: "));
    Serial.println(raw);
    Serial.println(F("  Sensor should be in IDLE state now"));
    
    Serial.println(F("  Waiting for period expiration (5 seconds)..."));
    delay(5100);
    Serial.println(F("  After period expiration: sensor should transition to BEGIN_READING"));
    
    Serial.println();
    delay(2000);
}

void loop() {
    static uint8_t testNum = 0;
    
    switch (testNum) {
        case 0:
            testInitialization();
            break;
        case 1:
            testPeriodicReading();
            break;
        case 2:
            testForceCheck();
            break;
        case 3:
            testCalibrationResistive();
            break;
        case 4:
            testCalibrationCapacitive();
            break;
        case 5:
            testCalibrationBoundaries();
            break;
        case 6:
            testPowerManagement();
            break;
        case 7:
            testFSMTransitions();
            break;
        default:
            Serial.println(F("=== All tests completed ==="));
            Serial.println(F("Press RESET to run tests again"));
            delay(10000);
            return;
    }
    
    testNum++;
}

