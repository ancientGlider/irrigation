# Модуль настроек (`settings`)

## Назначение

Модуль обеспечивает загрузку, валидацию и сохранение системных настроек в EEPROM. Формат данных описан в `src/settings/settings_data.h`, а валидация соответствует требованиям проекта (`PROJECT.md`).

## Структура хранения

```
struct Header {
    char signature[2];   // 'I', 'R'
    uint8_t version;     // STORAGE_VERSION (сейчас 4)
    uint8_t length;      // sizeof(Data)
};

struct Data {            // 32 байта, см. settings_data.h
    uint8_t systemMode;
    uint8_t germinationLengthDays;
    uint8_t springLengthDays;
    uint8_t summerLengthDays;
    uint8_t autumnLengthDays;
    uint8_t springDayHours;
    uint8_t summerDayHours;
    uint8_t autumnDayHours;
    uint16_t wateringDurationSec;
    uint16_t wateringPauseSec;
    uint8_t soilMoistureStartPercent;
    uint8_t soilMoistureStopPercent;
    uint8_t wateringMaxAttempts;
    uint8_t trainingMoisturePercent;
    uint16_t cleaningDurationSec;
    uint8_t cleaningCycles;
    uint16_t cleaningPauseSec;
    uint16_t sensorCheckPeriodSec;
    uint16_t soilCalibrationDry;
    uint16_t soilCalibrationWet;
    uint32_t rtcInitTimeSec;
    uint8_t flags;
};

struct Block {
    Header header;
    Data data;
    uint16_t crc;        // CRC16/IBM от header+data
};
```

Полный блок занимает 38 байт. Сигнатура и версия помогают распознать корректный блок, CRC защищает от повреждения.

## API

```cpp
bool Settings::Manager::get(Key key, uint32_t& value);
bool Settings::Manager::set(Key key, uint32_t value);
```

- При первом обращении `Manager` автоматически загружает блок из EEPROM, проверяя сигнатуру, версию и CRC. При ошибке используется `DEFAULT_DATA`, который сразу записывается обратно. В оперативной памяти хранится только единый экземпляр `Settings::Data`.
 - `get` возвращает текущее значение настройки. `Key::SystemMode` возвращает одно из значений `Settings::SystemMode`. `Key::CycleLengthDays` вычисляется на лету (сумма длительностей всех периодов).
- `set` валидирует значение в контексте уже сохранённых параметров (диапазоны, суммарная длительность полива, взаимные зависимости влажности). При успешной проверке кэш обновляется и немедленно сохраняется в EEPROM.

## Ограничения и проверки

- `