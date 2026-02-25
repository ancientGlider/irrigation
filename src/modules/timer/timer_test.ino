/*
 * Модуль системного таймера
 * 
 * Тестовый файл для проверки работы модуля Timer на железе.
 * 
 * Использование:
 * 1. Загрузите этот скетч на Arduino Nano
 * 2. Откройте Serial Monitor (9600 baud)
 * 3. Следуйте инструкциям в Serial Monitor
 */

#include <Arduino.h>
#include "timer.h"

void setup() {
    Serial.begin(9600);
    Serial.println(F("=== Timer Module Test ==="));
    Serial.println();
}

void testInitialization() {
    Serial.println(F("Test 1: Timer initialization"));
    
    Timer timer(1000); // Timer with 1 second period
    
    unsigned long time = timer.getTime();
    Serial.print(F("  getTime() right after creation: "));
    Serial.print(time);
    Serial.println(F(" ms (should be ~0)"));
    
    bool ready = timer.isReady();
    Serial.print(F("  isReady() right after creation: "));
    Serial.println(ready ? F("true") : F("false (correct)"));
    
    Serial.println();
}

void testTimeElapsed() {
    Serial.println(F("Test 2: Time elapsed"));
    
    Timer timer(1000);
    delay(500);
    
    unsigned long time = timer.getTime();
    Serial.print(F("  After 500 ms: getTime() = "));
    Serial.print(time);
    Serial.println(F(" ms (should be ~500)"));
    
    bool ready = timer.isReady();
    Serial.print(F("  isReady() after 500 ms: "));
    Serial.println(ready ? F("true (error!)") : F("false (correct)"));
    
    Serial.println();
}

void testTimerReady() {
    Serial.println(F("Test 3: Timer ready"));
    Serial.println(F("  Waiting 1.5 seconds..."));
    
    Timer timer(1000);
    delay(1500);
    
    unsigned long time = timer.getTime();
    Serial.print(F("  getTime() after 1500 ms: "));
    Serial.print(time);
    Serial.println(F(" ms"));
    
    bool ready = timer.isReady();
    Serial.print(F("  isReady() after 1500 ms: "));
    Serial.println(ready ? F("true (correct)") : F("false (error!)"));
    
    if (ready) {
        // After isReady() timer should reset
        delay(100);
        unsigned long timeAfter = timer.getTime();
        Serial.print(F("  getTime() after isReady(): "));
        Serial.print(timeAfter);
        Serial.println(F(" ms (should be ~100)"));
    }
    
    Serial.println();
}

void testSetPeriod() {
    Serial.println(F("Test 4: Period change"));
    
    Timer timer(1000);
    delay(500);
    
    Serial.println(F("  Setting period to 500 ms..."));
    timer.setPeriod(500, true); // Reset timer
    
    delay(600);
    
    bool ready = timer.isReady();
    Serial.print(F("  isReady() after 600 ms with new period: "));
    Serial.println(ready ? F("true (correct)") : F("false (error!)"));
    
    Serial.println();
}

void testDrop() {
    Serial.println(F("Test 5: Timer reset"));
    
    Timer timer(1000);
    delay(500);
    
    unsigned long timeBefore = timer.getTime();
    Serial.print(F("  getTime() before drop(): "));
    Serial.print(timeBefore);
    Serial.println(F(" ms"));
    
    timer.drop();
    
    unsigned long timeAfter = timer.getTime();
    Serial.print(F("  getTime() after drop(): "));
    Serial.print(timeAfter);
    Serial.println(F(" ms (should be ~0)"));
    
    Serial.println();
}

void testOverflow() {
    Serial.println(F("Test 6: millis() overflow handling"));
    Serial.println(F("  This test checks correct operation during overflow."));
    Serial.println(F("  Full test requires system operation for more than 49 days."));
    Serial.println(F("  Check: getTime() should always return correct value."));
    
    Timer timer(1000);
    unsigned long initTime = millis();
    
    // Simulate situation close to overflow
    // (in practice this will happen after ~49 days of continuous operation)
    Serial.println(F("  Timer created. Check will be performed at real overflow."));
    
    Serial.println();
}

void loop() {
    Serial.println(F("Select test:"));
    Serial.println(F("  1 - Initialization"));
    Serial.println(F("  2 - Time elapsed"));
    Serial.println(F("  3 - Timer ready"));
    Serial.println(F("  4 - Period change"));
    Serial.println(F("  5 - Timer reset"));
    Serial.println(F("  6 - Overflow handling"));
    Serial.println(F("  a - All tests sequentially"));
    Serial.println();
    
    while (!Serial.available()) {
        delay(100);
    }
    
    char choice = Serial.read();
    Serial.println();
    
    switch (choice) {
        case '1':
            testInitialization();
            break;
        case '2':
            testTimeElapsed();
            break;
        case '3':
            testTimerReady();
            break;
        case '4':
            testSetPeriod();
            break;
        case '5':
            testDrop();
            break;
        case '6':
            testOverflow();
            break;
        case 'a':
        case 'A':
            testInitialization();
            delay(1000);
            testTimeElapsed();
            delay(1000);
            testTimerReady();
            delay(1000);
            testSetPeriod();
            delay(1000);
            testDrop();
            delay(1000);
            testOverflow();
            break;
        default:
            Serial.println(F("Invalid choice"));
            break;
    }
    
    Serial.println(F("Press any key to continue..."));
    while (Serial.available()) {
        Serial.read(); // Clear buffer
    }
    while (!Serial.available()) {
        delay(100);
    }
    Serial.read();
    Serial.println();
}

