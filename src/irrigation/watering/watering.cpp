#include "watering.h"

using Settings::Key;
using Settings::Manager;

namespace Irrigation {

WateringState Watering::_state = WateringState::Stopping;
WateringState Watering::_previousState = WateringState::Stopping;
Timer Watering::_stateTimer = Timer();
uint8_t Watering::_pumpPin = 0xFF;
bool Watering::_pumpEnabled = false;
bool Watering::_highIsOn = true;
uint8_t Watering::_attempts = 0U;
uint16_t Watering::_humidity = 0U;

void Watering::begin(uint8_t pinPump, bool highIsOn) {
    _pumpPin = pinPump;
    _highIsOn = highIsOn;
    pinMode(_pumpPin, OUTPUT);
    _pumpEnabled = true; // заставляем _setPump выполнить запись на пин
    _setPump(false);
    _state = WateringState::Stopping;
}

WateringState Watering::process(uint16_t soilMoisturePercent) {
    _humidity = soilMoisturePercent;
    switch (_state) {
        case WateringState::Stopping:
            return _state;

        case WateringState::Idle:
            _handleIdle();
            break;

        case WateringState::AutoPause:
            _handleAutoPause();
            break;

        case WateringState::AutoWatering:
            _handleAutoWatering();
            break;

        case WateringState::ManualWatering:
            _handleManualWatering();
            break;

        case WateringState::ManualCleaning:
            _handleManualCleaning();
            break;

        case WateringState::ManualPause:
            _handleManualPause();
            break;

        case WateringState::TrainingWaiting:
            _handleTrainingWaiting();
            break;

        case WateringState::TrainingReady:
            _handleTrainingReady();
            break;

        case WateringState::OutOfWater:
            _handleOutOfWater();
            break;
    }
    return _state;
}

void Watering::setManualWatering() {
    _transitionTo(WateringState::ManualWatering);
}

void Watering::setManualCleaning() {
    _transitionTo(WateringState::ManualCleaning);
}

void Watering::setTraining() {
    _transitionTo(WateringState::TrainingWaiting);
}

void Watering::setIdle() {
    _transitionTo(WateringState::Idle);
}

void Watering::stop() {
    _transitionTo(WateringState::Stopping);
}

WateringState Watering::getState() {
    return _state;
}

uint32_t Watering::getRemainingSeconds() {
    if (_state == WateringState::Stopping) {
        return 0U;
    }

    unsigned long periodMs = _stateTimer.getPeriod();
    if (periodMs == 0UL) {
        return 0U;
    }

    unsigned long elapsedMs = _stateTimer.getTime();
    if (elapsedMs >= periodMs) {
        return 0U;
    }

    unsigned long remainingMs = periodMs - elapsedMs;
    return static_cast<uint32_t>((remainingMs + 999UL) / 1000UL);
}

void Watering::_transitionTo(WateringState nextState) {
    if (_state == nextState) {
        return;
    }

    switch (nextState) {
        case WateringState::Stopping:
            _setPump(false);
            break;

        case WateringState::Idle:
            _setPump(false);
            break;

        case WateringState::AutoPause: {
            _setPump(false);
            if (_state == WateringState::Idle) {
                _attempts = 0U;
            }
            unsigned long pauseMs = _getSetting(Key::WateringPauseSec) * ONE_SECOND_MS;
            _stateTimer.setPeriod(pauseMs);
            break;
        }

        case WateringState::AutoWatering: {
            _setPump(true);
            unsigned long durationMs = _getSetting(Key::WateringDurationSec) * ONE_SECOND_MS;
            _stateTimer.setPeriod(durationMs);
            break;
        }

        case WateringState::ManualWatering: {
            _previousState = _state;
            _setPump(true);
            unsigned long durationMs = _getSetting(Key::WateringDurationSec) * ONE_SECOND_MS;
            _stateTimer.setPeriod(durationMs);
            break;
        }

        case WateringState::ManualCleaning: {
            if (_state != WateringState::ManualPause) {
                _previousState = _state;
                _attempts = 0U;
            }
            _setPump(true);
            unsigned long durationMs = _getSetting(Key::CleaningDurationSec) * ONE_SECOND_MS;
            _stateTimer.setPeriod(durationMs);
            break;
        }

        case WateringState::ManualPause: {
            _setPump(false);
            unsigned long pauseMs = _getSetting(Key::CleaningPauseSec) * ONE_SECOND_MS;
            _stateTimer.setPeriod(pauseMs);
            break;
        }

        case WateringState::TrainingWaiting:
            _setPump(false);
            break;

        case WateringState::TrainingReady:
            _setPump(false);
            _stateTimer.setPeriod(ONE_DAY_MS);
            break;

        case WateringState::OutOfWater:
            _setPump(false);
            _stateTimer.setPeriod(ONE_DAY_MS);
            break;
    }
    _state = nextState;

    return;
}

void Watering::_handleIdle() {
    uint16_t startThreshold = static_cast<uint16_t>(_getSetting(Key::SoilMoistureStartPercent));
    if (_humidity <= startThreshold) {
        _transitionTo(WateringState::AutoPause);
    }
}

void Watering::_handleAutoPause() {
    if (!_stateTimer.isReady()) {
        return;
    }

    uint16_t stopThreshold = static_cast<uint16_t>(_getSetting(Key::SoilMoistureStopPercent));
    if (_humidity >= stopThreshold) {
        _transitionTo(WateringState::Idle);
        return;
    }

    uint8_t maxAttempts = static_cast<uint8_t>(_getSetting(Key::WateringMaxAttempts));
    if (_attempts < maxAttempts) {
        ++_attempts;
        _transitionTo(WateringState::AutoWatering);
        return;
    }

    uint16_t startThreshold = static_cast<uint16_t>(_getSetting(Key::SoilMoistureStartPercent));
    if (_humidity > startThreshold) {
        _transitionTo(WateringState::Idle);
        return;
    }

    _transitionTo(WateringState::OutOfWater);
}

void Watering::_handleAutoWatering() {
    if (_stateTimer.isReady()) {
        _transitionTo(WateringState::AutoPause);
    }
}

void Watering::_handleManualWatering() {
    if (!_stateTimer.isReady()) {
        return;
    }

    if (_previousState == WateringState::Stopping) {
        _transitionTo(WateringState::Stopping);
    } else {
        _transitionTo(WateringState::Idle);
    }
}

void Watering::_handleManualCleaning() {
    if (_stateTimer.isReady()) {
        ++_attempts;
        _transitionTo(WateringState::ManualPause);
    }
}

void Watering::_handleManualPause() {
    if (!_stateTimer.isReady()) {
        return;
    }

    uint8_t cycles = static_cast<uint8_t>(_getSetting(Key::CleaningCycles));
    if (_attempts < cycles) {
        _transitionTo(WateringState::ManualCleaning);
        return;
    }

    if (_previousState == WateringState::Stopping) {
        _transitionTo(WateringState::Stopping);
    } else {
        _transitionTo(WateringState::Idle);
    }
}

void Watering::_handleTrainingWaiting() {
    uint16_t trainingThreshold = static_cast<uint16_t>(_getSetting(Key::TrainingMoisturePercent));
    if (_humidity <= trainingThreshold) {
        _transitionTo(WateringState::TrainingReady);
    }
}

void Watering::_handleTrainingReady() {
    if (_stateTimer.isReady()) {
        _transitionTo(WateringState::Idle);
    }
}

void Watering::_handleOutOfWater() {
    if (_stateTimer.isReady()) {
        _transitionTo(WateringState::Idle);
    }
}

void Watering::_setPump(bool enabled) {
    if (_pumpPin == 0xFF) {
        _pumpEnabled = enabled;
        return;
    }

    if (_pumpEnabled == enabled) {
        return;
    }

    _pumpEnabled = enabled;
    digitalWrite(_pumpPin, ((enabled) == _highIsOn) ? HIGH : LOW);
}

uint32_t Watering::_getSetting(Key key) {
    return Manager::get(key);
}

} // namespace Irrigation


