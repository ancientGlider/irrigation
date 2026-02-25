/*
 * System state structures
 *
 * Определяет структуры данных для агрегированного состояния системы.
 * 
 * Используется:
 * - SystemController для хранения данных о состоянии системы
 * - Display для понимания структуры данных, переданных через указатель
 * 
 * SystemState является единым источником сведений о структуре данных,
 * описывающих состояние системы.
 */

#pragma once

#include <Arduino.h>
#include "watering/watering.h"
#include "growing_cycle.h"
#include "../settings/settings_data.h"

namespace Irrigation {

/**
 * Агрегированное состояние системы.
 * Содержит все данные, необходимые для отображения и управления.
 */
struct SystemState {
    // Состояние полива
    WateringState wateringState = WateringState::Idle;
    bool pumpActive = false;
    uint32_t wateringRemainingSeconds = 0;

    // Состояние цикла выращивания
    GrowingPeriod period = GrowingPeriod::Germination;
    uint16_t currentDay = 1;
    uint16_t totalDays = 70;
    uint8_t currentHour = 0;
    uint8_t currentMinute = 0;

    // Состояние освещения
    bool lightOn = false;

    // Данные датчиков
    struct {
        uint8_t soilHumidity = 0;        // 0-100%
        int airTemperature = 0;          // десятые доли °C (например, 235 = 23.5°C)
        int airHumidity = 0;             // десятые доли % (например, 580 = 58.0%)
        bool sensorsValid = false;       // валидность данных датчиков
    } sensors;

    // Системный режим
    Settings::SystemMode systemMode = Settings::SystemMode::Growing;

    // Время последнего обновления (для отладки)
    unsigned long lastUpdateMillis = 0;
};

} // namespace Irrigation

