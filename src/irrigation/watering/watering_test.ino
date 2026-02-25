#include <Arduino.h>
#include "../settings/settings.h"
#include "watering.h"

using IrState = Irrigation::WateringState;
using Irrigation::Watering;

extern volatile unsigned long timer0_millis;

namespace {

constexpr uint8_t TEST_PUMP_PIN = 4; // свободный цифровой пин для стенда

constexpr unsigned long WATERING_DURATION_SEC = 6;
constexpr unsigned long WATERING_PAUSE_SEC = 60;
constexpr uint8_t WATERING_MAX_ATTEMPTS = 2;

constexpr unsigned long CLEANING_DURATION_SEC = 5;
constexpr unsigned long CLEANING_PAUSE_SEC = 10;
constexpr uint8_t CLEANING_CYCLES = 2;

constexpr uint16_t HUMIDITY_WET = 80;
constexpr uint16_t HUMIDITY_DRY = 35;
constexpr uint16_t HUMIDITY_TRAINING_READY = 25;

void advanceMillis(unsigned long deltaMs) {
    noInterrupts();
    timer0_millis += deltaMs;
    interrupts();
}

void configureSettings() {
    Settings::Manager::set(Settings::Key::SoilMoistureStartPercent, 45);
    Settings::Manager::set(Settings::Key::SoilMoistureStopPercent, 70);
    Settings::Manager::set(Settings::Key::WateringDurationSec, WATERING_DURATION_SEC);
    Settings::Manager::set(Settings::Key::WateringPauseSec, WATERING_PAUSE_SEC);
    Settings::Manager::set(Settings::Key::WateringMaxAttempts, WATERING_MAX_ATTEMPTS);
    Settings::Manager::set(Settings::Key::TrainingMoisturePercent, 30);
    Settings::Manager::set(Settings::Key::CleaningDurationSec, CLEANING_DURATION_SEC);
    Settings::Manager::set(Settings::Key::CleaningPauseSec, CLEANING_PAUSE_SEC);
    Settings::Manager::set(Settings::Key::CleaningCycles, CLEANING_CYCLES);
}

const __FlashStringHelper* stateName(IrState state) {
    switch (state) {
        case IrState::Stopping:         return F("Stopping");
        case IrState::Idle:             return F("Idle");
        case IrState::AutoPause:        return F("AutoPause");
        case IrState::AutoWatering:     return F("AutoWatering");
        case IrState::ManualWatering:   return F("ManualWatering");
        case IrState::ManualCleaning:   return F("ManualCleaning");
        case IrState::ManualPause:      return F("ManualPause");
        case IrState::TrainingWaiting:  return F("TrainingWaiting");
        case IrState::TrainingReady:    return F("TrainingReady");
        case IrState::OutOfWater:       return F("OutOfWater");
    }
    return F("Unknown");
}

constexpr unsigned long STEP_MS = 100UL;

void logTransition(IrState from, IrState to, const __FlashStringHelper* reason = nullptr) {
    Serial.print(F("  Transition: "));
    Serial.print(stateName(from));
    Serial.print(F(" -> "));
    Serial.println(stateName(to));
    if (reason) {
        Serial.print(F("    Reason: "));
        Serial.println(reason);
    }
}

IrState waitForState(IrState target,
                     uint16_t humidity,
                     unsigned long timeoutMs,
                     const __FlashStringHelper* context,
                     unsigned long stepMs = STEP_MS) {
    unsigned long elapsed = 0;
    IrState current = Watering::getState();
    Serial.print(F("Waiting for "));
    Serial.print(stateName(target));
    Serial.print(F(" ("));
    Serial.print(context);
    Serial.println(F(")"));

    if (current == target) {
        Serial.println(F("  Already in target state."));
        return current;
    }

    while (elapsed <= timeoutMs) {
        IrState before = current;
        for (uint8_t i = 0; i < 10; ++i) {
            advanceMillis(stepMs);
            Watering::process(humidity);
            elapsed += stepMs;
            current = Watering::getState();
            if (current != before) {
                break;
            }
        }
        current = Watering::getState();
        if (current != before) {
            logTransition(before, current);
        }
        if (current == target) {
            Serial.print(F("  Reached target after "));
            Serial.print(elapsed);
            Serial.println(F(" ms"));
            return current;
        }
    }

    Serial.print(F("  ERROR: did not reach "));
    Serial.print(stateName(target));
    Serial.print(F(" within "));
    Serial.print(timeoutMs);
    Serial.println(F(" ms"));
    return current;
}

void resetToIdle() {
    Watering::stop();
    for (uint8_t i = 0; i < 10; ++i) {
        advanceMillis(STEP_MS);
        Watering::process(HUMIDITY_WET);
    }
    Watering::setIdle();
    for (uint8_t i = 0; i < 10; ++i) {
        advanceMillis(STEP_MS);
        Watering::process(HUMIDITY_WET);
    }
    Serial.print(F("Reset -> "));
    Serial.println(stateName(Watering::getState()));
}

void testAutoCycleSuccess() {
    Serial.println(F("\n=== Test: Automatic cycle success ==="));
    resetToIdle();

    Watering::process(HUMIDITY_DRY);
    waitForState(IrState::AutoPause, HUMIDITY_DRY, 2000UL, F("Idle -> AutoPause"));

    waitForState(IrState::AutoWatering,
                 HUMIDITY_DRY,
                 (WATERING_PAUSE_SEC * 1000UL) + 5000UL,
                 F("AutoPause -> AutoWatering"));

    waitForState(IrState::AutoPause,
                 HUMIDITY_DRY,
                 (WATERING_DURATION_SEC * 1000UL) + 5000UL,
                 F("AutoWatering -> AutoPause"));

    waitForState(IrState::Idle,
                 HUMIDITY_WET,
                 (WATERING_PAUSE_SEC * 1000UL) + 5000UL,
                 F("AutoPause -> Idle (moisture stop)"));
}

void testAutoCycleOutOfWater() {
    Serial.println(F("\n=== Test: Automatic cycle -> OutOfWater ==="));
    resetToIdle();

    Watering::process(HUMIDITY_DRY);
    waitForState(IrState::AutoPause, HUMIDITY_DRY, 2000UL, F("Idle -> AutoPause"));

    for (uint8_t attempt = 1; attempt <= WATERING_MAX_ATTEMPTS; ++attempt) {
        Serial.print(F("  Dry attempt #"));
        Serial.println(attempt);
        waitForState(IrState::AutoWatering,
                     HUMIDITY_DRY,
                     (WATERING_PAUSE_SEC * 1000UL) + 5000UL,
                     F("AutoPause -> AutoWatering (dry)"));
        waitForState(IrState::AutoPause,
                     HUMIDITY_DRY,
                     (WATERING_DURATION_SEC * 1000UL) + 5000UL,
                     F("AutoWatering -> AutoPause (dry)"));
    }

    waitForState(IrState::OutOfWater,
                 HUMIDITY_DRY,
                 (WATERING_PAUSE_SEC * 1000UL) + 5000UL,
                 F("AutoPause -> OutOfWater (no moisture gain)"));

    waitForState(IrState::Idle,
                 HUMIDITY_DRY,
                 86400000UL + 1000UL,
                 F("OutOfWater -> Idle after 24h"),
                 60000UL);
}

void testManualWatering() {
    Serial.println(F("\n=== Test: Manual watering ==="));
    resetToIdle();

    Watering::setManualWatering();
    for (uint8_t i = 0; i < 10; ++i) {
        advanceMillis(STEP_MS);
        Watering::process(HUMIDITY_WET);
    }
    waitForState(IrState::ManualWatering, HUMIDITY_WET, 1000UL, F("Enter ManualWatering"));

    waitForState(IrState::Idle,
                 HUMIDITY_WET,
                 (WATERING_DURATION_SEC * 1000UL) + 2000UL,
                 F("ManualWatering -> Idle"));
}

void testManualCleaning() {
    Serial.println(F("\n=== Test: Manual cleaning ==="));
    resetToIdle();

    Watering::setManualCleaning();
    for (uint8_t i = 0; i < 10; ++i) {
        advanceMillis(STEP_MS);
        Watering::process(HUMIDITY_WET);
    }
    waitForState(IrState::ManualCleaning, HUMIDITY_WET, 1000UL, F("Enter ManualCleaning"));

    for (uint8_t cycle = 1; cycle <= CLEANING_CYCLES; ++cycle) {
        Serial.print(F("  Cleaning cycle #"));
        Serial.println(cycle);
        waitForState(IrState::ManualPause,
                     HUMIDITY_WET,
                     (CLEANING_DURATION_SEC * 1000UL) + 5000UL,
                     F("ManualCleaning -> ManualPause"));

        IrState next = (cycle == CLEANING_CYCLES) ? IrState::Idle : IrState::ManualCleaning;
        waitForState(next,
                     HUMIDITY_WET,
                     (CLEANING_PAUSE_SEC * 1000UL) + 5000UL,
                     F("ManualPause transition"));
    }
}

void testTrainingMode() {
    Serial.println(F("\n=== Test: Training mode ==="));
    resetToIdle();

    Watering::setTraining();
    for (uint8_t i = 0; i < 10; ++i) {
        advanceMillis(STEP_MS);
        Watering::process(HUMIDITY_DRY);
    }
    waitForState(IrState::TrainingWaiting, HUMIDITY_DRY, 1000UL, F("Enter TrainingWaiting"));

    Serial.println(F("  Check that high humidity keeps TrainingWaiting"));
    for (uint8_t i = 0; i < 5; ++i) {
        advanceMillis(STEP_MS);
        Watering::process(HUMIDITY_DRY);
    }
    Serial.println(F("  Feeding low humidity to reach TrainingReady"));

    waitForState(IrState::TrainingReady,
                 HUMIDITY_TRAINING_READY,
                 2000UL,
                 F("TrainingWaiting -> TrainingReady"));

    waitForState(IrState::Idle,
                 HUMIDITY_TRAINING_READY,
                 86400000UL + 1000UL,
                 F("TrainingReady -> Idle after 24h"),
                 60000UL);
}

} // namespace

void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    Serial.println(F("=== Watering FSM Test ==="));
    configureSettings();

    Watering::begin(TEST_PUMP_PIN);
    Watering::process(HUMIDITY_WET);

    testAutoCycleSuccess();
    testAutoCycleOutOfWater();
    testManualWatering();
    testManualCleaning();
    testTrainingMode();

    Serial.println(F("\nAll tests executed."));
}

void loop() {
    // Тест выполняется однократно в setup().
}

