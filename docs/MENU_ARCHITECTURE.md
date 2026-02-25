# Архитектура меню настроек

## 1. Обзор

Модуль меню предоставляет иерархический интерфейс для редактирования настроек системы на OLED-дисплее 128×64. Архитектура оптимизирована для Arduino Nano с учётом ограничений памяти (2 КБ RAM, 32 КБ Flash).

### Принципы проектирования

1. **Данные в PROGMEM** — структура меню, названия пунктов хранятся во Flash
2. **Минимум RAM** — только текущее состояние навигации (несколько байт)
3. **Декларативное описание** — меню описывается таблицами, не кодом
4. **Единый рендерер** — одна функция отрисовки для всех типов пунктов
5. **Интеграция с Settings** — прямое чтение/запись через `Settings::Manager`

---

## 2. Структура меню

### Иерархия пунктов

```
ГЛАВНОЕ МЕНЮ (8 пунктов)
│
│   ┌─────────────────────────────────────────────────────────────────┐
│   │ Спецсимволы добавляются программно в зависимости от типа:       │
│   │   "►" — Action (действие)                                       │
│   │   "*" — Submenu (подменю с настройками)                         │
│   │ Это позволяет использовать одну строку для разных типов пунктов │
│   └─────────────────────────────────────────────────────────────────┘
│
├── ► ТРЕНИРОВКА  [Action] → SystemController::setTraining()
├── ► ОЧИСТКА     [Action] → SystemController::setManualCleaning()
│
├── * ЦИКЛ (7 параметров)
│   ├── Режим        SystemMode         [РОСТ/ВЕСНА/ЛЕТО/ОСЕНЬ]
│   ├── День         CurrentDay         [1-200] *виртуальный
│   ├── Час          CurrentHour        [00:00-23:50] шаг 10 мин *виртуальный
│   ├── Росток       GerminationLengthDays [3-7]
│   ├── Весна        SpringLengthDays   [0-197]
│   ├── Лето         SummerLengthDays   [0-197]
│   └── Осень        AutumnLengthDays   [0-197]
│
├── * СВЕТ (3 параметра)
│   ├── Весна        SpringDayHours     [19-22]
│   ├── Лето         SummerDayHours     [15-18]
│   └── Осень        AutumnDayHours     [8-14]
│
├── * ПОЛИВ (6 параметров)
│   ├── Время        WateringDurationSec [1-120] сек
│   ├── Пауза        WateringPauseSec   [60-3600] сек → отображать мин
│   ├── Мин.%        SoilMoistureStartPercent [0-99]
│   ├── Макс.%       SoilMoistureStopPercent  [1-100]
│   ├── Попыток      WateringMaxAttempts [1-10]
│   └── Тренир.%     TrainingMoisturePercent [0..(StartPercent-1)] *динамический max
│
├── * ОЧИСТКА (3 параметра)
│   ├── Время        CleaningDurationSec [1-300] сек
│   ├── Циклов       CleaningCycles     [1-10]
│   └── Пауза        CleaningPauseSec   [10-60] сек
│
├── * ДАТЧИК (3 параметра)
│   ├── Период       SensorCheckPeriodSec [60-3600] сек → отображать мин
│   ├── Сухо         SoilCalibrationDry [0-1023] *с live-значением
│   └── Влажно       SoilCalibrationWet [0-1023] *с live-значением
│
└── ► СБРОС → подтверждение → Settings::resetToDefaults()
```

**Итого:** 8 пунктов в корне, 22 редактируемых параметра + 3 действия (тренировка, очистка, сброс).

### Оптимизация строк

Строка `"ОЧИСТКА"` используется дважды: для Action и для Submenu. Символ-префикс ("►" или "*") добавляется программно при отрисовке в зависимости от `MenuItemType`.

---

## 3. Архитектура данных

### 3.1 Типы пунктов меню

```cpp
enum class MenuItemType : uint8_t {
    Submenu,    // Переход в подменю
    Value,      // Редактируемое значение (Settings::Key)
    Action      // Действие (сброс настроек, и т.д.)
};
```

### 3.2 Структура пункта меню (PROGMEM)

```cpp
struct MenuItem {
    const char* label;        // Название пункта (PROGMEM строка)
    MenuItemType type;        // Тип пункта
    uint16_t data;            // Зависит от типа:
                              //   Submenu: индекс подменю в SUBMENUS[]
                              //   Value: Settings::Key (может быть >255)
                              //   Action: код действия
};
```

**Размер:** 5 байт на пункт (указатель 2 байта + тип 1 байт + данные 2 байта).

### 3.3 Структура подменю (PROGMEM)

```cpp
struct Submenu {
    const char* title;        // Заголовок подменю (PROGMEM строка)
    const MenuItem* items;    // Массив пунктов (PROGMEM)
    uint8_t itemCount;        // Количество пунктов
};
```

### 3.4 Описание параметра для редактирования (PROGMEM)

```cpp
struct ParamDescriptor {
    Settings::Key key;        // Ключ настройки
    int16_t minValue;         // Минимальное значение
    int16_t maxValue;         // Максимальное значение
    uint8_t step;             // Шаг изменения (1, 5, 10, 60...)
    uint8_t displayMode : 4;  // Режим отображения (см. ниже)
    uint8_t hasLiveValue : 1; // Показывать live-значение с датчика
    uint8_t reserved : 3;     // Резерв для будущих флагов
};

enum DisplayMode : uint8_t {
    AsNumber,     // Число как есть
    AsMinutes,    // Секунды → минуты (value / 60)
    AsPercent,    // С символом %
    AsEnum,       // Из списка строк (для SystemMode)
    AsTimeHHMM    // Время в формате HH:MM (value = минуты от 00:00)
};
```

**Примечание:** `hasLiveValue = 1` используется для `SoilCalibrationDry` и `SoilCalibrationWet`. При редактировании этих параметров меню запрашивает текущее значение с датчика через callback.

---

## 4. Состояние навигации (RAM)

```cpp
struct MenuState {
    uint8_t level;            // 0 = главное меню, 1 = подменю
    uint8_t menuIndex;        // Индекс текущего подменю (для level=1)
    uint8_t selectedItem;     // Индекс выбранного пункта
    uint8_t scrollOffset;     // Смещение прокрутки (для списков > 5 пунктов)
    bool editing;             // Режим редактирования значения
    int16_t editValue;        // Временное значение при редактировании
};
```

**Размер:** ~8 байт RAM.

---

## 5. Отображение на дисплее

### 5.1 Макет экрана меню

```
      X: 0          32          64          96         128
         │           │           │           │           │
     Y=0 ┌───────────────────────────────────────────────┐
         │ ▌ЗАГОЛОВОК МЕНЮ                     (инверт)  │ Высота: 10px
    Y=10 ├───────────────────────────────────────────────┤
    Y=12 │   Пункт 1                                     │ Строка 1
    Y=21 │ ► Пункт 2 (выбран)                   [знач]   │ Строка 2 (инверсия)
    Y=30 │   Пункт 3                            [знач]   │ Строка 3
    Y=39 │   Пункт 4                            [знач]   │ Строка 4
    Y=48 │   Пункт 5                            [знач]   │ Строка 5
    Y=54 ├───────────────────────────────────────────────┤
    Y=56 │ [OK]      [▲]       [▼]       [←]             │ Иконки: 4×32px
    Y=64 └───────────────────────────────────────────────┘
```

**Размеры:**
- Заголовок: Y=0..9 (10 px), шрифт 6×8
- Область списка: Y=10..53 (44 px), 5 строк × ~9 px
- Нижняя панель: Y=54..63 (10 px), иконки 8×8 + текст
- Колонка значений: X=90..127 (~38 px, для чисел/текста)

- **Заголовок:** инвертированная строка с названием текущего меню/подменю
- **Список:** до 5 видимых пунктов, прокрутка при необходимости
- **Выделение:** инвертированная строка для выбранного пункта
- **Значения:** справа от названия (для Value-пунктов)
- **Нижняя панель:** иконки-подсказки для кнопок

### 5.2 Макет режима редактирования

```
      X: 0          32          64          96         128
         │           │           │           │           │
     Y=0 ┌───────────────────────────────────────────────┐
         │ ▌РЕДАКТИРОВАНИЕ                     (инверт)  │ Заголовок
    Y=10 ├───────────────────────────────────────────────┤
    Y=14 │                                               │
    Y=18 │         Название параметра                    │ Шрифт 6×8, центр
    Y=26 │                                               │
    Y=30 │            ◄ [ЗНАЧЕНИЕ] ►                     │ Шрифт 10×16, центр
    Y=42 │                                               │
    Y=46 │         min: XX    max: YY                    │ Шрифт 5×7, центр
    Y=54 ├───────────────────────────────────────────────┤
    Y=56 │ [OK]      [+]       [-]       [←]             │ Иконки кнопок
    Y=64 └───────────────────────────────────────────────┘
```

**Размеры:**
- Название параметра: Y=18, шрифт 6×8
- Значение: Y=30..42 (12 px), шрифт 10×16 или 8×12
- Границы min/max: Y=46, шрифт 5×7
- Нижняя панель: Y=56..63

- При редактировании отображается текущее значение крупным шрифтом
- Показаны допустимые границы (min/max)
- Стрелки ◄ ► указывают на возможность изменения

### 5.3 Макет редактирования с live-значением (калибровка датчика)

Для параметров `SoilCalibrationDry` и `SoilCalibrationWet` отображается текущее "сырое" значение с датчика почвы:

```
      X: 0          32          64          96         128
         │           │           │           │           │
     Y=0 ┌───────────────────────────────────────────────┐
         │ ▌РЕДАКТИРОВАНИЕ                     (инверт)  │
    Y=10 ├───────────────────────────────────────────────┤
    Y=14 │         Сухо (калибровка)                     │ Название параметра
    Y=24 │                                               │
    Y=26 │            ◄ [750] ►                          │ Редактируемое (крупный)
    Y=38 │                                               │
    Y=40 │      Текущее: 623                             │ Live-значение (мелкий)
    Y=48 │         min: 0    max: 1023                   │ Границы
    Y=54 ├───────────────────────────────────────────────┤
    Y=56 │ [OK]      [+]       [-]       [←]             │
    Y=64 └───────────────────────────────────────────────┘
```

**Размеры:**
- Название: Y=14, шрифт 6×8
- Значение: Y=26..36 (10 px), шрифт 8×12
- Live-значение: Y=40, шрифт 6×8 (выделено)
- Границы: Y=48, шрифт 5×7

**Реализация:**
- В `ParamDescriptor` добавляется флаг `hasLiveValue`
- При `hasLiveValue == true` меню вызывает callback для получения текущего значения
- Live-значение обновляется при каждой перерисовке экрана

### 5.4 Режимы отображения значений

| DisplayMode | Описание | Пример |
|-------------|----------|--------|
| `AsNumber` | Число как есть | `45`, `120` |
| `AsMinutes` | Секунды → минуты | `3600` → `60 мин` |
| `AsPercent` | С символом % | `75%` |
| `AsEnum` | Из списка строк | `РОСТ`, `ВЕСНА` |
| `AsTimeHHMM` | Время в формате HH:MM | `150` → `02:30` |

**AsTimeHHMM:** Значение хранится как минуты от 00:00 (0–1430). Отображается в формате `HH:MM`:
```cpp
uint8_t hours = value / 60;
uint8_t mins = value % 60;
sprintf_P(buf, PSTR("%02d:%02d"), hours, mins);
```

---

## 6. Навигация и управление

### 6.1 Кнопки

| Кнопка | Пин | В меню | При редактировании |
|--------|-----|--------|-------------------|
| OK     | D2  | Войти / Выбрать | Сохранить |
| UP     | D5  | Вверх по списку | Увеличить значение |
| DOWN   | D6  | Вниз по списку | Уменьшить значение |
| CANCEL | D7  | Назад / Выход | Отмена без сохранения |

### 6.2 Логика навигации

```
ГЛАВНОЕ МЕНЮ
    │
    ├─ OK на Submenu-пункте → вход в подменю (level=1)
    ├─ OK на Action-пункте → выполнение действия
    ├─ CANCEL → выход из меню (возврат к MainScreen)
    │
ПОДМЕНЮ
    │
    ├─ OK на Value-пункте → режим редактирования (editing=true)
    ├─ CANCEL → возврат в главное меню (level=0)
    │
РЕДАКТИРОВАНИЕ
    │
    ├─ UP/DOWN → изменение editValue (с учётом step, min, max)
    ├─ OK → сохранение в Settings::Manager::set(), выход из редактирования
    ├─ CANCEL → отмена, editValue отбрасывается
```

### 6.3 Ускорение при удержании

- При удержании UP/DOWN более 500 мс — автоповтор с периодом 100 мс
- После 2 секунд удержания — увеличенный шаг (step × 10)

---

## 7. API модуля Menu

### 7.1 Заголовочный файл `src/menu/menu.h`

```cpp
namespace Menu {

class Manager {
public:
    Manager() = delete;
    
    /**
     * Инициализация меню.
     * @param display Ссылка на объект U8G2
     */
    static void begin(U8G2& display);
    
    /**
     * Открыть меню (вызывается при нажатии кнопки Settings).
     */
    static void open();
    
    /**
     * Закрыть меню и вернуться к основному экрану.
     */
    static void close();
    
    /**
     * Проверить, открыто ли меню.
     */
    static bool isOpen();
    
    /**
     * Обновление состояния меню с текущими состояниями кнопок.
     * Вызывается из loop() когда меню открыто.
     * @param stateOK     Состояние кнопки OK
     * @param stateUp     Состояние кнопки UP
     * @param stateDown   Состояние кнопки DOWN
     * @param stateCancel Состояние кнопки CANCEL
     * Значения: BUTTON_UNPRESSED / BUTTON_PRESSED / BUTTON_LONGPRESSED
     */
    static void update(uint8_t stateOK, uint8_t stateUp, uint8_t stateDown, uint8_t stateCancel);
    
    /**
     * Отрисовка текущего состояния меню.
     * Вызывается в loop() когда isOpen() == true.
     */
    static void render();

    /**
     * Устанавливает callback для получения live-значения датчика почвы.
     */
    static void setLiveValueCallback(int16_t (*callback)());

private:
    static void _renderMainMenu();
    static void _renderSubmenu();
    static void _renderEditScreen();
    static void _navigateUp();
    static void _navigateDown();
    static void _selectItem();
    static void _goBack();
    static void _changeValue(int8_t delta);
    static void _saveValue();
    static void _processEditButtons(uint8_t stateUp, uint8_t stateDown);
    
    static U8G2* _display;
    static MenuState _state;
    static bool _isOpen;
    
    // Автоповтор кнопок
    static Timer _repeatTimer;
    static uint8_t _heldButtonIdx;    // 0xFF = нет удержания
    static uint8_t _repeatCount;      // Счётчик для ускорения
    
    // Callback для live-значения
    static int16_t (*_liveValueCallback)();
};

} // namespace Menu
```

### 7.2 Интеграция в главный цикл

**Важно:** `SystemController::process()` вызывается **всегда**, независимо от того, открыто меню или нет. Это гарантирует, что полив, управление светом и другие критические функции продолжают работать во время настройки параметров пользователем.

Подробный пример интеграции с использованием модуля `Button` см. в разделе 11.7.

---

## 8. Хранение данных в PROGMEM

### 8.1 Строки меню

```cpp
// Названия разделов главного меню (используются для Action и Submenu)
const char STR_MENU_TRAIN[] PROGMEM = "ТРЕНИРОВКА";  // Action: запуск тренировки
const char STR_MENU_CLEAN[] PROGMEM = "ОЧИСТКА";     // Action: запуск очистки / Submenu: настройки
const char STR_MENU_CYCLE[] PROGMEM = "ЦИКЛ";
const char STR_MENU_LIGHT[] PROGMEM = "СВЕТ";
const char STR_MENU_WATER[] PROGMEM = "ПОЛИВ";
const char STR_MENU_SENSOR[] PROGMEM = "ДАТЧИК";
const char STR_MENU_RESET[] PROGMEM = "СБРОС";

// Названия параметров (примеры)
const char STR_PARAM_MODE[] PROGMEM = "Режим";
const char STR_PARAM_DAY[] PROGMEM = "День";
const char STR_PARAM_HOUR[] PROGMEM = "Час";
const char STR_PARAM_GERM[] PROGMEM = "Росток";
const char STR_PARAM_TRAIN_PCT[] PROGMEM = "Тренир.%";
// ... и т.д.
```

### 8.2 Структура главного меню

```cpp
// Коды действий для Action-пунктов
enum class ActionCode : uint8_t {
    StartTraining = 0,   // SystemController::setTraining()
    StartCleaning = 1,   // SystemController::setManualCleaning()
    ResetSettings = 2    // Settings::resetToDefaults()
};

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
```

**Примечание:** Строка `STR_MENU_CLEAN` ("ОЧИСТКА") используется дважды — для Action и Submenu. Различие видно пользователю благодаря префиксам "►" и "*".

### 8.3 Структуры подменю

```cpp
// Подменю "ЦИКЛ"
const MenuItem CYCLE_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_MODE,   MenuItemType::Value, static_cast<uint16_t>(Key::SystemMode) },
    { STR_PARAM_DAY,    MenuItemType::Value, static_cast<uint16_t>(Key::CurrentDay) },
    { STR_PARAM_HOUR,   MenuItemType::Value, static_cast<uint16_t>(Key::CurrentHour) },
    { STR_PARAM_GERM,   MenuItemType::Value, static_cast<uint16_t>(Key::GerminationLengthDays) },
    { STR_PARAM_SPRING, MenuItemType::Value, static_cast<uint16_t>(Key::SpringLengthDays) },
    { STR_PARAM_SUMMER, MenuItemType::Value, static_cast<uint16_t>(Key::SummerLengthDays) },
    { STR_PARAM_AUTUMN, MenuItemType::Value, static_cast<uint16_t>(Key::AutumnLengthDays) }
};

// Подменю "ПОЛИВ" — включает параметр тренировки
const MenuItem WATER_SUBMENU_ITEMS[] PROGMEM = {
    { STR_PARAM_DURATION,  MenuItemType::Value, static_cast<uint16_t>(Key::WateringDurationSec) },
    { STR_PARAM_PAUSE,     MenuItemType::Value, static_cast<uint16_t>(Key::WateringPauseSec) },
    { STR_PARAM_MIN_PCT,   MenuItemType::Value, static_cast<uint16_t>(Key::SoilMoistureStartPercent) },
    { STR_PARAM_MAX_PCT,   MenuItemType::Value, static_cast<uint16_t>(Key::SoilMoistureStopPercent) },
    { STR_PARAM_ATTEMPTS,  MenuItemType::Value, static_cast<uint16_t>(Key::WateringMaxAttempts) },
    { STR_PARAM_TRAIN_PCT, MenuItemType::Value, static_cast<uint16_t>(Key::TrainingMoisturePercent) }
};

// Массив всех подменю (5 подменю, без ТРЕНИРОВКА)
const Submenu SUBMENUS[] PROGMEM = {
    { STR_MENU_CYCLE,  CYCLE_SUBMENU_ITEMS,  7 },
    { STR_MENU_LIGHT,  LIGHT_SUBMENU_ITEMS,  3 },
    { STR_MENU_WATER,  WATER_SUBMENU_ITEMS,  6 },  // +1 параметр TrainingMoisturePercent
    { STR_MENU_CLEAN,  CLEAN_SUBMENU_ITEMS,  3 },
    { STR_MENU_SENSOR, SENSOR_SUBMENU_ITEMS, 3 }
};
```

### 8.4 Дескрипторы параметров

```cpp
const ParamDescriptor PARAM_DESCRIPTORS[] PROGMEM = {
    // Key                          min    max   step  display         live
    { Key::SystemMode,                0,     3,    1,  DisplayMode::AsEnum,     0 },
    { Key::CurrentDay,                1,   200,    1,  DisplayMode::AsNumber,   0 },  // *виртуальный
    { Key::CurrentHour,               0,  1430,   10,  DisplayMode::AsTimeHHMM, 0 },  // *виртуальный
    { Key::GerminationLengthDays,     3,     7,    1,  DisplayMode::AsNumber,   0 },
    { Key::SpringLengthDays,          0,   197,    1,  DisplayMode::AsNumber,   0 },
    { Key::SummerLengthDays,          0,   197,    1,  DisplayMode::AsNumber,   0 },
    { Key::AutumnLengthDays,          0,   197,    1,  DisplayMode::AsNumber,   0 },
    { Key::SpringDayHours,           19,    22,    1,  DisplayMode::AsNumber,   0 },
    { Key::SummerDayHours,           15,    18,    1,  DisplayMode::AsNumber,   0 },
    { Key::AutumnDayHours,            8,    14,    1,  DisplayMode::AsNumber,   0 },
    { Key::WateringDurationSec,       1,   120,    1,  DisplayMode::AsNumber,   0 },
    { Key::WateringPauseSec,         60,  3600,   60,  DisplayMode::AsMinutes,  0 },
    { Key::SoilMoistureStartPercent,  0,    99,    1,  DisplayMode::AsPercent,  0 },  // max динамич.
    { Key::SoilMoistureStopPercent,   1,   100,    1,  DisplayMode::AsPercent,  0 },  // min динамич.
    { Key::WateringMaxAttempts,       1,    10,    1,  DisplayMode::AsNumber,   0 },
    { Key::TrainingMoisturePercent,   0,    99,    1,  DisplayMode::AsPercent,  0 },  // max динамич.
    { Key::CleaningDurationSec,       1,   300,    5,  DisplayMode::AsNumber,   0 },
    { Key::CleaningCycles,            1,    10,    1,  DisplayMode::AsNumber,   0 },
    { Key::CleaningPauseSec,         10,    60,    5,  DisplayMode::AsNumber,   0 },
    { Key::SensorCheckPeriodSec,     60,  3600,   60,  DisplayMode::AsMinutes,  0 },
    { Key::SoilCalibrationDry,        0,  1023,   10,  DisplayMode::AsNumber,   1 },  // live-значение!
    { Key::SoilCalibrationWet,        0,  1023,   10,  DisplayMode::AsNumber,   1 }   // live-значение!
};

// Примечание: hasLiveValue=1 для SoilCalibrationDry и SoilCalibrationWet
// При редактировании этих параметров отображается текущее "сырое" значение с датчика почвы
```

---

## 9. Оценка потребления памяти

### Flash (PROGMEM)

| Компонент | Размер |
|-----------|--------|
| Строки меню (~25 строк × ~10 символов) | ~250 байт |
| MAIN_MENU_ITEMS (8 × 5 байт) | 40 байт |
| Подменю (22 пункта × 5 байт) | 110 байт |
| SUBMENUS (5 × 5 байт) | 25 байт |
| PARAM_DESCRIPTORS (22 × 7 байт) | 154 байт |
| Код модуля Menu | ~1700 байт |
| **Итого** | **~2300 байт** |

### RAM

| Компонент | Размер |
|-----------|--------|
| MenuState | 8 байт |
| Указатель на display | 2 байта |
| Флаг isOpen | 1 байт |
| Timer _repeatTimer | 8 байт |
| _heldButtonIdx + _repeatCount | 2 байта |
| Указатель на callback | 2 байта |
| Буфер для строки (временный) | 16 байт |
| **Итого** | **~39 байт** |

---

## 10. Валидация при редактировании

### Принцип: валидация в реальном времени

Меню **не позволяет** пользователю установить недопустимое значение. Проверка границ выполняется **при каждом нажатии UP/DOWN**, а не только при сохранении:

```cpp
void Menu::Manager::_changeValue(int8_t direction) {
    // Получаем дескриптор с учётом динамических ограничений
    ParamDescriptor desc;
    _getParamDescriptor(_currentKey, desc);  // Вычисляет min/max
    
    int16_t newValue = _state.editValue + (direction * desc.step);
    
    // Ограничиваем значение допустимыми границами
    if (newValue < desc.minValue) newValue = desc.minValue;
    if (newValue > desc.maxValue) newValue = desc.maxValue;
    
    _state.editValue = newValue;
}
```

### Статические и динамические ограничения

**Статические** — задаются в `PARAM_DESCRIPTORS[]` и не меняются:
- `WateringDurationSec: [1..120]`
- `SpringDayHours: [19..22]`
- и т.д.

**Динамические** — вычисляются в момент редактирования на основе текущих настроек:

| Параметр | Ограничение | Зависимость |
|----------|-------------|-------------|
| `TrainingMoisturePercent` | max = `SoilMoistureStartPercent - 1` | Меньше порога старта полива |
| `SoilMoistureStartPercent` | max = `SoilMoistureStopPercent - 1` | Меньше порога остановки |
| `SoilMoistureStopPercent` | min = `SoilMoistureStartPercent + 1` | Больше порога старта |

### Реализация динамических ограничений

```cpp
void Menu::Manager::_getParamDescriptor(Settings::Key key, ParamDescriptor& out) {
    // Копируем базовые значения из PROGMEM
    memcpy_P(&out, &PARAM_DESCRIPTORS[keyIndex], sizeof(ParamDescriptor));
    
    // Применяем динамические ограничения
    switch (key) {
        case Key::TrainingMoisturePercent:
            // Влажность тренировки всегда < порога старта полива
            out.maxValue = Settings::Manager::get(Key::SoilMoistureStartPercent) - 1;
            if (out.maxValue < 0) out.maxValue = 0;
            break;
            
        case Key::SoilMoistureStartPercent:
            // Порог старта < порог остановки
            out.maxValue = Settings::Manager::get(Key::SoilMoistureStopPercent) - 1;
            break;
            
        case Key::SoilMoistureStopPercent:
            // Порог остановки > порог старта
            out.minValue = Settings::Manager::get(Key::SoilMoistureStartPercent) + 1;
            break;
    }
}
```

### Взаимосвязанные параметры (информационно)

При редактировании связанных параметров пользователю может быть недоступен полный диапазон значений до тех пор, пока не изменены другие связанные параметры. Например:

- Если `SoilMoistureStartPercent = 60`, то `TrainingMoisturePercent` можно установить только в диапазоне `[0..59]`
- Чтобы установить `TrainingMoisturePercent = 70`, сначала нужно увеличить `SoilMoistureStartPercent` до 71+

---

## 11. Интеграция с системой (FSM-архитектура)

### 11.1 Принцип: всё есть FSM

Меню — это **конечный автомат**, интегрированный в основной цикл без блокировок:

```
┌─────────────────────────────────────────────────────────────────────┐
│                          MAIN LOOP                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│   SystemController::process()  ← ВСЕГДА выполняется                 │
│        │                                                            │
│        ├─ Watering::process()      [FSM полива]                     │
│        ├─ LightControl::update()   [FSM освещения]                  │
│        ├─ SoilSensor (внутри)      [FSM датчика]                    │
│        └─ AirSensor (внутри)       [FSM датчика]                    │
│                                                                     │
│   if (Menu::Manager::isOpen()) {                                    │
│       Menu::Manager::handleButton()  ← кнопки → меню                │
│       Menu::Manager::render()        ← отрисовка меню               │
│   } else {                                                          │
│       MainScreen::update()           ← основной экран               │
│       handleMainScreenButtons()      ← кнопки → основной режим      │
│   }                                                                 │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 11.2 Состояния FSM меню

```cpp
enum class MenuFSMState : uint8_t {
    Closed,         // Меню закрыто, управление у MainScreen
    MainMenu,       // Главное меню отображается
    Submenu,        // Открыто подменю
    Editing,        // Режим редактирования параметра
    Confirmation    // Диалог подтверждения (для Action "СБРОС")
};
```

**Переходы:**
```
Closed ──[OK]──► MainMenu
MainMenu ──[OK на Submenu]──► Submenu
MainMenu ──[OK на Action]──► выполнение / Confirmation
MainMenu ──[CANCEL]──► Closed
Submenu ──[OK на Value]──► Editing
Submenu ──[CANCEL]──► MainMenu
Editing ──[OK]──► Submenu (сохранение)
Editing ──[CANCEL]──► Submenu (отмена)
```

### 11.3 Взаимодействие с другими модулями

| Модуль | Тип взаимодействия | API |
|--------|-------------------|-----|
| **Settings::Manager** | Чтение/запись настроек | `get(Key)`, `set(Key, value)` |
| **SystemController** | Выполнение Action | `setTraining()`, `setManualCleaning()`, `stopWatering()` |
| **SoilSensor** | Live-значение при калибровке | `getRawSensorData(true)` — принудительное чтение |
| **Timer** | Автоповтор кнопок, таймауты | Существующий класс `Timer` |
| **U8G2** | Отрисовка | Указатель передаётся в `begin()` |

### 11.4 Получение live-значения датчика

При редактировании `SoilCalibrationDry` / `SoilCalibrationWet` меню запрашивает текущее "сырое" значение датчика **через callback** (см. раздел 11.6).

**Добавить в SystemController:**

```cpp
// system_controller.h
static int16_t getSoilRawValue(bool forceCheck = false);

// system_controller.cpp
int16_t SystemController::getSoilRawValue(bool forceCheck) {
    if (!_soilSensor) return 0;
    return static_cast<int16_t>(_soilSensor->getRawSensorData(forceCheck));
}
```

**Почему callback, а не прямой вызов:**
- Меню не зависит напрямую от SystemController/SoilSensor
- Callback регистрируется при инициализации (dependency injection)
- Легко подменить для тестирования

### 11.5 Работа с кнопками

#### Принципы

1. **Изоляция:** Меню не взаимодействует с `Button` напрямую — получает состояния через параметры
2. **Оптимальность:** Минимум RAM, используем существующие модули
3. **Защита:** Корректная обработка нештатных ситуаций (две кнопки одновременно)

#### Существующий модуль Button

Модуль `src/modules/button/` предоставляет состояния:

```cpp
#define BUTTON_UNPRESSED      0  // Кнопка отпущена
#define BUTTON_PRESSED        1  // Кнопка нажата (момент нажатия)
#define BUTTON_LONGPRESSED    2  // Удержание > 500 мс (однократно!)
```

**Важно:** `BUTTON_LONGPRESSED` срабатывает **один раз** после 500 мс, затем состояние остаётся `BUTTON_LONGPRESSED` до отпускания.

#### Идентификация кнопок

Используем индексы 0-3, соответствующие порядку передачи состояний:

```cpp
enum ButtonIndex : uint8_t {
    BTN_IDX_OK     = 0,
    BTN_IDX_UP     = 1,
    BTN_IDX_DOWN   = 2,
    BTN_IDX_CANCEL = 3
};
```

#### Автоповтор при удержании

```cpp
class Menu::Manager {
private:
    static Timer _repeatTimer;            // Существующий класс Timer
    static uint8_t _heldButtonIdx;        // Индекс удерживаемой кнопки (0xFF = нет)
    static uint8_t _repeatCount;          // Счётчик повторов (для ускорения)
    
    static constexpr uint8_t NO_BUTTON = 0xFF;
    static constexpr unsigned long REPEAT_PERIOD_MS = 100;
    static constexpr uint8_t FAST_AFTER_REPEATS = 15;  // Ускорение после 15 повторов (~1.5 сек)
};
```

**Счётчик вместо millis():** После 15 повторов (15 × 100 мс = 1.5 сек) включается ускорение (step × 10).

#### API меню

```cpp
/**
 * Обновление состояния меню. Вызывается из loop() с текущими состояниями кнопок.
 * @param stateOK     Состояние кнопки OK (BUTTON_UNPRESSED / PRESSED / LONGPRESSED)
 * @param stateUp     Состояние кнопки UP
 * @param stateDown   Состояние кнопки DOWN
 * @param stateCancel Состояние кнопки CANCEL
 */
static void update(uint8_t stateOK, uint8_t stateUp, uint8_t stateDown, uint8_t stateCancel);
```

#### Логика обработки (режим редактирования)

```cpp
void Menu::Manager::_processEditButtons(uint8_t stateUp, uint8_t stateDown) {
    // Определяем активную кнопку (UP или DOWN)
    uint8_t activeIdx = NO_BUTTON;
    uint8_t activeState = BUTTON_UNPRESSED;
    int8_t direction = 0;
    
    if (stateUp > BUTTON_UNPRESSED) {
        activeIdx = BTN_IDX_UP;
        activeState = stateUp;
        direction = 1;
    } else if (stateDown > BUTTON_UNPRESSED) {
        activeIdx = BTN_IDX_DOWN;
        activeState = stateDown;
        direction = -1;
    }
    
    // === Защита от двух кнопок ===
    // Если удерживается одна кнопка, а нажата другая — игнорируем вторую
    if (_heldButtonIdx != NO_BUTTON && activeIdx != _heldButtonIdx) {
        // Проверяем, отпущена ли удерживаемая кнопка
        uint8_t heldState = (_heldButtonIdx == BTN_IDX_UP) ? stateUp : stateDown;
        if (heldState == BUTTON_UNPRESSED) {
            // Удерживаемая отпущена — сбрасываем
            _heldButtonIdx = NO_BUTTON;
            _repeatCount = 0;
        } else {
            return;  // Игнорируем вторую кнопку
        }
    }
    
    // === Обработка событий ===
    if (activeState == BUTTON_PRESSED && _heldButtonIdx == NO_BUTTON) {
        // Момент нажатия — однократное изменение
        _changeValue(direction);
        _heldButtonIdx = activeIdx;
        _repeatCount = 0;
    }
    else if (activeState == BUTTON_LONGPRESSED && _heldButtonIdx == activeIdx) {
        // Начало удержания — запускаем таймер
        _repeatTimer.setPeriod(REPEAT_PERIOD_MS);  // setPeriod с dropTimer=true по умолчанию
    }
    else if (activeIdx == NO_BUTTON && _heldButtonIdx != NO_BUTTON) {
        // Обе кнопки отпущены — сброс
        _heldButtonIdx = NO_BUTTON;
        _repeatCount = 0;
    }
    
    // === Автоповтор ===
    if (_heldButtonIdx != NO_BUTTON && _repeatTimer.isReady()) {
        _repeatCount++;
        int8_t dir = (_heldButtonIdx == BTN_IDX_UP) ? 1 : -1;
        int8_t multiplier = (_repeatCount > FAST_AFTER_REPEATS) ? 10 : 1;
        _changeValue(dir * multiplier);
    }
}
```

**Защита от двух кнопок:**
- Пока первая кнопка удерживается, вторая игнорируется
- Переключение на вторую возможно только после отпускания первой
- Это предотвращает непредсказуемое поведение

### 11.6 Callback для live-значения датчика

Для параметров калибровки (`SoilCalibrationDry`, `SoilCalibrationWet`) с флагом `hasLiveValue == true` меню запрашивает текущее значение через callback.

**Регистрация (в setup):**

```cpp
void setup() {
    Menu::Manager::begin(display);
    
    // Callback: возвращает сырое значение датчика почвы
    Menu::Manager::setLiveValueCallback([]() -> int16_t {
        return Irrigation::SystemController::getSoilRawValue();
    });
}
```

**Использование при отрисовке:**

```cpp
void Menu::Manager::_renderEditScreen() {
    // ...
    if (_currentParamHasLiveValue() && _liveValueCallback) {
        int16_t liveValue = _liveValueCallback();
        // Отображаем: "Текущее: XXX"
    }
}
```

**Преимущества callback:**
- Меню не зависит от `SystemController` / `SoilSensor`
- Легко подменить для тестирования
- Dependency injection через функцию

### 11.7 Порядок вызовов в loop()

Меню **не взаимодействует** с объектами `Button` напрямую. В `loop()` состояния кнопок считываются один раз и передаются в меню:

```cpp
// Объекты кнопок (глобальные или в main.ino)
Button btnOK(PIN_BTN_OK);
Button btnUp(PIN_BTN_UP);
Button btnDown(PIN_BTN_DOWN);
Button btnCancel(PIN_BTN_CANCEL);

void loop() {
    // 1. Опрос кнопок (FSM антидребезга) — один раз за итерацию
    btnOK.check();
    btnUp.check();
    btnDown.check();
    btnCancel.check();
    
    // 2. Критические процессы (ВСЕГДА, независимо от UI)
    Irrigation::SystemController::process();
    
    // 3. Считываем состояния один раз
    uint8_t sOK     = btnOK.getState();
    uint8_t sUp     = btnUp.getState();
    uint8_t sDown   = btnDown.getState();
    uint8_t sCancel = btnCancel.getState();
    
    // 4. UI (в зависимости от состояния меню)
    if (Menu::Manager::isOpen()) {
        // Передаём состояния в меню (один вызов)
        Menu::Manager::update(sOK, sUp, sDown, sCancel);
        Menu::Manager::render();
    } else {
        // Основной экран
        Display::MainScreen::update();
        
        // Кнопки основного режима
        if (sOK == BUTTON_PRESSED) {
            Menu::Manager::open();
        }
        // ... другие кнопки основного экрана (через SystemController)
    }
}
```

**Преимущества:**
- Состояния кнопок считываются **один раз** за итерацию loop()
- Меню получает состояния как параметры — **изоляция**
- Один вызов `update()` вместо четырёх `handleButton()` — **оптимальность**
- Порядок параметров фиксирован — соответствует `ButtonIndex` enum

---

## 12. Расширяемость

### Добавление нового параметра

1. Добавить `Key` в `Settings::Key` enum
2. Добавить поле в `Settings::Data`
3. Добавить дескриптор в `PARAM_DESCRIPTORS[]`
4. Добавить строку названия в PROGMEM
5. Добавить `MenuItem` в нужное подменю

### Добавление нового подменю

1. Создать массив `MenuItem` для нового подменю
2. Добавить запись в `SUBMENUS[]`
3. Добавить `MenuItem` типа `Submenu` в главное меню

### Добавление нового действия

1. Добавить код действия в enum `ActionCode`
2. Добавить `MenuItem` типа `Action` в нужное меню
3. Добавить обработчик в `_executeAction()`

---

## 13. Файловая структура

```
src/menu/
├── menu.h              # Публичный API (Menu::Manager)
├── menu.cpp            # Реализация логики навигации и FSM
├── menu_data.h         # Структуры данных и таблицы (PROGMEM)
├── menu_render.cpp     # Функции отрисовки экранов
└── menu_strings.h      # Строки меню (PROGMEM)
```

---

## 14. План реализации

План реализации вынесен в отдельный документ: **[MENU_IMPLEMENTATION_PLAN.md](./MENU_IMPLEMENTATION_PLAN.md)**
