#pragma once

#include <Arduino.h>
#include "../../modules/timer/timer.h"
#include "../../settings/settings.h"

namespace Irrigation {

enum class WateringState : uint8_t {
    Stopping = 0,
    Idle,
    AutoPause,
    AutoWatering,
    ManualWatering,
    ManualCleaning,
    ManualPause,
    TrainingWaiting,
    TrainingReady,
    OutOfWater
};

class Watering {
public:
    Watering() = delete;

    static void begin(uint8_t pinPump, bool highIsOn = true);
    static WateringState process(uint16_t soilMoisturePercent);

    static void setManualWatering();
    static void setManualCleaning();
    static void setTraining();
    static void setIdle();
    static void stop();

    static WateringState getState();
    static inline bool isPumpOn() { return _pumpEnabled; }
    /** Количество секунд до завершения текущей задержки/цикла; 0, если таймер не активен. */
    static uint32_t getRemainingSeconds();

private:
    static void _transitionTo(WateringState nextState);

    static void _handleIdle();
    static void _handleAutoPause();
    static void _handleAutoWatering();
    static void _handleManualWatering();
    static void _handleManualCleaning();
    static void _handleManualPause();
    static void _handleTrainingWaiting();
    static void _handleTrainingReady();
    static void _handleOutOfWater();

    static void _setPump(bool enabled);
    static uint32_t _getSetting(Settings::Key key);

    static constexpr unsigned long ONE_SECOND_MS = 1000UL;
    static constexpr unsigned long ONE_DAY_MS = 86400UL * ONE_SECOND_MS;

    static WateringState _state;
    static WateringState _previousState;
    static Timer _stateTimer;
    static uint8_t _pumpPin;
    static bool _pumpEnabled;
    static bool _highIsOn;
    static uint8_t _attempts;
    static uint16_t _humidity;
};

} // namespace Irrigation


