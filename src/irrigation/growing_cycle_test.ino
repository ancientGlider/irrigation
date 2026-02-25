/*
 * Growing Cycle Module Test
 *
 * Требования:
 *  - Подключенный модуль DS1302 (TimerRTC)
 *  - Возможность управлять настройками через Settings::Manager
 *
 * Команды (Serial 9600 baud):
 *   d - показать текущий статус цикла
 *   s - установить день (ожидает число)
 *   h - установить час (ожидает число 0..23)
 *   t - установить прошедшие секунды (ожидает число)
 *   m - выбрать системный режим (0=growing,1=spring,2=summer,3=autumn)
 *   p - вывести длительности сезонов
 *   r - сбросить все настройки и таймер к значениям по умолчанию
 *   a - запустить встроенные автотесты
 *   ? - повторно показать меню
 */

#include <Arduino.h>
#include "growing_cycle.h"
#include "../settings/settings.h"

using Irrigation::GrowingCycle;
using Irrigation::GrowingPeriod;
using Settings::Key;

static constexpr uint8_t RTC_PIN_ENA = 12;
static constexpr uint8_t RTC_PIN_CLK = 4;
static constexpr uint8_t RTC_PIN_DAT = 3;

TimerRTC rtc;

namespace {

const __FlashStringHelper* periodName(GrowingPeriod period) {
    switch (period) {
        case GrowingPeriod::Germination: return F("Germination");
        case GrowingPeriod::Spring:      return F("Spring");
        case GrowingPeriod::Summer:      return F("Summer");
        case GrowingPeriod::Autumn:      return F("Autumn");
        case GrowingPeriod::Completed:   return F("Completed");
    }
    return F("Unknown");
}

const __FlashStringHelper* modeName(Settings::SystemMode mode) {
    switch (mode) {
        case Settings::SystemMode::Growing: return F("Growing");
        case Settings::SystemMode::Spring:  return F("Spring");
        case Settings::SystemMode::Summer:  return F("Summer");
        case Settings::SystemMode::Autumn:  return F("Autumn");
    }
    return F("Unknown");
}

void applyDefaultSettings() {
    const Settings::Data& defaults = Settings::DEFAULT_DATA;
    Settings::Manager::set(Key::SystemMode, defaults.systemMode);
    Settings::Manager::set(Key::GerminationLengthDays, defaults.germinationLengthDays);
    Settings::Manager::set(Key::SpringLengthDays,      defaults.springLengthDays);
    Settings::Manager::set(Key::SummerLengthDays,      defaults.summerLengthDays);
    Settings::Manager::set(Key::AutumnLengthDays,      defaults.autumnLengthDays);
    Settings::Manager::set(Key::SpringDayHours,        defaults.springDayHours);
    Settings::Manager::set(Key::SummerDayHours,        defaults.summerDayHours);
    Settings::Manager::set(Key::AutumnDayHours,        defaults.autumnDayHours);
    Settings::Manager::set(Key::WateringDurationSec,   defaults.wateringDurationSec);
    Settings::Manager::set(Key::WateringPauseSec,      defaults.wateringPauseSec);
    Settings::Manager::set(Key::WateringMaxAttempts,   defaults.wateringMaxAttempts);
    Settings::Manager::set(Key::SoilMoistureStartPercent, defaults.soilMoistureStartPercent);
    Settings::Manager::set(Key::SoilMoistureStopPercent,  defaults.soilMoistureStopPercent);
    Settings::Manager::set(Key::TrainingMoisturePercent,  defaults.trainingMoisturePercent);
    Settings::Manager::set(Key::CleaningDurationSec,   defaults.cleaningDurationSec);
    Settings::Manager::set(Key::CleaningCycles,        defaults.cleaningCycles);
    Settings::Manager::set(Key::CleaningPauseSec,      defaults.cleaningPauseSec);
    Settings::Manager::set(Key::SensorCheckPeriodSec,  defaults.sensorCheckPeriodSec);
    Settings::Manager::set(Key::SoilCalibrationDry,    defaults.soilCalibrationDry);
    Settings::Manager::set(Key::SoilCalibrationWet,    defaults.soilCalibrationWet);
    Settings::Manager::set(Key::RtcInitTimeSec,        defaults.rtcInitTimeSec);
    Settings::Manager::set(Key::Flags,                 defaults.flags);
}

void setCyclePosition(uint16_t day, uint8_t hour) {
    uint32_t seconds =
        (static_cast<uint32_t>(day - 1U) * 86400UL) +
        (static_cast<uint32_t>(hour % 24U) * 3600UL);
    rtc.setTime(seconds, false);
    Settings::Manager::set(Key::RtcInitTimeSec, rtc.getInitTime());
}

uint16_t cycleLength() {
    return static_cast<uint16_t>(Settings::Manager::get(Key::CycleLengthDays));
}

void printMenu() {
    Serial.println();
    Serial.println(F("Commands:"));
    Serial.println(F("  d - display current status"));
    Serial.println(F("  s - set current day"));
    Serial.println(F("  h - set current hour"));
    Serial.println(F("  t - set elapsed seconds"));
    Serial.println(F("  m - select system mode (0=growing,1=spring,2=summer,3=autumn)"));
    Serial.println(F("  p - print season lengths"));
    Serial.println(F("  r - reset to defaults"));
    Serial.println(F("  a - run automated tests"));
    Serial.println(F("  ? - show this menu again"));
    Serial.println();
}

void printStatus() {
    Serial.println(F("=== Growing Cycle ==="));
    Serial.print(F("Mode: "));
    Serial.println(modeName(static_cast<Settings::SystemMode>(Settings::Manager::get(Key::SystemMode))));
    Serial.print(F("Day: "));
    Serial.println(GrowingCycle::getCurrentDay());
    Serial.print(F("Hour: "));
    Serial.println(GrowingCycle::getCurrentHour());
    Serial.print(F("Period: "));
    Serial.println(periodName(GrowingCycle::getPeriod()));
    Serial.print(F("Completed: "));
    Serial.println(GrowingCycle::isCompleted() ? F("true") : F("false"));
    Serial.print(F("Elapsed seconds: "));
    Serial.println(rtc.getTime());
    Serial.println(F("======================"));
}

void printSeasons() {
    Serial.println(F("--- Season Lengths ---"));
    Serial.print(F("Cycle length: ")); Serial.println(Settings::Manager::get(Key::CycleLengthDays));
    Serial.print(F("Germination: "));  Serial.println(Settings::Manager::get(Key::GerminationLengthDays));
    Serial.print(F("Spring: "));       Serial.println(Settings::Manager::get(Key::SpringLengthDays));
    Serial.print(F("Summer: "));       Serial.println(Settings::Manager::get(Key::SummerLengthDays));
    Serial.print(F("Autumn: "));       Serial.println(Settings::Manager::get(Key::AutumnLengthDays));
    Serial.print(F("Spring day hours: ")); Serial.println(Settings::Manager::get(Key::SpringDayHours));
    Serial.print(F("Summer day hours: ")); Serial.println(Settings::Manager::get(Key::SummerDayHours));
    Serial.print(F("Autumn day hours: ")); Serial.println(Settings::Manager::get(Key::AutumnDayHours));
    Serial.println(F("-----------------------"));
}

void printSummary(const __FlashStringHelper* context) {
    Serial.println();
    Serial.print(F("=== Snapshot after "));
    Serial.print(context);
    Serial.println(F(" ==="));
    printStatus();
}

bool expect(const __FlashStringHelper* message, bool condition) {
    Serial.print(condition ? F("[PASS] ") : F("[FAIL] "));
    Serial.println(message);
    return condition;
}

bool expectPeriodAtDay(uint16_t day, GrowingPeriod expected, const __FlashStringHelper* message) {
    setCyclePosition(day, 0U);
    return expect(message, GrowingCycle::getPeriod() == expected);
}

bool expectSetDay(uint16_t day, bool expectedResult, const __FlashStringHelper* message) {
    bool result = GrowingCycle::setCurrentDay(day);
    bool ok = (result == expectedResult);
    if (result && expectedResult) {
        ok = ok && (GrowingCycle::getCurrentDay() == day);
    }
    return expect(message, ok);
}

bool expectSetHour(uint8_t hour, bool expectedResult, const __FlashStringHelper* message) {
    bool result = GrowingCycle::setCurrentHour(hour);
    bool ok = (result == expectedResult);
    if (result && expectedResult) {
        ok = ok && (GrowingCycle::getCurrentHour() == hour);
    }
    return expect(message, ok);
}

bool expectModeFixed(Settings::SystemMode mode,
                     GrowingPeriod expectedPeriod,
                     const __FlashStringHelper* message) {
    Settings::Manager::set(Key::SystemMode, static_cast<uint32_t>(mode));
    setCyclePosition(1U, 0U);
    bool ok = (GrowingCycle::getPeriod() == expectedPeriod);
    setCyclePosition(100U, 0U);
    ok = ok && (GrowingCycle::getPeriod() == expectedPeriod);
    ok = ok && !GrowingCycle::isCompleted();
    return expect(message, ok);
}

bool runAutomatedTests() {
    Serial.println(F("--- Running automated growing_cycle tests ---"));

    // Сбрасываем всё к дефолту.
    applyDefaultSettings();
    Settings::Manager::set(Key::SystemMode, static_cast<uint32_t>(Settings::SystemMode::Growing));
    setCyclePosition(1U, 0U);

    bool ok = true;
    ok &= expect(F("Day counter starts at 1"), GrowingCycle::getCurrentDay() == 1);
    ok &= expect(F("Hour counter starts at 0"), GrowingCycle::getCurrentHour() == 0);

    uint16_t length = cycleLength();

    ok &= expectSetDay(0, false, F("setCurrentDay rejects 0"));
    ok &= expectSetDay(length + 1U, false, F("setCurrentDay rejects day beyond cycle length"));
    ok &= expectSetDay(1, true, F("setCurrentDay accepts valid day within cycle"));

    ok &= expectPeriodAtDay(1, GrowingPeriod::Germination, F("Day 1 is Germination"));
    ok &= expectPeriodAtDay(Settings::Manager::get(Key::GerminationLengthDays),
                            GrowingPeriod::Germination,
                            F("Last day of germination still Germination"));

    uint16_t springStart = Settings::Manager::get(Key::GerminationLengthDays) + 1U;
    ok &= expectPeriodAtDay(springStart, GrowingPeriod::Spring, F("First day after germination is Spring"));

    uint16_t summerStart = Settings::Manager::get(Key::GerminationLengthDays) +
                           Settings::Manager::get(Key::SpringLengthDays) + 1U;
    ok &= expectPeriodAtDay(summerStart, GrowingPeriod::Summer, F("First day after spring is Summer"));

    uint16_t autumnStart = Settings::Manager::get(Key::GerminationLengthDays) +
                           Settings::Manager::get(Key::SpringLengthDays) +
                           Settings::Manager::get(Key::SummerLengthDays) + 1U;
    uint8_t autumnLength = Settings::Manager::get(Key::AutumnLengthDays);
    if (autumnLength > 0U) {
        ok &= expectPeriodAtDay(autumnStart, GrowingPeriod::Autumn, F("First day after summer is Autumn"));
    } else {
        ok &= expectPeriodAtDay(autumnStart, GrowingPeriod::Completed, F("No autumn: first day after summer is Completed"));
    }

    setCyclePosition(length, 23U);
    ok &= expect(F("Cycle not completed on last valid day"),
                 GrowingCycle::getPeriod() != GrowingPeriod::Completed);

    setCyclePosition(length + 1U, 0U);
    ok &= expect(F("Cycle completed after last day"),
                 GrowingCycle::getPeriod() == GrowingPeriod::Completed);
    ok &= expect(F("isCompleted returns true after cycle completion"),
                 GrowingCycle::isCompleted());

    ok &= expectSetHour(24, false, F("setCurrentHour rejects hour 24"));
    ok &= expectSetHour(5, true, F("setCurrentHour accepts valid hour 5"));

    // Проверка режимов, отличных от growing.
    ok &= expectModeFixed(Settings::SystemMode::Spring,
                          GrowingPeriod::Spring,
                          F("SystemMode::Spring forces Spring period and never completes"));
    ok &= expectModeFixed(Settings::SystemMode::Summer,
                          GrowingPeriod::Summer,
                          F("SystemMode::Summer forces Summer period and never completes"));
    ok &= expectModeFixed(Settings::SystemMode::Autumn,
                          GrowingPeriod::Autumn,
                          F("SystemMode::Autumn forces Autumn period and never completes"));

    // Восстанавливаем исходный режим и позицию.
    Settings::Manager::set(Key::SystemMode, static_cast<uint32_t>(Settings::SystemMode::Growing));
    setCyclePosition(1U, 0U);

    Serial.print(F("--- Automated tests result: "));
    Serial.println(ok ? F("SUCCESS") : F("FAILURE"));
    Serial.println();
    return ok;
}

void resetToDefaults() {
    applyDefaultSettings();
    Settings::Manager::set(Key::SystemMode, static_cast<uint32_t>(Settings::SystemMode::Growing));
    setCyclePosition(1U, 0U);
    GrowingCycle::begin(rtc);
}

void handleSetDay() {
    Serial.println(F("Enter new day (1..cycleLength):"));
    while (!Serial.available()) {}
    uint16_t day = static_cast<uint16_t>(Serial.parseInt());
    bool ok = GrowingCycle::setCurrentDay(day);
    Serial.print(F("setCurrentDay returned: "));
    Serial.println(ok ? F("true") : F("false"));
    printSummary(F("setCurrentDay"));
}

void handleSetHour() {
    Serial.println(F("Enter new hour (0..23):"));
    while (!Serial.available()) {}
    uint8_t hour = static_cast<uint8_t>(Serial.parseInt());
    bool ok = GrowingCycle::setCurrentHour(hour);
    Serial.print(F("setCurrentHour returned: "));
    Serial.println(ok ? F("true") : F("false"));
    printSummary(F("setCurrentHour"));
}

void handleSetElapsed() {
    Serial.println(F("Enter elapsed seconds since cycle start:"));
    while (!Serial.available()) {}
    uint32_t seconds = static_cast<uint32_t>(Serial.parseInt());
    rtc.setTime(seconds, false);
    Settings::Manager::set(Key::RtcInitTimeSec, rtc.getInitTime());
    printSummary(F("setElapsedSeconds"));
}

void handleSetMode() {
    Serial.println(F("Select mode (0=growing, 1=spring, 2=summer, 3=autumn):"));
    while (!Serial.available()) {}
    int32_t value = Serial.parseInt();
    if (value < 0 || value > 3) {
        Serial.println(F("[ERROR] Invalid mode value."));
        return;
    }
    bool ok = Settings::Manager::set(Key::SystemMode, static_cast<uint32_t>(value));
    Serial.print(F("set(SystemMode) returned: "));
    Serial.println(ok ? F("true") : F("false"));
    printSummary(F("setSystemMode"));
}

} // namespace

void setup() {
    Serial.begin(9600);
    while (!Serial) { /* wait for USB */ }

    rtc.begin(RTC_PIN_ENA, RTC_PIN_CLK, RTC_PIN_DAT);
    GrowingCycle::begin(rtc);

    resetToDefaults();

    Serial.println(F("=== Growing Cycle Module Test ==="));
    printMenu();

    runAutomatedTests();
    printSummary(F("initialisation"));
}

void loop() {
    if (!Serial.available()) {
        return;
    }

    char cmd = static_cast<char>(Serial.read());
    bool handled = true;
    switch (cmd) {
        case 'd':
        case 'D':
            printSummary(F("manual inspection"));
            break;
        case 's':
        case 'S':
            handleSetDay();
            break;
        case 'h':
        case 'H':
            handleSetHour();
            break;
        case 't':
        case 'T':
            handleSetElapsed();
            break;
        case 'm':
        case 'M':
            handleSetMode();
            break;
        case 'p':
        case 'P':
            printSeasons();
            break;
        case 'r':
        case 'R':
            resetToDefaults();
            printSummary(F("reset to defaults"));
            break;
        case 'a':
        case 'A':
            runAutomatedTests();
            printSummary(F("automated tests"));
            break;
        case '?':
            printMenu();
            break;
        default:
            handled = false;
            break;
    }

    if (handled && cmd != '?') {
        printMenu();
    }
}
