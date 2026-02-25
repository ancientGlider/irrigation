/*
 * Menu data structures and tables (PROGMEM)
 *
 * Описание структур меню и таблицы данных.
 * Все таблицы хранятся во Flash для экономии RAM.
 */

#pragma once

#include <Arduino.h>
#include "menu_strings.h"
#include "../settings/settings.h"

namespace Menu {

// ============================================================================
// Типы данных
// ============================================================================

/**
 * Типы пунктов меню
 */
enum class MenuItemType : uint8_t {
    Submenu,    // Переход в подменю
    Value,      // Редактируемое значение
    Action      // Действие (тренировка, очистка, сброс)
};

/**
 * Коды действий для Action-пунктов
 */
enum class ActionCode : uint8_t {
    StartTraining = 0,   // SystemController::setTraining()
    StartCleaning = 1,   // SystemController::setManualCleaning()
    ResetSettings = 2    // Settings reset to defaults
};

/**
 * Режимы отображения значений
 */
enum class DisplayMode : uint8_t {
    AsNumber,     // Число как есть
    AsMinutes,    // Секунды → минуты (value / 60)
    AsPercent,    // С символом %
    AsEnum,       // Из списка строк (для SystemMode)
    AsTimeHHMM    // Время в формате HH:MM (value = минуты от 00:00)
};

/**
 * Расширенные ключи параметров (включая виртуальные)
 * Значения 0-99 соответствуют Settings::Key
 * Значения 100+ — виртуальные параметры
 */
enum class ParamKey : uint8_t {
    // Settings::Key values (0-99)
    SystemMode              = static_cast<uint8_t>(Settings::Key::SystemMode),
    GerminationLengthDays   = static_cast<uint8_t>(Settings::Key::GerminationLengthDays),
    SpringLengthDays        = static_cast<uint8_t>(Settings::Key::SpringLengthDays),
    SummerLengthDays        = static_cast<uint8_t>(Settings::Key::SummerLengthDays),
    AutumnLengthDays        = static_cast<uint8_t>(Settings::Key::AutumnLengthDays),
    SpringDayHours          = static_cast<uint8_t>(Settings::Key::SpringDayHours),
    SummerDayHours          = static_cast<uint8_t>(Settings::Key::SummerDayHours),
    AutumnDayHours          = static_cast<uint8_t>(Settings::Key::AutumnDayHours),
    WateringDurationSec     = static_cast<uint8_t>(Settings::Key::WateringDurationSec),
    WateringPauseSec        = static_cast<uint8_t>(Settings::Key::WateringPauseSec),
    SoilMoistureStartPercent = static_cast<uint8_t>(Settings::Key::SoilMoistureStartPercent),
    SoilMoistureStopPercent  = static_cast<uint8_t>(Settings::Key::SoilMoistureStopPercent),
    WateringMaxAttempts     = static_cast<uint8_t>(Settings::Key::WateringMaxAttempts),
    TrainingMoisturePercent = static_cast<uint8_t>(Settings::Key::TrainingMoisturePercent),
    CleaningDurationSec     = static_cast<uint8_t>(Settings::Key::CleaningDurationSec),
    CleaningCycles          = static_cast<uint8_t>(Settings::Key::CleaningCycles),
    CleaningPauseSec        = static_cast<uint8_t>(Settings::Key::CleaningPauseSec),
    SensorCheckPeriodSec    = static_cast<uint8_t>(Settings::Key::SensorCheckPeriodSec),
    SoilCalibrationDry      = static_cast<uint8_t>(Settings::Key::SoilCalibrationDry),
    SoilCalibrationWet      = static_cast<uint8_t>(Settings::Key::SoilCalibrationWet),
    
    // Virtual parameters (100+)
    CurrentDay  = 100,   // Виртуальный день (через GrowingCycle)
    CurrentHour = 101    // Виртуальный час (через GrowingCycle)
};

/**
 * Структура пункта меню
 */
struct MenuItem {
    const char* label;        // Название пункта (PROGMEM строка)
    MenuItemType type;        // Тип пункта
    uint16_t data;            // Зависит от типа:
                              //   Submenu: индекс подменю в SUBMENUS[]
                              //   Value: ParamKey
                              //   Action: ActionCode
};

/**
 * Структура подменю
 */
struct Submenu {
    const char* title;        // Заголовок подменю (PROGMEM строка)
    const MenuItem* items;    // Массив пунктов (PROGMEM)
    uint8_t itemCount;        // Количество пунктов
};

/**
 * Дескриптор параметра для редактирования
 */
struct ParamDescriptor {
    ParamKey key;             // Ключ параметра
    int16_t minValue;         // Минимальное значение
    int16_t maxValue;         // Максимальное значение
    uint8_t step;             // Шаг изменения
    DisplayMode displayMode;  // Режим отображения
    uint8_t hasLiveValue : 1; // Показывать live-значение с датчика
    uint8_t hasDynamicMax : 1; // Динамический maxValue
    uint8_t hasDynamicMin : 1; // Динамический minValue
    uint8_t reserved : 5;     // Резерв
};

// ============================================================================
// Подменю
// ============================================================================

// --- ЦИКЛ ---
const MenuItem CYCLE_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_MODE,   MenuItemType::Value, static_cast<uint16_t>(ParamKey::SystemMode) },
    { STR_PARAM_DAY,    MenuItemType::Value, static_cast<uint16_t>(ParamKey::CurrentDay) },
    { STR_PARAM_HOUR,   MenuItemType::Value, static_cast<uint16_t>(ParamKey::CurrentHour) },
    { STR_PARAM_GERM,   MenuItemType::Value, static_cast<uint16_t>(ParamKey::GerminationLengthDays) },
    { STR_PARAM_SPRING, MenuItemType::Value, static_cast<uint16_t>(ParamKey::SpringLengthDays) },
    { STR_PARAM_SUMMER, MenuItemType::Value, static_cast<uint16_t>(ParamKey::SummerLengthDays) },
    { STR_PARAM_AUTUMN, MenuItemType::Value, static_cast<uint16_t>(ParamKey::AutumnLengthDays) }
};

// --- СВЕТ ---
const MenuItem LIGHT_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_SPRING, MenuItemType::Value, static_cast<uint16_t>(ParamKey::SpringDayHours) },
    { STR_PARAM_SUMMER, MenuItemType::Value, static_cast<uint16_t>(ParamKey::SummerDayHours) },
    { STR_PARAM_AUTUMN, MenuItemType::Value, static_cast<uint16_t>(ParamKey::AutumnDayHours) }
};

// --- ПОЛИВ (включает TrainingMoisturePercent) ---
const MenuItem WATER_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_DURATION,  MenuItemType::Value, static_cast<uint16_t>(ParamKey::WateringDurationSec) },
    { STR_PARAM_PAUSE,     MenuItemType::Value, static_cast<uint16_t>(ParamKey::WateringPauseSec) },
    { STR_PARAM_MIN_PCT,   MenuItemType::Value, static_cast<uint16_t>(ParamKey::SoilMoistureStartPercent) },
    { STR_PARAM_MAX_PCT,   MenuItemType::Value, static_cast<uint16_t>(ParamKey::SoilMoistureStopPercent) },
    { STR_PARAM_ATTEMPTS,  MenuItemType::Value, static_cast<uint16_t>(ParamKey::WateringMaxAttempts) },
    { STR_PARAM_TRAIN_PCT, MenuItemType::Value, static_cast<uint16_t>(ParamKey::TrainingMoisturePercent) }
};

// --- ОЧИСТКА ---
const MenuItem CLEAN_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_CLEAN_DUR,    MenuItemType::Value, static_cast<uint16_t>(ParamKey::CleaningDurationSec) },
    { STR_PARAM_CLEAN_CYCLES, MenuItemType::Value, static_cast<uint16_t>(ParamKey::CleaningCycles) },
    { STR_PARAM_CLEAN_PAUSE,  MenuItemType::Value, static_cast<uint16_t>(ParamKey::CleaningPauseSec) }
};

// --- ДАТЧИК ---
const MenuItem SENSOR_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_CHECK_PERIOD, MenuItemType::Value, static_cast<uint16_t>(ParamKey::SensorCheckPeriodSec) },
    { STR_PARAM_CALIB_DRY,    MenuItemType::Value, static_cast<uint16_t>(ParamKey::SoilCalibrationDry) },
    { STR_PARAM_CALIB_WET,    MenuItemType::Value, static_cast<uint16_t>(ParamKey::SoilCalibrationWet) }
};

// Массив всех подменю
const Submenu SUBMENUS[] PROGMEM = {
    { STR_MENU_CYCLE,  CYCLE_SUBMENU_ITEMS,  7 },  // [0]
    { STR_MENU_LIGHT,  LIGHT_SUBMENU_ITEMS,  3 },  // [1]
    { STR_MENU_WATER,  WATER_SUBMENU_ITEMS,  6 },  // [2]
    { STR_MENU_CLEAN,  CLEAN_SUBMENU_ITEMS,  3 },  // [3]
    { STR_MENU_SENSOR, SENSOR_SUBMENU_ITEMS, 3 }   // [4]
};
constexpr uint8_t SUBMENUS_COUNT = sizeof(SUBMENUS) / sizeof(Submenu);

// ============================================================================
// Главное меню
// ============================================================================

const MenuItem MAIN_MENU_ITEMS[] PROGMEM = {
    // Actions — в начале меню (символ "►" добавляется при отрисовке)
    { STR_MENU_TRAIN,  MenuItemType::Action,  static_cast<uint16_t>(ActionCode::StartTraining) },
    { STR_MENU_CLEAN,  MenuItemType::Action,  static_cast<uint16_t>(ActionCode::StartCleaning) },
    
    // Submenus — настройки (символ "*" добавляется при отрисовке)
    { STR_MENU_CYCLE,  MenuItemType::Submenu, 0 },  // → SUBMENUS[0]
    { STR_MENU_LIGHT,  MenuItemType::Submenu, 1 },  // → SUBMENUS[1]
    { STR_MENU_WATER,  MenuItemType::Submenu, 2 },  // → SUBMENUS[2]
    { STR_MENU_CLEAN,  MenuItemType::Submenu, 3 },  // → SUBMENUS[3] (та же строка!)
    { STR_MENU_SENSOR, MenuItemType::Submenu, 4 },  // → SUBMENUS[4]
    
    // Action — в конце меню
    { STR_MENU_RESET,  MenuItemType::Action,  static_cast<uint16_t>(ActionCode::ResetSettings) }
};
constexpr uint8_t MAIN_MENU_COUNT = sizeof(MAIN_MENU_ITEMS) / sizeof(MenuItem);

// ============================================================================
// Дескрипторы параметров
// ============================================================================

const ParamDescriptor PARAM_DESCRIPTORS[] PROGMEM = {
    // key                              min    max   step  display              live dynMax dynMin
    { ParamKey::SystemMode,               0,     3,    1,  DisplayMode::AsEnum,     0,  0,  0 },
    { ParamKey::CurrentDay,               1,   200,    1,  DisplayMode::AsNumber,   0,  0,  0 },  // *виртуальный
    { ParamKey::CurrentHour,              0,  1430,   10,  DisplayMode::AsTimeHHMM, 0,  0,  0 },  // *виртуальный
    { ParamKey::GerminationLengthDays,    3,     7,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::SpringLengthDays,         0,   197,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::SummerLengthDays,         0,   197,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::AutumnLengthDays,         0,   197,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::SpringDayHours,          19,    22,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::SummerDayHours,          15,    18,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::AutumnDayHours,           8,    14,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::WateringDurationSec,      1,   120,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::WateringPauseSec,        60,  3600,   60,  DisplayMode::AsMinutes,  0,  0,  0 },
    { ParamKey::SoilMoistureStartPercent, 0,    99,    1,  DisplayMode::AsPercent,  0,  1,  1 },  // dynMax, dynMin
    { ParamKey::SoilMoistureStopPercent,  1,   100,    1,  DisplayMode::AsPercent,  0,  0,  1 },  // dynMin
    { ParamKey::WateringMaxAttempts,      1,    10,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::TrainingMoisturePercent,  0,    99,    1,  DisplayMode::AsPercent,  0,  1,  0 },  // dynMax
    { ParamKey::CleaningDurationSec,      1,   300,    5,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::CleaningCycles,           1,    10,    1,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::CleaningPauseSec,        10,    60,    5,  DisplayMode::AsNumber,   0,  0,  0 },
    { ParamKey::SensorCheckPeriodSec,    60,  3600,   60,  DisplayMode::AsMinutes,  0,  0,  0 },
    { ParamKey::SoilCalibrationDry,       0,  1023,   10,  DisplayMode::AsNumber,   1,  0,  0 },  // live!
    { ParamKey::SoilCalibrationWet,       0,  1023,   10,  DisplayMode::AsNumber,   1,  0,  0 }   // live!
};
constexpr uint8_t PARAM_DESCRIPTORS_COUNT = sizeof(PARAM_DESCRIPTORS) / sizeof(ParamDescriptor);

// ============================================================================
// Вспомогательные функции
// ============================================================================

/**
 * Находит дескриптор параметра по ключу.
 * @param key Ключ параметра
 * @param out Выходной дескриптор (копируется из PROGMEM)
 * @return true если найден
 */
inline bool getParamDescriptor(ParamKey key, ParamDescriptor& out) {
    for (uint8_t i = 0; i < PARAM_DESCRIPTORS_COUNT; i++) {
        ParamDescriptor desc;
        memcpy_P(&desc, &PARAM_DESCRIPTORS[i], sizeof(ParamDescriptor));
        if (desc.key == key) {
            out = desc;
            return true;
        }
    }
    return false;
}

/**
 * Проверяет, является ли ключ виртуальным параметром.
 */
inline bool isVirtualParam(ParamKey key) {
    return static_cast<uint8_t>(key) >= 100;
}

/**
 * Конвертирует ParamKey в Settings::Key (только для не-виртуальных).
 */
inline Settings::Key toSettingsKey(ParamKey key) {
    return static_cast<Settings::Key>(static_cast<uint8_t>(key));
}

} // namespace Menu
