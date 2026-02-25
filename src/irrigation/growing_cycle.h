/*
 * Growing cycle module
 *
 * Управляет расчётом текущего дня, часа и периода (проращивание / весна / лето / осень)
 * на основе настроек и таймера реального времени (TimerRTC).
 */

#pragma once

#include <Arduino.h>
#include "../modules/timerRTC/timerRTC.h"
#include "../settings/settings.h"

namespace Irrigation {

enum class GrowingPeriod : uint8_t {
    Germination = 0,
    Spring,
    Summer,
    Autumn,
    Completed
};

class GrowingCycle {
public:
    GrowingCycle() = delete;

    /** Инициализирует модуль. */
    static void begin(TimerRTC& rtc);

    /** Возвращает текущий день цикла (1..cycleLength). */
    static uint16_t getCurrentDay();

    /** Возвращает текущий час (0..23) в рамках текущего дня. */
    static uint8_t getCurrentHour();

    /** Возвращает текущую минуту (0..59) в рамках текущего часа. */
    static uint8_t getCurrentMinute();

    /** Возвращает текущий период выращивания. */
    static GrowingPeriod getPeriod();

    /** Устанавливает текущий день. */
    static bool setCurrentDay(uint16_t day);

    /** Устанавливает текущий час (0..23) в рамках текущего дня. */
    static bool setCurrentHour(uint8_t hour);

    /** Возвращает true, если цикл завершён. */
    static bool isCompleted();

private:
    static uint32_t _elapsedSeconds();
    static uint16_t _cycleLength();

    static TimerRTC* _rtc;
    static bool _initialized;
};

} // namespace Irrigation
