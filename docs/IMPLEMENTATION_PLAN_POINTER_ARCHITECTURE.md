# План реализации: Архитектура с передачей указателя на SystemState

## Цель

Реализовать архитектуру, где:
1. SystemController сохраняет данные в структуре `SystemState`
2. При инициализации Display передается указатель на `SystemState`
3. Display использует заголовочный файл `system_state.h` для понимания структуры данных
4. Display является инструментом для SystemController
5. SystemState является единым источником сведений о структуре данных

## Текущее состояние

### SystemController
- Имеет `_state` (SystemState) - внутреннее состояние
- Имеет `_previousState` - для сравнения изменений
- Имеет `DirtyFlags` - флаги изменений
- Имеет методы-геттеры: `getDisplayHeader()`, `getDisplayWatering()`, `getDisplayTelemetry()`, `getDisplayCycle()`
- Имеет методы: `getDirtyFlags()`, `clearDirtyFlags()`
- Имеет `_checkChanges()` - проверка изменений
- Имеет методы преобразования: `_mapWateringState()`, `_mapGrowingPeriod()`

### Display::MainScreen
- Имеет структуры: `MainScreenState`, `MainScreenAlerts`, `MainScreenTelemetry`, `MainScreenCycle`
- Имеет enum: `WateringViewState`, `SeasonLabel`
- Имеет `setState()` (deprecated)
- Имеет `update()` - читает данные через геттеры с использованием dirty flags

## Целевое состояние

### SystemController
- Имеет `_state` (SystemState) - состояние системы
- Имеет `getSystemState()` - возвращает указатель на `_state`
- **Убрать**: `_previousState`, `DirtyFlags`, `_checkChanges()`, все геттеры для Display, все преобразования

### Display::MainScreen
- Имеет `begin()` - принимает указатель на `SystemState` (один раз)
- Имеет `update()` - полная перерисовка (читает данные напрямую из `SystemState`)
- **Убрать**: все структуры данных (`MainScreenState`, `MainScreenAlerts`, и т.д.), все enum (`WateringViewState`, `SeasonLabel`), `setState()`
- **Добавить**: `#include "../irrigation/system_state.h"` для понимания структуры данных

## Детальный план реализации

### Этап 1: Обновление SystemState

#### 1.1. Обновить комментарии в system_state.h
**Файл**: `src/irrigation/system_state.h`

Добавить комментарий о том, что `SystemState` используется:
- SystemController для хранения данных
- Display для понимания структуры данных

#### 1.2. Проверить, что все необходимые поля есть в SystemState
**Файл**: `src/irrigation/system_state.h`

Убедиться, что `SystemState` содержит все данные, необходимые для отображения:
- Состояние полива (wateringState, pumpActive, wateringRemainingSeconds)
- Состояние цикла (period, currentDay, totalDays, currentHour, currentMinute)
- Состояние освещения (lightOn)
- Данные датчиков (sensors.soilHumidity, sensors.airTemperature, sensors.airHumidity, sensors.sensorsValid)
- Системный режим (systemMode)

### Этап 2: Обновление SystemController

#### 2.1. Убрать все методы для Display
**Файл**: `src/irrigation/system_controller.h`

Удалить:
- `struct DirtyFlags`
- `getDirtyFlags()`
- `clearDirtyFlags()`
- `getDisplayHeader()`
- `getDisplayWatering()`
- `getDisplayTelemetry()`
- `getDisplayCycle()`
- `_mapWateringState()`
- `_mapGrowingPeriod()`
- `_checkChanges()`

#### 2.2. Обновить метод getSystemState()
**Файл**: `src/irrigation/system_controller.h`

```cpp
/**
 * Возвращает указатель на структуру данных состояния системы.
 * 
 * Используется для передачи указателя модулю Display при инициализации.
 * Display использует заголовочный файл system_state.h для понимания структуры.
 * 
 * @return const указатель на SystemState (не копируется)
 */
static const SystemState* getSystemState();
```

**Файл**: `src/irrigation/system_controller.cpp`

```cpp
const SystemState* SystemController::getSystemState() {
    return &_state;
}
```

#### 2.3. Убрать ненужные переменные
**Файл**: `src/irrigation/system_controller.h`

Удалить:
- `DirtyFlags _dirtyFlags`
- `SystemState _previousState`

**Файл**: `src/irrigation/system_controller.cpp`

Удалить:
- Инициализацию `_previousState` и `_dirtyFlags` из `begin()`
- Реализации всех удаленных методов
- Вызов `_checkChanges()` из `process()`

#### 2.4. Убрать forward declarations для Display
**Файл**: `src/irrigation/system_controller.h`

Убрать forward declarations для типов Display, т.к. они больше не нужны.

### Этап 3: Обновление Display::MainScreen

#### 3.1. Убрать все структуры и enum
**Файл**: `src/display/main_screen.h`

Удалить:
- `enum class WateringViewState`
- `struct MainScreenAlerts`
- `struct MainScreenTelemetry`
- `enum class SeasonLabel`
- `struct MainScreenCycle`
- `struct MainScreenState`

#### 3.2. Добавить include system_state.h
**Файл**: `src/display/main_screen.h`

```cpp
#include "../irrigation/system_state.h"
```

Убрать:
```cpp
#include "../irrigation/system_controller.h"
```

#### 3.3. Обновить метод begin()
**Файл**: `src/display/main_screen.h`

```cpp
/**
 * Инициализирует экран.
 * 
 * @param display ссылка на инициализированный объект U8g2.
 * @param refreshTimer внешний таймер, управляющий частотой update().
 * @param systemState указатель на структуру данных состояния системы.
 *                    Передается SystemController при инициализации.
 *                    Display использует system_state.h для понимания структуры.
 */
static void begin(U8G2& display, Timer& refreshTimer, 
                 const Irrigation::SystemState* systemState);
```

**Файл**: `src/display/main_screen.cpp` (если существует)

```cpp
static const Irrigation::SystemState* _systemState = nullptr;

void MainScreen::begin(U8G2& display, Timer& refreshTimer, 
                       const Irrigation::SystemState* systemState) {
    _systemState = systemState;
    // ... остальная инициализация ...
}
```

#### 3.4. Обновить метод update()
**Файл**: `src/display/main_screen.h`

```cpp
/**
 * Нужно вызывать в цикле. Метод проверяет таймер,
 * читает данные из переданного указателя SystemState
 * и выполняет полную перерисовку экрана.
 */
static void update();
```

**Файл**: `src/display/main_screen.cpp`

```cpp
void MainScreen::update() {
    if (!_refreshTimer.isReady() || !_systemState) {
        return;
    }
    
    // Полная перерисовка всех блоков
    _renderFirstBlock();
    _renderSecondBlock();
    _renderThirdBlock();
    _renderBottomIcons();
    
    _refreshTimer.reset();
}
```

#### 3.5. Обновить методы рендеринга
**Файл**: `src/display/main_screen.cpp`

Все методы `_render*()` должны читать данные напрямую из `_systemState`:
- `_renderFirstBlock()` - использует `_systemState->lightOn`, `currentHour`, `currentMinute`, `pumpActive`, `wateringState`
- `_renderSecondBlock()` - использует `_systemState->wateringState`, `sensors`
- `_renderThirdBlock()` - использует `_systemState->period`, `currentDay`, `totalDays`

**Примечание**: Display должен сам преобразовывать `WateringState` и `GrowingPeriod` в нужные представления для отображения, если это необходимо.

#### 3.6. Удалить ненужные методы
**Файл**: `src/display/main_screen.h`

Удалить:
- `setState()` (deprecated)
- `getState()`

**Файл**: `src/display/main_screen.cpp`

Удалить реализации удаленных методов.

### Этап 4: Обновление документации

#### 4.1. Обновить ARCHITECTURE_SYSTEM_CONTROLLER.md
- Убрать описание DirtyFlags и преобразований
- Добавить описание роли SystemState как единого источника сведений
- Обновить пример использования в `loop()`

#### 4.2. Обновить POINTER_BASED_ARCHITECTURE.md
- Обновить описание архитектуры
- Убрать упоминания о промежуточных структурах
- Добавить описание роли SystemState

#### 4.3. Обновить примеры использования
**Файл**: `docs/ARCHITECTURE_SYSTEM_CONTROLLER.md` или основной README

```cpp
void setup() {
    // ... инициализация других модулей ...
    
    SystemController::begin(/* параметры */);
    
    // Получаем указатель на состояние системы
    const Irrigation::SystemState* systemState = 
        SystemController::getSystemState();
    
    // Передаем указатель при инициализации Display (один раз)
    Display::MainScreen::begin(display, refreshTimer, systemState);
}

void loop() {
    // Обработка всех FSM, агрегация состояния системы
    SystemController::process();
    
    // Display читает данные через указатель и выполняет полную перерисовку
    Display::MainScreen::update();
}
```

## Порядок выполнения

1. **Этап 1**: Обновление SystemState (шаги 1.1-1.2)
2. **Этап 2**: Обновление SystemController (шаги 2.1-2.4)
3. **Этап 3**: Обновление Display::MainScreen (шаги 3.1-3.6)
4. **Этап 4**: Обновление документации (шаги 4.1-4.3)
5. **Этап 5**: Тестирование и проверка компиляции

## Проверка реализации

### Критерии успеха

1. ✅ SystemController не имеет DirtyFlags, _previousState, преобразований
2. ✅ SystemController имеет только `getSystemState()` для получения указателя
3. ✅ Display не имеет промежуточных структур (MainScreenState, и т.д.)
4. ✅ Display включает `system_state.h` для понимания структуры
5. ✅ Display::MainScreen::begin() принимает указатель на SystemState
6. ✅ Display::MainScreen::update() выполняет полную перерисовку
7. ✅ Display читает данные напрямую из SystemState
8. ✅ Код компилируется без ошибок
9. ✅ Документация обновлена

### Тестирование

1. Проверить компиляцию проекта
2. Проверить, что данные корректно агрегируются в SystemState
3. Проверить, что Display получает актуальные данные через указатель
4. Проверить, что нет утечек памяти (указатели)

## Замечания

1. **Безопасность указателя**: Указатель передается один раз при инициализации и не меняется. SystemController гарантирует, что данные обновляются в `process()`, поэтому указатель всегда валиден.

2. **Производительность**: Полная перерисовка при каждом `update()` может быть избыточной, но упрощает код. Если производительность станет проблемой, можно добавить простую проверку изменений в Display.

3. **Преобразования данных**: Если Display нужны преобразования `WateringState` или `GrowingPeriod` для отображения, эти преобразования должны выполняться в Display, а не в SystemController.

4. **Единый источник сведений**: SystemState является единым источником сведений о структуре данных. Изменение структуры требует изменения только SystemState и Display.
