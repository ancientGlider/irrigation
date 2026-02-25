#include "growing_cycle.h"

namespace Irrigation {

using Settings::Key;
using Settings::Manager;

static constexpr uint32_t SECONDS_PER_DAY = 86400UL;
static constexpr uint32_t SECONDS_PER_HOUR = 3600UL;
static constexpr uint32_t SECONDS_PER_MINUTE = 60UL;

TimerRTC* GrowingCycle::_rtc = nullptr;
bool GrowingCycle::_initialized = false;

void GrowingCycle::begin(TimerRTC& rtc) {
    _rtc = &rtc;
    _initialized = true;
}

uint32_t GrowingCycle::_elapsedSeconds() {
    return _rtc ? _rtc->getTime() : 0U;
}

uint16_t GrowingCycle::_cycleLength() {
    return static_cast<uint16_t>(Manager::get(Key::CycleLengthDays));
}

uint16_t GrowingCycle::getCurrentDay() {
    return static_cast<uint16_t>(_elapsedSeconds() / SECONDS_PER_DAY + 1U);
}

uint8_t GrowingCycle::getCurrentHour() {
    uint32_t remainder = _elapsedSeconds() % SECONDS_PER_DAY;
    return static_cast<uint8_t>((remainder / SECONDS_PER_HOUR) % 24UL);
}

uint8_t GrowingCycle::getCurrentMinute() {
    uint32_t remainder = _elapsedSeconds() % SECONDS_PER_HOUR;
    return static_cast<uint8_t>((remainder / SECONDS_PER_MINUTE) % 60UL);
}

GrowingPeriod GrowingCycle::getPeriod() {
    Settings::SystemMode mode = static_cast<Settings::SystemMode>(Manager::get(Key::SystemMode));
    if (mode != Settings::SystemMode::Growing) {
        switch (mode) {
            case Settings::SystemMode::Spring:
                return GrowingPeriod::Spring;
            case Settings::SystemMode::Summer:
                return GrowingPeriod::Summer;
            case Settings::SystemMode::Autumn:
                return GrowingPeriod::Autumn;
        }
    }

    uint16_t day = getCurrentDay();

    uint8_t germinationDays = static_cast<uint8_t>(Manager::get(Key::GerminationLengthDays));
    if (day <= germinationDays) {
        return GrowingPeriod::Germination;
    }

    uint8_t springDays = static_cast<uint8_t>(Manager::get(Key::SpringLengthDays));
    uint16_t periodEnd = static_cast<uint16_t>(germinationDays) + springDays;
    if (day <= periodEnd) {
        return GrowingPeriod::Spring;
    }

    uint8_t summerDays = static_cast<uint8_t>(Manager::get(Key::SummerLengthDays));
    periodEnd += summerDays;
    if (day <= periodEnd) {
        return GrowingPeriod::Summer;
    }

    uint8_t autumnDays = static_cast<uint8_t>(Manager::get(Key::AutumnLengthDays));
    periodEnd += autumnDays;
    if (day <= periodEnd) {
        return GrowingPeriod::Autumn;
    }

    return GrowingPeriod::Completed;
}

bool GrowingCycle::setCurrentDay(uint16_t day) {
    if (!_rtc || !(day >= 1U && day <= _cycleLength())) return false;

    _rtc->setTime(_elapsedSeconds() % SECONDS_PER_DAY + (static_cast<uint32_t>(day - 1U) * SECONDS_PER_DAY));

    Manager::set(Key::RtcInitTimeSec, _rtc->getInitTime());
    
    return true;
}

bool GrowingCycle::setCurrentHour(uint8_t hour) {
    if (!_rtc || hour > 23U) return false;

    uint32_t elapsed = _elapsedSeconds();
    uint32_t dayBase = (elapsed / SECONDS_PER_DAY) * SECONDS_PER_DAY;
    uint32_t remainderWithinHour = elapsed % SECONDS_PER_HOUR;

    _rtc->setTime(dayBase + static_cast<uint32_t>(hour) * SECONDS_PER_HOUR + remainderWithinHour);
    Manager::set(Key::RtcInitTimeSec, _rtc->getInitTime());

    return true;
}

bool GrowingCycle::isCompleted() {
    if (!_rtc) return false;
    return getPeriod() == GrowingPeriod::Completed;
}

} // namespace Irrigation
