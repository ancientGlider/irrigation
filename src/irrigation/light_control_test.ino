/*
 * Light Control Module Test
 *
 * Требования:
 *  - Подключенный DS1302 (TimerRTC)
 *  - Модуль growing_cycle должен быть инициализирован (используем те же контакты, что и в growing_cycle_test.ino)
 *
 * Команды (Serial 9600 baud):
 *   d - показать текущий статус света
 *   s - установить день (оставляя текущий час)
 *   h - установить час (оставляя текущий день)
 *   t - установить произвольное число секунд с начала цикла
 */

#include <Arduino.h>
#include "growing_cycle.h"
#include "light_control.h"

static constexpr uint8_t RTC_PIN_ENA = 12;
static constexpr uint8_t RTC_PIN_CLK = 4;
static constexpr uint8_t RTC_PIN_DAT = 3;
static constexpr uint8_t TEST_LIGHT_PIN = A3;

TimerRTC rtc;

void printLightState();

void setCyclePosition(uint16_t day, uint8_t hour) {
    uint32_t seconds = (static_cast<uint32_t>(day - 1U) * 86400UL) +
                       (static_cast<uint32_t>(hour % 24U) * 3600UL);
    rtc.setTime(seconds, false);
    Settings::Manager::set(Settings::Key::RtcInitTimeSec, rtc.getInitTime());
}

const __FlashStringHelper* lightStateName(Irrigation::LightState state) {
    switch (state) {
        case Irrigation::LightState::Day:   return F("Day");
        case Irrigation::LightState::Night: return F("Night");
        case Irrigation::LightState::Off:   return F("Off");
    }
    return F("Unknown");
}

void expectLight(uint16_t day,
                 uint8_t hour,
                 Irrigation::LightState expected,
                 const __FlashStringHelper* rationale) {
    Serial.println(rationale);
    Serial.print(F("    Expectation: "));
    Serial.println(lightStateName(expected));
    Serial.print(F("    Position: day="));
    Serial.print(day);
    Serial.print(F(" hour="));
    Serial.println(hour);

    setCyclePosition(day, hour);
    Irrigation::LightControl::update();
    Irrigation::LightState actual = Irrigation::LightControl::getLightState();

    Serial.print(F("    Actual:      "));
    Serial.println(lightStateName(actual));
    Serial.println(actual == expected
        ? F("    Result matches expectation.")
        : F("    RESULT DOES NOT MATCH EXPECTATION!"));
    Serial.println();
}

void printSummary(const __FlashStringHelper* context) {
    Serial.println();
    Serial.print(F("=== Light State Snapshot after "));
    Serial.print(context);
    Serial.println(F(" ==="));
    printLightState();
}

void printMenu() {
    Serial.println();
    Serial.println(F("Commands:"));
    Serial.println(F("  d - display current state"));
    Serial.println(F("  s - set current day (hour preserved)"));
    Serial.println(F("  h - set current hour (day preserved)"));
    Serial.println(F("  t - set elapsed seconds since cycle start"));
    Serial.println();
}

void runSmokeTests() {
    Serial.println(F("--- Running light control smoke tests ---"));

    const uint8_t germinationDays = Settings::Manager::get(Settings::Key::GerminationLengthDays);
    const uint8_t springHours = Settings::Manager::get(Settings::Key::SpringDayHours);
    const uint8_t summerHours = Settings::Manager::get(Settings::Key::SummerDayHours);
    const uint8_t autumnHours = Settings::Manager::get(Settings::Key::AutumnDayHours);
    const uint16_t springLength = Settings::Manager::get(Settings::Key::SpringLengthDays);
    const uint16_t summerLength = Settings::Manager::get(Settings::Key::SummerLengthDays);

    // Проращивание
    expectLight(1, 0, Irrigation::LightState::Day,
                F("[EXPECT] Day 1 hour 0 should be Day (first-hour illumination)"));
    expectLight(1, 1, Irrigation::LightState::Night,
                F("[EXPECT] Day 1 hour >=1 should be Night"));
    if (germinationDays >= 2U) {
        expectLight(2, 0, Irrigation::LightState::Night,
                    F("[EXPECT] Days 2..germinationDays are Night"));
    }

    // Весна
    const uint16_t firstSpringDay = germinationDays + 1U;
    const uint8_t springInsideHour = (springHours == 0U) ? 0U : static_cast<uint8_t>(springHours - 1U);
    expectLight(firstSpringDay, 0, Irrigation::LightState::Day,
                F("[EXPECT] Spring hour 0 should be Day"));
    expectLight(firstSpringDay, springInsideHour, Irrigation::LightState::Day,
                F("[EXPECT] Spring hour within SpringDayHours range should be Day"));
    expectLight(firstSpringDay, static_cast<uint8_t>(springHours), Irrigation::LightState::Night,
                F("[EXPECT] Spring hour >= SpringDayHours should be Night"));

    // Лето
    const uint16_t firstSummerDay = germinationDays + springLength + 1U;
    const uint8_t summerInsideHour = (summerHours == 0U) ? 0U : static_cast<uint8_t>(summerHours - 1U);
    expectLight(firstSummerDay, 0, Irrigation::LightState::Day,
                F("[EXPECT] Summer hour 0 should be Day"));
    expectLight(firstSummerDay, summerInsideHour, Irrigation::LightState::Day,
                F("[EXPECT] Summer hour within SummerDayHours range should be Day"));
    expectLight(firstSummerDay, static_cast<uint8_t>(summerHours), Irrigation::LightState::Night,
                F("[EXPECT] Summer hour >= SummerDayHours should be Night"));

    // Осень
    const uint16_t firstAutumnDay = germinationDays + springLength + summerLength + 1U;
    if (Settings::Manager::get(Settings::Key::AutumnLengthDays) > 0U) {
        const uint8_t autumnInsideHour = (autumnHours == 0U) ? 0U : static_cast<uint8_t>(autumnHours - 1U);
        expectLight(firstAutumnDay, 0, Irrigation::LightState::Day,
                    F("[EXPECT] Autumn hour 0 should be Day"));
        expectLight(firstAutumnDay, autumnInsideHour, Irrigation::LightState::Day,
                    F("[EXPECT] Autumn hour within AutumnDayHours range should be Day"));
        expectLight(firstAutumnDay, static_cast<uint8_t>(autumnHours), Irrigation::LightState::Night,
                    F("[EXPECT] Autumn hour >= AutumnDayHours should be Night"));
    }

    Serial.println(F("--- Smoke tests finished ---"));
    printSummary(F("smoke tests"));
    printMenu();
}

void printLightState() {
    Irrigation::LightControl::update();
    Serial.print(F("Day: ")); Serial.println(Irrigation::GrowingCycle::getCurrentDay());
    Serial.print(F("Hour: ")); Serial.println(Irrigation::GrowingCycle::getCurrentHour());
    Serial.print(F("Period: "));
    switch (Irrigation::GrowingCycle::getPeriod()) {
        case Irrigation::GrowingPeriod::Germination: Serial.println(F("Germination")); break;
        case Irrigation::GrowingPeriod::Spring:      Serial.println(F("Spring")); break;
        case Irrigation::GrowingPeriod::Summer:      Serial.println(F("Summer")); break;
        case Irrigation::GrowingPeriod::Autumn:      Serial.println(F("Autumn")); break;
        case Irrigation::GrowingPeriod::Completed:   Serial.println(F("Completed")); break;
    }
    Serial.print(F("Light state: "));
    switch (Irrigation::LightControl::getLightState()) {
        case Irrigation::LightState::Uninitialized: Serial.println(F("Uninitialized")); break;
        case Irrigation::LightState::Off:           Serial.println(F("Off")); break;
        case Irrigation::LightState::Day:           Serial.println(F("Day")); break;
        case Irrigation::LightState::Night:         Serial.println(F("Night")); break;
    }
    Serial.println();
}

void setup() {
    Serial.begin(9600);
    while (!Serial) { /* wait */ }

    Settings::Manager::get(Settings::Key::SpringLengthDays); // ensure cache is loaded

    // Задаём контролируемые настройки для тестов освещения
    Settings::Manager::set(Settings::Key::GerminationLengthDays, 3);
    Settings::Manager::set(Settings::Key::SpringLengthDays, 20);
    Settings::Manager::set(Settings::Key::SummerLengthDays, 20);
    Settings::Manager::set(Settings::Key::AutumnLengthDays, 10);
    Settings::Manager::set(Settings::Key::SpringDayHours, 19);
    Settings::Manager::set(Settings::Key::SummerDayHours, 16);
    Settings::Manager::set(Settings::Key::AutumnDayHours, 10);

    rtc.begin(RTC_PIN_ENA, RTC_PIN_CLK, RTC_PIN_DAT);
    Irrigation::GrowingCycle::begin(rtc);
    Irrigation::LightControl::begin(TEST_LIGHT_PIN);

    Serial.println(F("Light control test ready."));
    runSmokeTests();
}

void loop() {
    if (Serial.available()) {
        char cmd = static_cast<char>(Serial.read());
        bool handled = true;
        switch (cmd) {
            case 'd':
            case 'D':
                printSummary(F("manual inspection"));
                break;
            case 's':
            case 'S': {
                while (!Serial.available()) {}
                uint16_t day = static_cast<uint16_t>(Serial.parseInt());
                bool ok = Irrigation::GrowingCycle::setCurrentDay(day);
                Serial.print(F("setCurrentDay returned: "));
                Serial.println(ok ? F("true") : F("false"));
                printSummary(F("setCurrentDay"));
                break;
            }
            case 'h':
            case 'H': {
                while (!Serial.available()) {}
                uint8_t hour = static_cast<uint8_t>(Serial.parseInt());
                uint32_t elapsed = rtc.getTime();
                uint32_t dayBase = (Irrigation::GrowingCycle::getCurrentDay() - 1UL) * 86400UL;
                uint32_t newElapsed = dayBase + (hour % 24UL) * 3600UL;
                rtc.setTime(newElapsed, false);
                Settings::Manager::set(Settings::Key::RtcInitTimeSec, rtc.getInitTime());
                printSummary(F("setCurrentHour"));
                break;
            }
            case 't':
            case 'T': {
                while (!Serial.available()) {}
                uint32_t seconds = static_cast<uint32_t>(Serial.parseInt());
                rtc.setTime(seconds, false);
                Settings::Manager::set(Settings::Key::RtcInitTimeSec, rtc.getInitTime());
                printSummary(F("setElapsedSeconds"));
                break;
            }
            default:
                handled = false;
                break;
        }

        if (handled) {
            printMenu();
        }
    }
}
