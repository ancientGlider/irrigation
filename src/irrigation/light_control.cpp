#include "light_control.h"
#include <Arduino.h>

namespace Irrigation {

using Settings::Key;
using Settings::Manager;

constexpr uint8_t GERMINATION_DAY_1_HOURS = 1;

LightState LightControl::_state = LightState::Uninitialized;
uint8_t LightControl::_pin = 0xFF;
bool LightControl::_highIsOn = true;

void LightControl::begin(uint8_t pin, bool highIsOn) {
    _pin = pin;
    _highIsOn = highIsOn;
    pinMode(_pin, OUTPUT);
    _applyState(LightState::Off);
    _recalculate();
}

void LightControl::update() {
    _recalculate();
}

bool LightControl::isLightOn() {
    return getLightState() == LightState::Day;
}

LightState LightControl::getLightState() {
    _recalculate();
    return _state;
}

void LightControl::_recalculate() {
    GrowingPeriod period = GrowingCycle::getPeriod();
    uint16_t day = GrowingCycle::getCurrentDay();
    uint8_t hour = GrowingCycle::getCurrentHour();

    switch (period) {
        case GrowingPeriod::Germination: {
            LightState state = (day == 1U && hour < GERMINATION_DAY_1_HOURS)
                                   ? LightState::Day
                                   : LightState::Night;
            _applyState(state);
            return;
        }
        case GrowingPeriod::Spring: {
            const uint8_t springDayHours = static_cast<uint8_t>(Manager::get(Key::SpringDayHours));
            _applyState((hour < springDayHours) ? LightState::Day : LightState::Night);
            return;
        }
        case GrowingPeriod::Summer: {
            const uint8_t summerDayHours = static_cast<uint8_t>(Manager::get(Key::SummerDayHours));
            _applyState((hour < summerDayHours) ? LightState::Day : LightState::Night);
            return;
        }
        case GrowingPeriod::Autumn: {
            const uint8_t autumnDayHours = static_cast<uint8_t>(Manager::get(Key::AutumnDayHours));
            _applyState((hour < autumnDayHours) ? LightState::Day : LightState::Night);
            return;
        }
        case GrowingPeriod::Completed:
            _applyState(LightState::Off);
            return;
    }
}

void LightControl::_applyState(LightState state) {
    if (_state == state) {
        // Состояние освещения не изменилось — дополнительная запись не требуется.
        return;
    }

    _state = state;

    if (_pin == 0xFF) {
        return;
    }

    digitalWrite(_pin, ((state == LightState::Day) == _highIsOn) ? HIGH : LOW);
}

} // namespace Irrigation