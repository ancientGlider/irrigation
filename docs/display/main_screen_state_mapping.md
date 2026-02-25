# Соответствие данных главного экрана и источников

## WateringState → WateringViewState
- `WateringState::Stopping` → `WateringViewState::Stop`
- `WateringState::Idle` → `WateringViewState::Control`
- `WateringState::AutoPause`, `WateringState::AutoWatering` → `WateringViewState::AutoWatering`
- `WateringState::TrainingWaiting` → `WateringViewState::TrainingWait`
- `WateringState::TrainingReady` → `WateringViewState::Training`
- `WateringState::OutOfWater` → `WateringViewState::NoWater`
- `WateringState::ManualWatering`, `ManualCleaning`, `ManualPause` — главный экран не отображается (используются специализированные экраны ручного управления).

### Подписи состояний
- Хранятся в PROGMEM и выбираются по `WateringViewState`.
- Правило: одно слово → одна строка; два слова → две строки.
- Таблица (пример размещения в `display/main_screen_labels.h`):

```cpp
struct Caption {
    const char* line1;
    const char* line2;
};

constexpr Caption WATERING_CAPTIONS[] PROGMEM = {
    /* Stop         */ {"СТОП", nullptr},
    /* Control      */ {"КОНТРОЛЬ", nullptr},
    /* AutoWatering */ {"АВТО", "ПОЛИВ"},
    /* TrainingWait */ {"ОЖИДАНИЕ", "ТРЕНИРОВКИ"},
    /* Training     */ {"ТРЕНИРОВКА", nullptr},
    /* NoWater      */ {"НЕТ", "ВОДЫ"}
};
```

## Alerts
- `pumpActive`: берётся из `Watering::isPumpOn()`  
- `attention`: выставляется в статусах `WateringViewState::NoWater` и `WateringViewState::Training`.

## Telemetry
- `airTemperature`: из `AirSensor::getSensorData()` (температура в формате `int` в десятых долях градуса Цельсия, без умножения на 10).  
- `airHumidity`: из `AirSensor::getSensorData()` (влажность воздуха, целое значение 0–100).  
- `soilHumidity`: из `SoilSensor::getSensorData()` (процент влажности почвы).  
- `sensorCountdown`: из метода `Watering::getRemainingSeconds()` (секунды до следующей проверки датчиков).

## Cycle
- `season`: определяется по `GrowingCycle::getPeriod()`  
  - `GrowingPeriod::Germination` → `SeasonLabel::Germination` (отображается как "РОСТОК")
  - `GrowingPeriod::Spring` → `SeasonLabel::Spring`  
  - `GrowingPeriod::Summer` → `SeasonLabel::Summer`  
  - `GrowingPeriod::Autumn` → `SeasonLabel::Autumn`  
  - `GrowingPeriod::Completed` → `SeasonLabel::Completed` (отображается как "КОНЕЦ")
- `currentDay`: `GrowingCycle::getCurrentDay()`  
- `totalDays`: из `Settings::Manager::get(Key::CycleLengthDays)`

## Верхний блок
- `modeLabel`: определяется на основе `Settings::Manager::get(Key::SystemMode)`, отображаем как:
  - `SystemMode::Growing` → `РОСТ`
  - `SystemMode::Spring` → `ВЕСНА`
  - `SystemMode::Summer` → `ЛЕТО`
  - `SystemMode::Autumn` → `ОСЕНЬ`
- `lightOn`: `LightControl::isLightOn()`  
- `hour`/`minute`: из `GrowingCycle::getCurrentHour()`/`getCurrentMinute()`.  
- `alerts` использует структуру выше.

## Второй блок
- **Левая колонка**: отображается состояние модуля `watering` в соответствии с маппингом `WateringState → WateringViewState` (см. раздел выше) и подписями из `main_screen_labels.h` (структура `WATERING_CAPTIONS`).
- **Таймер датчиков** (третья строка левой колонки): `MainScreenTelemetry::sensorCountdown` получаем из метода `Watering::getRemainingSeconds()`.
- **Правая колонка**: отображаются данные датчиков (температура, влажность воздуха, влажность почвы) в формате, как на демо `main_screen_server.py`.

## Третий блок
- Отображается только если система работает в режиме `SystemMode::Growing`.
- Содержит текущий сезон и день цикла в формате, как на демо `main_screen_server.py`.
- В других режимах не отображается ничего.

## Блок иконок
- Отображаются иконки кнопок в формате, как на демо `main_screen_server.py`.

## Дополнительные требования
- Агрегатор `MainScreenStateProvider` должен подписаться на события от модулей или обновляться в `loop()`, чтобы гарантировать свежие данные до вызова `MainScreen::update()`.  
- Все строковые ресурсы (`РОСТ`, `ВЕСНА`, `ЛЕТО`, `ОСЕНЬ`, подписи режимов полива, сезонов) должны храниться в PROGMEM и выдаваться по enum для экономии SRAM.

