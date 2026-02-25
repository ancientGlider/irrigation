/*
 * Settings storage definitions
 *
 * Этап 4.1: описание структуры данных настроек и формата хранения в EEPROM.
 * Здесь определяются:
 *   - структура настроек (Settings::Data)
 *   - заголовок блока с версией и размером (Settings::Header)
 *   - полный блок хранения с CRC (Settings::Block)
 *   - значения по умолчанию (Settings::DEFAULT_DATA)
 *
 * Фактическая реализация загрузки/сохранения будет добавлена на шаге 4.2.
 */

#pragma once

#include <Arduino.h>
#include <stddef.h>

namespace Settings {

enum class SystemMode : uint8_t {
    Growing = 0,
    Spring  = 1,
    Summer  = 2,
    Autumn  = 3
};

// ASCII сигнатура блока в EEPROM (используется для быстрой проверки присутствия данных)
inline constexpr char STORAGE_SIGNATURE[2] = {'I', 'R'}; // «Irrigation»

// Версия структуры данных. При изменении структуры увеличить значение.
inline constexpr uint8_t STORAGE_VERSION = 4;

// Используемая схема контроля целостности (CRC-16/IBM, полином 0xA001)
inline constexpr uint16_t CRC_POLYNOM = 0xA001U;

// Основные настройки системы (все значения сохраняются в EEPROM)
// Память критична, поэтому используются минимально достаточные типы.
#pragma pack(push, 1)
struct Data {
    uint8_t systemMode;              // Режим работы системы (см. SystemMode)
    uint8_t germinationLengthDays;    // Длительность проращивания
    uint8_t springLengthDays;        // Длительность весны
    uint8_t summerLengthDays;        // Длительность лета
    uint8_t autumnLengthDays;        // Длительность осени

    uint8_t springDayHours;          // Длительность дня весной
    uint8_t summerDayHours;          // Длительность дня летом
    uint8_t autumnDayHours;          // Длительность дня осенью

    uint16_t wateringDurationSec;    // Продолжительность одного полива (секунды, <= 120)
    uint16_t wateringPauseSec;       // Пауза между поливами (секунды, 60-3600)

    uint8_t soilMoistureStartPercent; // Порог запуска полива (%, min < stop)
    uint8_t soilMoistureStopPercent;  // Порог остановки полива (%, max)
    uint8_t wateringMaxAttempts;      // Максимум повторов полива (1-10)
    uint8_t trainingMoisturePercent;  // Целевая влажность в режиме тренировки (% < start)

    uint16_t cleaningDurationSec;    // Длительность одного полива в режиме очистки (секунды, <= 300)
    uint8_t cleaningCycles;          // Количество циклов очистки (<= 10)
    uint16_t cleaningPauseSec;       // Пауза между циклами очистки (секунды, <= 60)

    uint16_t sensorCheckPeriodSec;   // Период проверки датчиков (секунды)

    uint16_t soilCalibrationDry;     // Калибровка датчика почвы (значение «сухо», 0-1023)
    uint16_t soilCalibrationWet;     // Калибровка датчика почвы (значение «влажно», 0-1023)

    uint32_t rtcInitTimeSec;         // Время инициализации таймера RTC (секунды, 0..MAX_SECONDS)

    uint8_t flags;                   // Зарезервированные флаги (будущие расширения)
};

// Заголовок блока в EEPROM: сигнатура, версия, размер полезных данных
struct Header {
    char signature[2];               // STORAGE_SIGNATURE
    uint8_t version;                 // STORAGE_VERSION
    uint8_t length;                  // sizeof(Data)
};

// Полный блок, сохранённый в EEPROM: заголовок + данные + CRC16
struct Block {
    Header header;
    Data data;
    uint16_t crc;                    // CRC16 от header и data (без этого поля)
};
#pragma pack(pop)

// Статическая проверка размеров
static_assert(sizeof(Data) == 32, "Settings::Data must be 32 bytes");
static_assert(sizeof(Block) == sizeof(Header) + sizeof(Data) + sizeof(uint16_t),
              "Unexpected Settings::Block size");

// Значения по умолчанию (см. PROJECT.md и reference/irr_ver0.2.1.2)
inline constexpr Data DEFAULT_DATA = {
    /* systemMode             */ static_cast<uint8_t>(SystemMode::Growing),
    /* germinationLengthDays  */ 3U,
    /* springLengthDays       */ 33U,
    /* summerLengthDays       */ 34U,
    /* autumnLengthDays       */ 0U,

    /* springDayHours         */ 20U,
    /* summerDayHours         */ 18U,
    /* autumnDayHours         */ 12U,

    /* wateringDurationSec    */ 18U,
    /* wateringPauseSec       */ 600U,

    /* soilMoistureStartPercent */ 58U,
    /* soilMoistureStopPercent  */ 85U,
    /* wateringMaxAttempts      */ 6U,
    /* trainingMoisturePercent  */ 25U,

    /* cleaningDurationSec    */ 15U,
    /* cleaningCycles         */ 10U,
    /* cleaningPauseSec       */ 60U,

    /* sensorCheckPeriodSec   */ 600U,

    /* soilCalibrationDry     */ 500U,
    /* soilCalibrationWet     */ 180U,

    /* rtcInitTimeSec         */ 0UL,

    /* flags                  */ 0U
};

inline constexpr Header DEFAULT_HEADER = {
    {STORAGE_SIGNATURE[0], STORAGE_SIGNATURE[1]},
    STORAGE_VERSION,
    static_cast<uint8_t>(sizeof(Data))
};

// Блок по умолчанию (CRC заполняется позже при сохранении)
inline constexpr Block DEFAULT_BLOCK = {
    DEFAULT_HEADER,
    DEFAULT_DATA,
    0x0000U
};

} // namespace Settings
