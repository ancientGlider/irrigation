/*
 * Air Sensor Module Test
 *
 * Hardware setup:
 *  - Connect DHT11 or DHT22 data pin to PIN_AIR_SENSOR (default D2)
 *  - Use a 10k pull-up resistor between VCC and data pin if the module does not include one
 *  - Power the sensor from 5V (DHT11) or 3.3/5V (DHT22)
 *
 * Usage:
 * 1. Upload this sketch to Arduino Nano
 * 2. Open Serial Monitor at 9600 baud
 * 3. Follow the instructions printed in Serial Monitor
 */

#include <Arduino.h>
#include "air_sensor.h"

#define PIN_AIR_SENSOR 2

// Use shorter period for testing (every 3 seconds)
AirSensor airSensor(PIN_AIR_SENSOR, 3000UL);

static void printReading(const char* prefix, bool updated, int temperature, int humidity) {
    Serial.print(prefix);
    Serial.print(updated ? F(" new data => ") : F(" cached data => "));
    Serial.print(F("Temperature: "));
    Serial.print(temperature / 10.0f, 1);
    Serial.print(F(" C, Humidity: "));
    Serial.print(humidity / 10.0f, 1);
    Serial.println(F(" %"));
}

void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Air Sensor Module Test ==="));
    Serial.println();

    Serial.println(F("Initializing sensor..."));
    airSensor.begin();
    Serial.println(F("Initialization complete."));
    Serial.println();
}

void testInitialization() {
    Serial.println(F("Test 1: Initialization"));
    int temperature = 0;
    int humidity = 0;
    bool updated = airSensor.getSensorData(&temperature, &humidity, true);
    printReading("  Immediate reading:", updated, temperature, humidity);
    Serial.println(F("  (Expect valid values if sensor is connected)"));
    Serial.println();
    delay(2000);
}

void testPeriodicReadings() {
    Serial.println(F("Test 2: Periodic readings"));
    int temperature = 0;
    int humidity = 0;

    Serial.println(F("  Reading right now:"));
    bool updated1 = airSensor.getSensorData(&temperature, &humidity);
    printReading("    Result:", updated1, temperature, humidity);

    Serial.println(F("  Waiting 1 second (should reuse cached data)..."));
    delay(1000);
    bool updated2 = airSensor.getSensorData(&temperature, &humidity);
    printReading("    After 1 s:", updated2, temperature, humidity);

    Serial.println(F("  Waiting another 3 seconds (new period)..."));
    delay(3000);
    bool updated3 = airSensor.getSensorData(&temperature, &humidity);
    printReading("    After 4 s:", updated3, temperature, humidity);

    Serial.println();
    delay(2000);
}

void testForceCheck() {
    Serial.println(F("Test 3: Force check"));
    int temperature = 0;
    int humidity = 0;

    Serial.println(F("  Forcing immediate reading..."));
    bool updated = airSensor.getSensorData(&temperature, &humidity, true);
    printReading("    Force result:", updated, temperature, humidity);

    Serial.println(F("  Immediately requesting data without force (should be cached)..."));
    updated = airSensor.getSensorData(&temperature, &humidity, false);
    printReading("    Cached result:", updated, temperature, humidity);

    Serial.println();
    delay(2000);
}

void testChecksumFailure() {
    Serial.println(F("Test 4: Checksum integrity (manual observation)"));
    Serial.println(F("  To test checksum handling, disconnect the data pin momentarily"));
    Serial.println(F("  and observe that new data is not reported (updated=false)."));
    int temperature = 0;
    int humidity = 0;
    bool updated = airSensor.getSensorData(&temperature, &humidity, true);
    printReading("    Reading:", updated, temperature, humidity);
    Serial.println(F("  (Reconnect sensor after the test)"));
    Serial.println();
    delay(2000);
}

void loop() {
    static uint8_t testIndex = 0;

    switch (testIndex) {
        case 0:
            testInitialization();
            break;
        case 1:
            testPeriodicReadings();
            break;
        case 2:
            testForceCheck();
            break;
        case 3:
            testChecksumFailure();
            break;
        default:
            Serial.println(F("=== All tests completed ==="));
            Serial.println(F("Reset the board to run again."));
            delay(10000);
            return;
    }

    ++testIndex;
}
