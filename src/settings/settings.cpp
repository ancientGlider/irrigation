#include "settings.h"

#include <EEPROM.h>
#include <string.h>

namespace Settings {

namespace {

constexpr Key kValidationOrder[] = {
    Key::SystemMode,
    Key::CycleLengthDays,
    Key::GerminationLengthDays,
    Key::SpringLengthDays,
    Key::SummerLengthDays,
    Key::AutumnLengthDays,
    Key::SpringDayHours,
    Key::SummerDayHours,
    Key::AutumnDayHours,
    Key::WateringDurationSec,
    Key::WateringPauseSec,
    Key::WateringMaxAttempts,
    Key::SoilMoistureStartPercent,
    Key::SoilMoistureStopPercent,
    Key::TrainingMoisturePercent,
    Key::CleaningDurationSec,
    Key::CleaningCycles,
    Key::CleaningPauseSec,
    Key::SensorCheckPeriodSec,
    Key::SoilCalibrationDry,
    Key::SoilCalibrationWet,
    Key::RtcInitTimeSec,
    Key::Flags
};

} // namespace

Block Manager::_block{};
bool Manager::_cacheLoaded = false;

uint16_t Manager::_crc16(const uint8_t* buffer, size_t length) {
    uint16_t crc = 0xFFFFU;
    for (size_t i = 0; i < length; ++i) {
        crc ^= buffer[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001U) {
                crc = (crc >> 1) ^ CRC_POLYNOM;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void Manager::_readBlock() {
    EEPROM.get(EEPROM_ADDRESS, _block);
}

void Manager::_writeBlock() {
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(&_block);
    for (size_t i = 0; i < sizeof(Block); ++i) {
        EEPROM.update(EEPROM_ADDRESS + static_cast<int>(i), raw[i]);
    }
}

void Manager::_makeBlockFromData() {
    _block.header = DEFAULT_HEADER;
    _block.crc = _crc16(reinterpret_cast<const uint8_t*>(&_block.header), sizeof(Header) + sizeof(Data));
}

void Manager::_storeToStorage() {
    _makeBlockFromData();
    _writeBlock();
}

bool Manager::_loadFromStorage() {
    _readBlock();

    bool headerOk = (_block.header.signature[0] == STORAGE_SIGNATURE[0]) &&
                    (_block.header.signature[1] == STORAGE_SIGNATURE[1]) &&
                    (_block.header.version == STORAGE_VERSION) &&
                    (_block.header.length == sizeof(Data));
    if (!headerOk) {
        return false;
    }

    uint16_t computed = _crc16(reinterpret_cast<const uint8_t*>(&_block.header), sizeof(Header) + sizeof(Data));
    if (computed != _block.crc) {
        return false;
    }

    if (!_validateAll()) {
        return false;
    }

    return true;
}

void Manager::_ensureCacheLoaded() {
    if (_cacheLoaded) {
        return;
    }

    if (!_loadFromStorage()) {
        _block.data = DEFAULT_DATA;
        _makeBlockFromData();
        _storeToStorage();
    }

    _cacheLoaded = true;
}

bool Manager::_validateAll() {
    auto extractValue = [&](Key key) -> uint32_t {
        switch (key) {
            case Key::CycleLengthDays:
                return static_cast<uint32_t>(_block.data.germinationLengthDays) +
                       static_cast<uint32_t>(_block.data.springLengthDays) +
                       static_cast<uint32_t>(_block.data.summerLengthDays) +
                       static_cast<uint32_t>(_block.data.autumnLengthDays);
            case Key::GerminationLengthDays: return _block.data.germinationLengthDays;
            case Key::SpringLengthDays:       return _block.data.springLengthDays;
            case Key::SummerLengthDays:       return _block.data.summerLengthDays;
            case Key::AutumnLengthDays:       return _block.data.autumnLengthDays;
            case Key::SpringDayHours:         return _block.data.springDayHours;
            case Key::SummerDayHours:         return _block.data.summerDayHours;
            case Key::AutumnDayHours:         return _block.data.autumnDayHours;
            case Key::WateringDurationSec:    return _block.data.wateringDurationSec;
            case Key::WateringPauseSec:       return _block.data.wateringPauseSec;
            case Key::WateringMaxAttempts:    return _block.data.wateringMaxAttempts;
            case Key::SoilMoistureStartPercent: return _block.data.soilMoistureStartPercent;
            case Key::SoilMoistureStopPercent:  return _block.data.soilMoistureStopPercent;
            case Key::TrainingMoisturePercent: return _block.data.trainingMoisturePercent;
            case Key::CleaningDurationSec:    return _block.data.cleaningDurationSec;
            case Key::CleaningCycles:         return _block.data.cleaningCycles;
            case Key::CleaningPauseSec:       return _block.data.cleaningPauseSec;
            case Key::SensorCheckPeriodSec:   return _block.data.sensorCheckPeriodSec;
            case Key::SoilCalibrationDry:     return _block.data.soilCalibrationDry;
            case Key::SoilCalibrationWet:     return _block.data.soilCalibrationWet;
            case Key::RtcInitTimeSec:         return _block.data.rtcInitTimeSec;
            case Key::SystemMode:             return _block.data.systemMode;
            case Key::Flags:                  return _block.data.flags;
        }
        return 0U;
    };

    for (Key key : kValidationOrder) {
        if (!_validateValue(key, extractValue(key))) {
            return false;
        }
    }

    return true;
}

bool Manager::_validateValue(Key key, uint32_t value) {
    switch (key) {
        case Key::CycleLengthDays:
            return value >= 30U && value <= 200U;

        case Key::SystemMode:
            return value <= static_cast<uint32_t>(SystemMode::Autumn);

        case Key::GerminationLengthDays:
            if (value < 3U || value > 7U) return false;
        case Key::SpringLengthDays:
        case Key::SummerLengthDays:
        case Key::AutumnLengthDays: {
            uint32_t cycle =
                ((key == Key::GerminationLengthDays) ? value : _block.data.germinationLengthDays) +
                ((key == Key::SpringLengthDays) ? value : _block.data.springLengthDays) +
                ((key == Key::SummerLengthDays) ? value : _block.data.summerLengthDays) +
                ((key == Key::AutumnLengthDays) ? value : _block.data.autumnLengthDays);
            return cycle >= 30U && cycle <= 200U;
        }

        case Key::SpringDayHours:
            return value >= 19U && value <= 22U;

        case Key::SummerDayHours:
            return value >= 15U && value <= 18U;

        case Key::AutumnDayHours:
            return value >= 8U && value <= 14U;

        case Key::WateringDurationSec:
        case Key::WateringPauseSec:
        case Key::WateringMaxAttempts: {
            uint32_t duration = (key == Key::WateringDurationSec) ? value : _block.data.wateringDurationSec;
            uint32_t pause    = (key == Key::WateringPauseSec) ? value : _block.data.wateringPauseSec;
            uint32_t attempts = (key == Key::WateringMaxAttempts) ? value : _block.data.wateringMaxAttempts;

            if (duration == 0U || duration > 120U) return false;
            if (pause < 60U || pause > 3600U) return false;
            if (attempts < 1U || attempts > 10U) return false;

            unsigned long durationTotal = static_cast<unsigned long>(duration) * attempts;
            unsigned long pauseTotal = (attempts > 1U)
                ? static_cast<unsigned long>(pause) * (attempts - 1U)
                : 0UL;
            return (durationTotal + pauseTotal) <= MAX_IRRIGATION_TOTAL_SEC;
        }

        case Key::SoilMoistureStartPercent:
        case Key::SoilMoistureStopPercent:
        case Key::TrainingMoisturePercent: {
            if (value > 100U) {
                return false;
            }
            uint32_t start = (key == Key::SoilMoistureStartPercent) ? value : _block.data.soilMoistureStartPercent;
            uint32_t stop = (key == Key::SoilMoistureStopPercent) ? value : _block.data.soilMoistureStopPercent;
            uint32_t training = (key == Key::TrainingMoisturePercent) ? value : _block.data.trainingMoisturePercent;
            if (start >= stop) return false;
            if (training >= start) return false;
            return true;
        }

        case Key::CleaningDurationSec:
            return value > 0U && value <= 300U;

        case Key::CleaningCycles:
            return value > 0U && value <= 10U;

        case Key::CleaningPauseSec:
            return value >= 10U && value <= 60U;

        case Key::SensorCheckPeriodSec:
            return value >= 60U && value <= 3600U;

        case Key::SoilCalibrationDry:
        case Key::SoilCalibrationWet: {
            if (value > 1023U) {
                return false;
            }
            uint32_t dry = (key == Key::SoilCalibrationDry) ? value : _block.data.soilCalibrationDry;
            uint32_t wet = (key == Key::SoilCalibrationWet) ? value : _block.data.soilCalibrationWet;
            return dry != wet;
        }

        case Key::RtcInitTimeSec:
            return true;

        case Key::Flags:
            return value == 0U;
    }

    return false;
}

uint32_t Manager::get(Key key) {
    _ensureCacheLoaded();

    switch (key) {
        case Key::CycleLengthDays: {
            return static_cast<uint32_t>(_block.data.germinationLengthDays) +
                   static_cast<uint32_t>(_block.data.springLengthDays) +
                   static_cast<uint32_t>(_block.data.summerLengthDays) +
                   static_cast<uint32_t>(_block.data.autumnLengthDays);
        }
        case Key::GerminationLengthDays: return _block.data.germinationLengthDays;
        case Key::SpringLengthDays: return _block.data.springLengthDays;
        case Key::SummerLengthDays: return _block.data.summerLengthDays;
        case Key::AutumnLengthDays: return _block.data.autumnLengthDays;
        case Key::SpringDayHours: return _block.data.springDayHours;
        case Key::SummerDayHours: return _block.data.summerDayHours;
        case Key::AutumnDayHours: return _block.data.autumnDayHours;
        case Key::WateringDurationSec: return _block.data.wateringDurationSec;
        case Key::WateringPauseSec: return _block.data.wateringPauseSec;
        case Key::WateringMaxAttempts: return _block.data.wateringMaxAttempts;
        case Key::SoilMoistureStartPercent: return _block.data.soilMoistureStartPercent;
        case Key::SoilMoistureStopPercent: return _block.data.soilMoistureStopPercent;
        case Key::TrainingMoisturePercent: return _block.data.trainingMoisturePercent;
        case Key::CleaningDurationSec: return _block.data.cleaningDurationSec;
        case Key::CleaningCycles: return _block.data.cleaningCycles;
        case Key::CleaningPauseSec: return _block.data.cleaningPauseSec;
        case Key::SensorCheckPeriodSec: return _block.data.sensorCheckPeriodSec;
        case Key::SoilCalibrationDry: return _block.data.soilCalibrationDry;
        case Key::SoilCalibrationWet: return _block.data.soilCalibrationWet;
        case Key::RtcInitTimeSec: return _block.data.rtcInitTimeSec;
        case Key::SystemMode: return _block.data.systemMode;
        case Key::Flags: return _block.data.flags;
    }
    return 0U;
}

bool Manager::set(Key key, uint32_t value) {
    _ensureCacheLoaded();

    if (!_validateValue(key, value)) {
        return false;
    }

    switch (key) {
        case Key::SpringLengthDays:
            _block.data.springLengthDays = static_cast<uint8_t>(value);
            break;
        case Key::SummerLengthDays:
            _block.data.summerLengthDays = static_cast<uint8_t>(value);
            break;
        case Key::AutumnLengthDays:
            _block.data.autumnLengthDays = static_cast<uint8_t>(value);
            break;
        case Key::GerminationLengthDays:
            _block.data.germinationLengthDays = static_cast<uint8_t>(value);
            break;
        case Key::SpringDayHours:
            _block.data.springDayHours = static_cast<uint8_t>(value);
            break;
        case Key::SummerDayHours:
            _block.data.summerDayHours = static_cast<uint8_t>(value);
            break;
        case Key::AutumnDayHours:
            _block.data.autumnDayHours = static_cast<uint8_t>(value);
            break;
        case Key::WateringDurationSec:
            _block.data.wateringDurationSec = static_cast<uint16_t>(value);
            break;
        case Key::WateringPauseSec:
            _block.data.wateringPauseSec = static_cast<uint16_t>(value);
            break;
        case Key::WateringMaxAttempts:
            _block.data.wateringMaxAttempts = static_cast<uint8_t>(value);
            break;
        case Key::SoilMoistureStartPercent:
            _block.data.soilMoistureStartPercent = static_cast<uint8_t>(value);
            break;
        case Key::SoilMoistureStopPercent:
            _block.data.soilMoistureStopPercent = static_cast<uint8_t>(value);
            break;
        case Key::TrainingMoisturePercent:
            _block.data.trainingMoisturePercent = static_cast<uint8_t>(value);
            break;
        case Key::CleaningDurationSec:
            _block.data.cleaningDurationSec = static_cast<uint16_t>(value);
            break;
        case Key::CleaningCycles:
            _block.data.cleaningCycles = static_cast<uint8_t>(value);
            break;
        case Key::CleaningPauseSec:
            _block.data.cleaningPauseSec = static_cast<uint16_t>(value);
            break;
        case Key::SensorCheckPeriodSec:
            _block.data.sensorCheckPeriodSec = static_cast<uint16_t>(value);
            break;
        case Key::SoilCalibrationDry:
            _block.data.soilCalibrationDry = static_cast<uint16_t>(value);
            break;
        case Key::SoilCalibrationWet:
            _block.data.soilCalibrationWet = static_cast<uint16_t>(value);
            break;
        case Key::RtcInitTimeSec:
            _block.data.rtcInitTimeSec = static_cast<uint32_t>(value) % RTC_MAX_SECONDS;
            break;
        case Key::SystemMode:
            _block.data.systemMode = static_cast<uint8_t>(value);
            break;
        case Key::Flags:
            _block.data.flags = static_cast<uint8_t>(value);
            break;
        case Key::CycleLengthDays:
            return false;
    }

    _storeToStorage();
    return true;
}

} // namespace Settings
