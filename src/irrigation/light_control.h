/*
 * Light control module
 *
 * Управляет графиком освещения в зависимости от текущего дня, часа и периода
 * (проращивание, весна, лето, осень) полученных из GrowingCycle.
 */

#pragma once

#include "growing_cycle.h"

namespace Irrigation {

enum class LightState : uint8_t {
    Uninitialized = 0,
    Off,
    Day,
    Night
};

class LightControl {
public:
    LightControl() = delete;

    static void begin(uint8_t pin, bool highIsOn = true);
    static void update();
    static bool isLightOn();
    static LightState getLightState();

private:
    static void _recalculate();
    static void _applyState(LightState state);
    static LightState _state;
    static uint8_t _pin;
    static bool _highIsOn;
};

} // namespace Irrigation
