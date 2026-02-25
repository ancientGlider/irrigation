# Модуль цикла выращивания (`growing_cycle`)

## Назначение

Отвечает за вычисление текущего дня, часа и периода выращивания (проращивание / весна / лето / осень) на основании:
- настроек (`Settings::Manager`), включая длительности сезонов
- времени, полученного через `TimerRTC`
- сохранённого значения `rtcInitTimeSec` (для восстановления после перезапуска)

## API

```cpp
class Irrigation::GrowingCycle {
public:
    static void begin(TimerRTC& rtc);
    static uint16_t getCurrentDay();
    static uint8_t getCurrentHour();
    static uint8_t getCurrentMinute();
    static GrowingPeriod getPeriod();
    static bool setCurrentDay(uint16_t day);
    static bool setCurrentHour(uint8_t hour);
    static bool isCompleted();
};
```

- `begin` — привязывает модуль к `TimerRTC` и синхронизирует время начала цикла из EEPROM.
- `getCurrentDay/Hour/Minute/Period` — вычисляют значения на лету, используя `TimerRTC` и текущие настройки.
- `setCurrentDay` — пересчитывает `rtcInitTimeSec`, обновляет `TimerRTC` и немедленно сохраняет значение в EEPROM через `Settings::Manager`. Возвращает `false`, если запрошенный день был вне диапазона.
- `setCurrentHour` — фиксирует конкретный час (0–23) в рамках текущего дня, сохраняя оставшиеся секунды внутри часа. Возвращает `false`, если час вне диапазона.
- `isCompleted` — возвращает `true`, когда период = `Completed` (достигнут конец цикла).

## Периоды

- День 1: свет включён только на 1 час (последующее время суток — ночь).
- Дни 2...`germinationLengthDays`: проращивание в темноте (настройка `GerminationLengthDays` задаёт диапазон 3-7 дней).
- Далее: весна, лето, осень согласно настройкам длительности периодов (`SpringLengthDays`, `SummerLengthDays`, `AutumnLengthDays`).
- Длительность светового дня берётся из `SpringDayHours` (19-22), `SummerDayHours` (15-18), `AutumnDayHours` (8-14).
- Валидация модуля настроек гарантирует, что суммарная длительность цикла (проращивание + сезоны) находится в диапазоне 30-200 дней.

## Хранение и восстановление

- При первом старте, если `rtcInitTimeSec` в EEPROM отсутствует, используется значение по умолчанию.
- При изменении дня модуль записывает новые `rtcInitTimeSec` в EEPROM через `Settings::Manager`.

## Тестирование

Используйте `growing_cycle_test.ino` (требуется подключённый DS1302). Команды:
- `d` — вывести текущий статус
- `s` — установить день
- `h` — установить час (по значению 0–23)
- `r` — сбросить цикл
- `t` — установить прошедшее время в секундах
- `p` — вывести длительности сезонов

Перед запуском убедитесь, что создан блок настроек (`settings_test.ino`), а `TimerRTC` подключён и синхронизирован.

---

# Модуль управления освещением (`light_control`)

## Назначение

Рассчитывает, должен ли свет быть включён, исходя из текущего периода выращивания и часа суток. Опирается на `GrowingCycle` и настройки (`spring/summer/autumn` длительности).

## API

```cpp
class Irrigation::LightControl {
public:
    static void begin(uint8_t pin, bool highIsOn = true);
    static void update();
    static bool isLightOn();
    static LightState getLightState();
};
```

- `begin(pin, highIsOn)` — конфигурирует цифровой пин управления освещением (`pinMode(pin, OUTPUT)`), задаёт уровень активного сигнала (по умолчанию HIGH включает свет) и выполняет первичный расчёт состояния.
- `update()` — синхронизирует состояние света с текущим временем в цикле выращивания.
- `isLightOn()` — упрощённая проверка (true, если состояние = `Day`).
- `getLightState()` — возвращает одно из значений: `Uninitialized`, `Off`, `Day`, `Night`.

## Алгоритм

- День 1 проращивания: свет горит первый час (остальное время суток — `Night`).
- Проращивание, дни 2..`germinationLengthDays`: свет выключен (`Night`).
- Весна: длительность светового дня задаётся настройкой `SpringDayHours` (допустимый диапазон 19–22 часов).
- Лето: длительность светового дня задаётся настройкой `SummerDayHours` (допустимый диапазон 15–18 часов).
- Осень: длительность светового дня задаётся настройкой `AutumnDayHours` (допустимый диапазон 8–14 часов).
- По завершении цикла (`GrowingPeriod::Completed`) свет выключен постоянно.

## Тестирование

Используйте `light_control_test.ino`. Команды аналогичны тесту `growing_cycle`: установка дня, часа, произвольного количества секунд и вывод текущего состояния света. Скетч содержит сценарии проверки прямой и инверсной логики управления пином.

---

# Модуль управления поливом (`watering`)

## Назначение

Финитный автомат, управляющий автоматическим поливом, ручным поливом, очисткой и режимом тренировки. Работает без `delay()`, использует `Timer` и настройки из `Settings::Manager`.

## API

```cpp
class Irrigation::Watering {
public:
    static void begin(uint8_t pinPump, bool highIsOn = true);
    static WateringState process(uint16_t soilMoisturePercent);
    static void setManualWatering();
    static void setManualCleaning();
    static void setTraining();
    static void setIdle();
    static void stop();
    static WateringState getState();
    static inline bool isPumpOn();
    static uint32_t getRemainingSeconds();
};
```

- `begin(pinPump, highIsOn)` — настраивает пин помпы, задаёт уровень активного сигнала (HIGH по умолчанию включает помпу) и переводит FSM в состояние `Stopping`.
- `process` — вызывается в основном цикле, обновляет FSM и возвращает текущее состояние.
- `setManualWatering` / `setManualCleaning` / `setTraining` / `setIdle` / `stop` — полный набор внешних команд.
- `getState` — возвращает текущее состояние; `isPumpOn` помогает при тестировании и индикации; `getRemainingSeconds` показывает, сколько секунд осталось до завершения текущего таймера (0, если таймер не активен).

## Состояния FSM

- `Stopping` — принудительная остановка.
- `Idle` — ожидание, отслеживание влажности.
- `AutoPause` ↔ `AutoWatering` — автоматические попытки полива согласно настройкам (`WateringDurationSec`, `WateringPauseSec`, `WateringMaxAttempts`).
- `ManualWatering` — ручной полив, прерывает автоматический цикл.
- `ManualCleaning` ↔ `ManualPause` — циклы очистки помпы (`CleaningDurationSec`, `CleaningPauseSec`, `CleaningCycles`).
- `TrainingWaiting` — ожидание снижения влажности до `TrainingMoisturePercent`.
- `TrainingReady` — суточный таймер ожидания действий пользователя (86400 секунд).
- `OutOfWater` — ошибка отсутствия воды, автоматически сбрасывается через 24 часа или внешними командами `stop`, `setManualWatering`, `setManualCleaning`.

Полное описание переходов и служебных переменных — в `fsm_description.md`.

## Взаимодействие

- Использует `Timer` для всех таймеров и пауз.
- Значения порогов и длительностей берёт из `Settings::Manager` (валидация выполняется там же).
- Логика оповещений, управление кнопками и отображение статуса реализуются во внешних модулях.

## Тестирование

- Скетч `watering_test.ino` моделирует сценарии: автоматический полив, достижение `OutOfWater`, ручной полив и очистка, режим тренировки, принудительные команды, а также проверяет инверсию сигнала управления помпой.
- Для симуляции влажности используется программная подстановка значений и управление через Serial.
