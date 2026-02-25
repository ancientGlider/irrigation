/*
 * Модуль таймера RTC (TimerRTC)
 * 
 * Тестовый файл для проверки работы модуля TimerRTC на железе.
 * 
 * Использование:
 * 1. Подключите DS1302 к Arduino Nano:
 *    - ENA к пину 12
 *    - CLK к пину 4
 *    - DAT к пину 3
 * 2. Загрузите этот скетч на Arduino Nano
 * 3. Откройте Serial Monitor (9600 baud)
 * 4. Следуйте инструкциям в Serial Monitor
 */

#include <Arduino.h>
#include "timerRTC.h"

#define PIN_RTC_ENA 12
#define PIN_RTC_CLK 4
#define PIN_RTC_DAT 3

void setup() {
    Serial.begin(9600);
    Serial.println(F("=== TimerRTC Module Test ==="));
    Serial.println();
    
    Serial.println(F("Initializing RTC..."));
    TimerRTC::begin(PIN_RTC_ENA, PIN_RTC_CLK, PIN_RTC_DAT);
    Serial.println(F("RTC initialized"));
    Serial.println();
}

void testInitialization() {
    Serial.println(F("Test 1: RTC initialization"));
    
    TimerRTC rtc(10); // Timer with 10 second period
    
    unsigned long time = rtc.getTime();
    Serial.print(F("  getTime() right after creation: "));
    Serial.print(time);
    Serial.println(F(" sec (should be ~0)"));
    
    bool ready = rtc.isReady();
    Serial.print(F("  isReady() right after creation: "));
    Serial.println(ready ? F("true") : F("false (correct)"));
    
    Serial.println();
}

void testTimeElapsed() {
    Serial.println(F("Test 2: Time elapsed"));
    
    TimerRTC rtc(10);
    delay(5000);
    
    unsigned long time = rtc.getTime();
    Serial.print(F("  After 5 sec: getTime() = "));
    Serial.print(time);
    Serial.println(F(" sec (should be ~5)"));
    
    bool ready = rtc.isReady();
    Serial.print(F("  isReady() after 5 sec: "));
    Serial.println(ready ? F("true (error!)") : F("false (correct)"));
    
    Serial.println();
}

void testTimerReady() {
    Serial.println(F("Test 3: Timer ready"));
    Serial.println(F("  Waiting 12 seconds..."));
    
    TimerRTC rtc(10);
    delay(12000);
    
    unsigned long time = rtc.getTime();
    Serial.print(F("  getTime() after 12 sec: "));
    Serial.print(time);
    Serial.println(F(" sec"));
    
    bool ready = rtc.isReady();
    Serial.print(F("  isReady() after 12 sec: "));
    Serial.println(ready ? F("true (correct)") : F("false (error!)"));
    
    if (ready) {
        delay(1000);
        unsigned long timeAfter = rtc.getTime();
        Serial.print(F("  getTime() after isReady(): "));
        Serial.print(timeAfter);
        Serial.println(F(" sec (should be ~1)"));
    }
    
    Serial.println();
}

void testSetPeriod() {
    Serial.println(F("Test 4: Period change"));
    
    TimerRTC rtc(10);
    delay(5000);
    
    Serial.println(F("  Setting period to 5 sec..."));
    rtc.setPeriod(5, true); // Reset timer
    
    delay(6000);
    
    bool ready = rtc.isReady();
    Serial.print(F("  isReady() after 6 sec with new period: "));
    Serial.println(ready ? F("true (correct)") : F("false (error!)"));
    
    Serial.println();
}

void testDrop() {
    Serial.println(F("Test 5: Timer reset"));
    
    TimerRTC rtc(10);
    delay(5000);
    
    unsigned long timeBefore = rtc.getTime();
    Serial.print(F("  getTime() before drop(): "));
    Serial.print(timeBefore);
    Serial.println(F(" sec"));
    
    rtc.drop();
    
    unsigned long timeAfter = rtc.getTime();
    Serial.print(F("  getTime() after drop(): "));
    Serial.print(timeAfter);
    Serial.println(F(" sec (should be ~0)"));
    
    Serial.println();
}

void testSetTime() {
    Serial.println(F("Test 6: Set time"));
    
    TimerRTC rtc(10);
    
    unsigned long timeBefore = rtc.getTime();
    Serial.print(F("  Current time: "));
    Serial.print(timeBefore);
    Serial.println(F(" sec"));
    
    Serial.println(F("  Setting time to 1000 sec..."));
    rtc.setTime(1000, true);
    
    unsigned long timeAfter = rtc.getTime();
    Serial.print(F("  getTime() after setTime(): "));
    Serial.print(timeAfter);
    Serial.println(F(" sec (should be ~0, timer reset)"));
    
    delay(2000);
    unsigned long timeAfter2Sec = rtc.getTime();
    Serial.print(F("  getTime() after 2 sec: "));
    Serial.print(timeAfter2Sec);
    Serial.println(F(" sec (should be ~2)"));
    
    Serial.println();
}

void testInitTime() {
    Serial.println(F("Test 7: InitTime save/restore"));
    
    TimerRTC rtc(10);
    delay(3000);
    
    unsigned long initTime = rtc.getInitTime();
    Serial.print(F("  Saved initTime: "));
    Serial.println(initTime);
    
    TimerRTC rtc2(10);
    rtc2.setInitTime(initTime);
    
    unsigned long time = rtc2.getTime();
    Serial.print(F("  getTime() after restore: "));
    Serial.print(time);
    Serial.println(F(" sec (should be ~3)"));
    
    Serial.println();
}

void testRTCSync() {
    Serial.println(F("Test 8: RTC synchronization"));
    Serial.println(F("  Waiting for RTC sync (up to 10 minutes)..."));
    
    TimerRTC rtc(10);
    unsigned long time1 = rtc.getTime();
    
    // Ждем синхронизации (период синхронизации DEFAULT_RTC_POLLING_PERIOD = 10 минут)
    // Для теста можно уменьшить период синхронизации или просто проверить работу
    delay(1000);
    
    unsigned long time2 = rtc.getTime();
    Serial.print(F("  Time before sync: "));
    Serial.print(time1);
    Serial.println(F(" sec"));
    Serial.print(F("  Time after 1 sec: "));
    Serial.print(time2);
    Serial.println(F(" sec"));
    Serial.println(F("  Note: Full sync test requires waiting 10 minutes"));
    
    Serial.println();
}

void loop() {
    Serial.println(F("Select test:"));
    Serial.println(F("  1 - Initialization"));
    Serial.println(F("  2 - Time elapsed"));
    Serial.println(F("  3 - Timer ready"));
    Serial.println(F("  4 - Period change"));
    Serial.println(F("  5 - Timer reset"));
    Serial.println(F("  6 - Set time"));
    Serial.println(F("  7 - InitTime save/restore"));
    Serial.println(F("  8 - RTC synchronization"));
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
            testSetTime();
            break;
        case '7':
            testInitTime();
            break;
        case '8':
            testRTCSync();
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
            testSetTime();
            delay(1000);
            testInitTime();
            delay(1000);
            testRTCSync();
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

