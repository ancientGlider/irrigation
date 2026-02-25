/*
 * Settings Module Test
 *
 * Протестируйте модуль на Arduino Nano:
 * 1. Загрузите этот скетч.
 * 2. Откройте Serial Monitor (9600 baud).
 * 3. Используйте команды:
 *      d - вывести текущие настройки
 *      r - перезаписать EEPROM значениями по умолчанию
 *      m - попробовать установить некорректные значения
 *      g - демонстрация работы get()/set()
 */
#include <Arduino.h>
#include "settings.h"

namespace {

void printSettings() {
    Serial.println(F("--- SETTINGS ---"));

    Serial.print(F("systemMode: ")); Serial.println(Settings::Manager::get(Settings::Key::SystemMode));
    Serial.print(F("cycleLengthDays: ")); Serial.println(Settings::Manager::get(Settings::Key::CycleLengthDays));
    Serial.print(F("germinationLengthDays: ")); Serial.println(Settings::Manager::get(Settings::Key::GerminationLengthDays));
    Serial.print(F("springLengthDays: ")); Serial.println(Settings::Manager::get(Settings::Key::SpringLengthDays));
    Serial.print(F("summerLengthDays: ")); Serial.println(Settings::Manager::get(Settings::Key::SummerLengthDays));
    Serial.print(F("autumnLengthDays: ")); Serial.println(Settings::Manager::get(Settings::Key::AutumnLengthDays));
    Serial.print(F("springDayHours: ")); Serial.println(Settings::Manager::get(Settings::Key::SpringDayHours));
    Serial.print(F("summerDayHours: ")); Serial.println(Settings::Manager::get(Settings::Key::SummerDayHours));
    Serial.print(F("autumnDayHours: ")); Serial.println(Settings::Manager::get(Settings::Key::AutumnDayHours));

    Serial.print(F("wateringDurationSec: ")); Serial.println(Settings::Manager::get(Settings::Key::WateringDurationSec));
    Serial.print(F("wateringPauseSec: ")); Serial.println(Settings::Manager::get(Settings::Key::WateringPauseSec));
    Serial.print(F("wateringMaxAttempts: ")); Serial.println(Settings::Manager::get(Settings::Key::WateringMaxAttempts));

    Serial.print(F("soilMoistureStartPercent: ")); Serial.println(Settings::Manager::get(Settings::Key::SoilMoistureStartPercent));
    Serial.print(F("soilMoistureStopPercent: ")); Serial.println(Settings::Manager::get(Settings::Key::SoilMoistureStopPercent));
    Serial.print(F("trainingMoisturePercent: ")); Serial.println(Settings::Manager::get(Settings::Key::TrainingMoisturePercent));

    Serial.print(F("cleaningDurationSec: ")); Serial.println(Settings::Manager::get(Settings::Key::CleaningDurationSec));
    Serial.print(F("cleaningCycles: ")); Serial.println(Settings::Manager::get(Settings::Key::CleaningCycles));
    Serial.print(F("cleaningPauseSec: ")); Serial.println(Settings::Manager::get(Settings::Key::CleaningPauseSec));

    Serial.print(F("sensorCheckPeriodSec: ")); Serial.println(Settings::Manager::get(Settings::Key::SensorCheckPeriodSec));

    Serial.print(F("soilCalibrationDry: ")); Serial.println(Settings::Manager::get(Settings::Key::SoilCalibrationDry));
    Serial.print(F("soilCalibrationWet: ")); Serial.println(Settings::Manager::get(Settings::Key::SoilCalibrationWet));

    Serial.print(F("rtcInitTimeSec: ")); Serial.println(Settings::Manager::get(Settings::Key::RtcInitTimeSec));
    Serial.print(F("flags: ")); Serial.println(Settings::Manager::get(Settings::Key::Flags));

    Serial.println(F("-----------------"));
}

void applyDefaultSettings() {
    const Settings::Data& defaults = Settings::DEFAULT_DATA;

    Settings::Manager::set(Settings::Key::GerminationLengthDays, defaults.germinationLengthDays);
    Settings::Manager::set(Settings::Key::SpringLengthDays, defaults.springLengthDays);
    Settings::Manager::set(Settings::Key::SummerLengthDays, defaults.summerLengthDays);
    Settings::Manager::set(Settings::Key::AutumnLengthDays, defaults.autumnLengthDays);
    Settings::Manager::set(Settings::Key::SpringDayHours, defaults.springDayHours);
    Settings::Manager::set(Settings::Key::SummerDayHours, defaults.summerDayHours);
    Settings::Manager::set(Settings::Key::AutumnDayHours, defaults.autumnDayHours);
    Settings::Manager::set(Settings::Key::WateringDurationSec, defaults.wateringDurationSec);
    Settings::Manager::set(Settings::Key::WateringPauseSec, defaults.wateringPauseSec);
    Settings::Manager::set(Settings::Key::WateringMaxAttempts, defaults.wateringMaxAttempts);
    Settings::Manager::set(Settings::Key::SoilMoistureStartPercent, defaults.soilMoistureStartPercent);
    Settings::Manager::set(Settings::Key::SoilMoistureStopPercent, defaults.soilMoistureStopPercent);
    Settings::Manager::set(Settings::Key::TrainingMoisturePercent, defaults.trainingMoisturePercent);
    Settings::Manager::set(Settings::Key::CleaningDurationSec, defaults.cleaningDurationSec);
    Settings::Manager::set(Settings::Key::CleaningCycles, defaults.cleaningCycles);
    Settings::Manager::set(Settings::Key::CleaningPauseSec, defaults.cleaningPauseSec);
    Settings::Manager::set(Settings::Key::SensorCheckPeriodSec, defaults.sensorCheckPeriodSec);
    Settings::Manager::set(Settings::Key::SoilCalibrationDry, defaults.soilCalibrationDry);
    Settings::Manager::set(Settings::Key::SoilCalibrationWet, defaults.soilCalibrationWet);
    Settings::Manager::set(Settings::Key::RtcInitTimeSec, defaults.rtcInitTimeSec);
    Settings::Manager::set(Settings::Key::Flags, defaults.flags);
    Settings::Manager::set(Settings::Key::SystemMode, defaults.systemMode);
}

void printMenu() {
    Serial.println();
    Serial.println(F("Commands:"));
    Serial.println(F("  d - print current EEPROM snapshot"));
    Serial.println(F("  r - apply default settings"));
    Serial.println(F("  m - run invalid update demo"));
    Serial.println(F("  g - run get/set demo"));
    Serial.println();
}

void printSummary(const __FlashStringHelper* context) {
    Serial.println();
    Serial.print(F("=== EEPROM Snapshot after "));
    Serial.print(context);
    Serial.println(F(" ==="));
    printSettings();
}

void expectSet(Settings::Key key, uint32_t value, bool expectSuccess, const __FlashStringHelper* rationale) {
    Serial.println(rationale);
    Serial.print(F("    Expectation: "));
    Serial.println(expectSuccess ? F("SUCCESS") : F("FAILURE"));
    Serial.print(F("    Value: "));
    Serial.println(value);

    bool ok = Settings::Manager::set(key, value);
    Serial.print(F("    Result: "));
    Serial.println(ok ? F("SUCCESS") : F("FAILURE"));
    Serial.println(ok == expectSuccess
        ? F("    Result matches expectation.")
        : F("    RESULT DOES NOT MATCH EXPECTATION!"));
    Serial.println();
}

void expectValue(Settings::Key key, uint32_t expected, const __FlashStringHelper* rationale) {
    Serial.println(rationale);
    uint32_t actual = Settings::Manager::get(key);
    Serial.print(F("    Expected: "));
    Serial.println(expected);
    Serial.print(F("    Actual:   "));
    Serial.println(actual);
    Serial.println(actual == expected
        ? F("    Value matches expectation.")
        : F("    VALUE DOES NOT MATCH EXPECTATION!"));
    Serial.println();
}

void demoInvalidUpdates() {
    Serial.println(F("--- Invalid updates ---"));

    expectSet(Settings::Key::SystemMode,
              4,
              false,
              F("[EXPECT] Setting systemMode=4 should fail (allowed values 0..3)"));

    expectSet(Settings::Key::WateringDurationSec,
              500,
              false,
              F("[EXPECT] Setting wateringDurationSec=500 should fail (exceeds 120-second limit)"));

    expectSet(Settings::Key::WateringPauseSec,
              10,
              false,
              F("[EXPECT] Setting wateringPauseSec=10 should fail (below 60-second minimum)"));

    expectSet(Settings::Key::GerminationLengthDays,
              8,
              false,
              F("[EXPECT] Setting germinationLengthDays=8 should fail (allowed range 3..7 days)"));

    expectSet(Settings::Key::SoilMoistureStartPercent,
              90,
              false,
              F("[EXPECT] Setting soilMoistureStartPercent=90 should fail (must stay below stop threshold)"));

    expectSet(Settings::Key::SpringLengthDays,
              200,
              false,
              F("[EXPECT] Setting springLengthDays=200 should fail (cycle would exceed 200 days)"));

    expectSet(Settings::Key::AutumnLengthDays,
              201,
              false,
              F("[EXPECT] Setting autumnLengthDays=201 should fail (constrains cycle to <=200 days)"));

    expectSet(Settings::Key::SpringDayHours,
              23,
              false,
              F("[EXPECT] Setting springDayHours=23 should fail (allowed range 19..22 hours)"));

    expectSet(Settings::Key::AutumnDayHours,
              7,
              false,
              F("[EXPECT] Setting autumnDayHours=7 should fail (allowed range 8..14 hours)"));

    Serial.println(F("--- End invalid updates ---"));
    printSummary(F("invalid update demo"));
}

void demoGetSet() {
    Serial.println(F("--- Testing get/set ---"));

    uint32_t originalMode = Settings::Manager::get(Settings::Key::SystemMode);
    uint32_t originalWatering = Settings::Manager::get(Settings::Key::WateringDurationSec);
    uint32_t originalStop = Settings::Manager::get(Settings::Key::SoilMoistureStopPercent);
    uint32_t originalStart = Settings::Manager::get(Settings::Key::SoilMoistureStartPercent);
    uint32_t originalTraining = Settings::Manager::get(Settings::Key::TrainingMoisturePercent);
    uint32_t originalSpringDayHours = Settings::Manager::get(Settings::Key::SpringDayHours);

    expectValue(Settings::Key::WateringDurationSec,
                originalWatering,
                F("[VERIFY] Initial wateringDurationSec snapshot before modifications"));

    expectSet(Settings::Key::SystemMode,
              static_cast<uint32_t>(Settings::SystemMode::Summer),
              true,
              F("[EXPECT] Setting systemMode=Summer should succeed"));
    expectValue(Settings::Key::SystemMode,
                static_cast<uint32_t>(Settings::SystemMode::Summer),
                F("[VERIFY] systemMode should equal Summer after valid update"));

    expectSet(Settings::Key::WateringDurationSec,
              90,
              true,
              F("[EXPECT] Setting wateringDurationSec=90 should succeed (within 1..120 seconds)"));
    expectValue(Settings::Key::WateringDurationSec,
                90,
                F("[VERIFY] wateringDurationSec should update to 90 after valid set"));

    expectSet(Settings::Key::WateringDurationSec,
              500,
              false,
              F("[EXPECT] Setting wateringDurationSec=500 should fail (exceeds allowed upper bound)"));
    expectValue(Settings::Key::WateringDurationSec,
                90,
                F("[VERIFY] wateringDurationSec remains 90 after invalid attempt"));

    expectSet(Settings::Key::SystemMode,
              5,
              false,
              F("[EXPECT] Setting systemMode=5 should fail (out of range)"));
    expectValue(Settings::Key::SystemMode,
                static_cast<uint32_t>(Settings::SystemMode::Summer),
                F("[VERIFY] systemMode remains Summer after invalid attempt"));

    Serial.println(F("Adjusting seasonal day hours..."));
    expectSet(Settings::Key::SpringDayHours,
              19,
              true,
              F("[EXPECT] Setting springDayHours=19 should succeed (within 19..22 hour range)"));
    expectValue(Settings::Key::SpringDayHours,
                19,
                F("[VERIFY] springDayHours should equal 19 after valid update"));

    expectSet(Settings::Key::SpringDayHours,
              25,
              false,
              F("[EXPECT] Setting springDayHours=25 should fail (above maximum 22 hours)"));
    expectValue(Settings::Key::SpringDayHours,
                19,
                F("[VERIFY] springDayHours stays at 19 after invalid attempt"));

    expectSet(Settings::Key::SpringDayHours,
              originalSpringDayHours,
              true,
              F("[EXPECT] Restoring springDayHours to original value should succeed"));

    Serial.println(F("Testing soil moisture thresholds..."));
    expectSet(Settings::Key::SoilMoistureStopPercent,
              80,
              true,
              F("[EXPECT] Setting soilMoistureStopPercent=80 should succeed (within 0..100 range)"));

    expectSet(Settings::Key::TrainingMoisturePercent,
              81,
              false,
              F("[EXPECT] Setting trainingMoisturePercent=81 should fail (must stay below stop threshold 80)"));

    expectSet(Settings::Key::TrainingMoisturePercent,
              50,
              true,
              F("[EXPECT] Setting trainingMoisturePercent=50 should succeed (below start and stop thresholds)"));
    expectValue(Settings::Key::TrainingMoisturePercent,
                50,
                F("[VERIFY] trainingMoisturePercent stored value should now be 50"));

    expectSet(Settings::Key::SoilMoistureStartPercent,
              40,
              false,
              F("[EXPECT] Setting soilMoistureStartPercent=40 should fail because training=50 must remain below start"));
    expectValue(Settings::Key::SoilMoistureStartPercent,
                originalStart,
                F("[VERIFY] soilMoistureStartPercent remains at original value after invalid attempt"));

    expectSet(Settings::Key::TrainingMoisturePercent,
              30,
              true,
              F("[EXPECT] Lowering trainingMoisturePercent to 30 should succeed (still below start/stop thresholds)"));
    expectValue(Settings::Key::TrainingMoisturePercent,
                30,
                F("[VERIFY] trainingMoisturePercent stored value should now be 30"));

    expectSet(Settings::Key::SoilMoistureStartPercent,
              40,
              true,
              F("[EXPECT] With training=30, setting soilMoistureStartPercent=40 should succeed (start < stop and training < start)"));
    expectValue(Settings::Key::SoilMoistureStartPercent,
                40,
                F("[VERIFY] soilMoistureStartPercent stored value should now be 40"));

    expectSet(Settings::Key::WateringDurationSec,
              originalWatering,
              true,
              F("[EXPECT] Restoring wateringDurationSec to original value should succeed"));

    expectSet(Settings::Key::SystemMode,
              originalMode,
              true,
              F("[EXPECT] Restoring systemMode to original value should succeed"));
    expectValue(Settings::Key::SystemMode,
                originalMode,
                F("[VERIFY] systemMode restored to original value"));

    expectSet(Settings::Key::SoilMoistureStopPercent,
              originalStop,
              true,
              F("[EXPECT] Restoring soilMoistureStopPercent to original value should succeed"));

    expectSet(Settings::Key::SoilMoistureStartPercent,
              originalStart,
              true,
              F("[EXPECT] Restoring soilMoistureStartPercent to original value should succeed"));

    expectSet(Settings::Key::TrainingMoisturePercent,
              originalTraining,
              true,
              F("[EXPECT] Restoring trainingMoisturePercent to original value should succeed"));

    Serial.println(F("--- End get/set test ---"));
    printSummary(F("get/set demo"));
}

} // namespace

void setup() {
    Serial.begin(9600);
    while (!Serial) { /* wait for USB */ }

    Serial.println(F("=== Settings Module Test ==="));
    Serial.println(F("EEPROM retains previous values. Use 'r' to apply defaults when required."));

    printSummary(F("initial power-up"));
    printMenu();
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
            case 'r':
            case 'R':
                Serial.println(F("[ACTION] Applying default settings (user request)"));
                applyDefaultSettings();
                printSummary(F("defaults applied by user"));
                break;
            case 'm':
            case 'M':
                demoInvalidUpdates();
                break;
            case 'g':
            case 'G':
                demoGetSet();
                break;
            default:
                handled = false;
                break;
        }

        if (handled) {
            printMenu();
        }
    }
}


