/*
 * Settings module API (Этап 4.2)
 *
 * Предоставляет функции загрузки, сохранения и валидации настроек,
 * сохранённых в EEPROM. Формат данных описан в settings_data.h.
 */

#pragma once

#include "settings_data.h"

namespace Settings {

// Количество байт, занимаемых блоком настроек в EEPROM
inline constexpr size_t STORAGE_SIZE = sizeof(Block);

/**
 * Идентификаторы отдельных настроек (используются геттерами/сеттерами).
 */
enum class Key : uint8_t {
    SystemMode,
    CycleLengthDays,
    GerminationLengthDays,
    SpringLengthDays,
    SummerLengthDays,
    AutumnLengthDays,
    SpringDayHours,
    SummerDayHours,
    AutumnDayHours,
    WateringDurationSec,
    WateringPauseSec,
    SoilMoistureStartPercent,
    SoilMoistureStopPercent,
    WateringMaxAttempts,
    TrainingMoisturePercent,
    CleaningDurationSec,
    CleaningCycles,
    CleaningPauseSec,
    SensorCheckPeriodSec,
    SoilCalibrationDry,
    SoilCalibrationWet,
    RtcInitTimeSec,
    Flags
};

/**
 * Класс-менеджер настроек. Использует статические методы и внутренний кэш,
 * чтобы избежать необходимости создавать объект в пользовательском коде.
 */
class Manager {
public:
    // Запрещаем создание экземпляров
    Manager() = delete;

    /** Получает значение отдельной настройки. */
    static uint32_t get(Key key);

    /** Устанавливает значение отдельной настройки. */
    static bool set(Key key, uint32_t value);

private:
    static constexpr uint32_t RTC_MAX_SECONDS = 3155760000UL;
    static void _ensureCacheLoaded();
    static bool _loadFromStorage();
    static void _storeToStorage();
    static void _makeBlockFromData();
    static void _readBlock();
    static void _writeBlock();
    static uint16_t _crc16(const uint8_t* buffer, size_t length);
    static bool _validateValue(Key key, uint32_t value);
    static bool _validateAll();

    // Статические данные
    static constexpr int EEPROM_ADDRESS = 0;
    static constexpr unsigned long MAX_IRRIGATION_TOTAL_SEC = 23UL * 3600UL;
    static Block _block;
    static bool _cacheLoaded;
};

} // namespace Settings
