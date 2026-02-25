# Архитектура главного контроллера системы

## Цель

Главный контроллер (`SystemController`) является центральным процессором системы, который:
1. Управляет вызовами всех FSM контроллеров в цикле `loop()`
2. Осуществляет циклический опрос всех модулей
3. Получает данные о состоянии системы от датчиков и конечных автоматов
4. На основании полученных данных принимает решения по управлению системой
5. Сохраняет данные о состоянии системы в структуре `SystemState`
6. Предоставляет доступ к состоянию системы для модуля Display

## Принципы проектирования

### 1. Единый источник истины
- Все данные о состоянии системы хранятся в `SystemController` в структуре `SystemState`
- Модули системы не дублируют данные друг друга
- `SystemState` является единым источником сведений о структуре данных, описывающих состояние системы
- Модуль `Display` получает указатель на `SystemState` при инициализации

### 2. Минимизация дублирования
- Данные читаются из модулей один раз за цикл
- Результаты агрегации кэшируются в структуре `SystemState`
- Модуль `Display` не обращается напрямую к модулям системы, работает только с `SystemState`

### 3. Оптимизация ресурсов
- Структура состояния компактна (минимальные типы данных)
- Нет промежуточных преобразований данных
- Избегание лишних вызовов геттеров модулей

## Структура модуля

### Файлы
```
src/irrigation/
├── system_controller.h      # Заголовочный файл
├── system_controller.cpp     # Реализация
└── system_state.h            # Структуры данных состояния
```

### Класс `Irrigation::SystemController`

```cpp
namespace Irrigation {

class SystemController {
public:
    SystemController() = delete;
    
    // Инициализация всех модулей
    // Параметры передаются напрямую в модули и не хранятся в контроллере
    static void begin(
        uint8_t pinPump, bool pumpHighIsOn,
        uint8_t pinLight, bool lightHighIsOn,
        uint8_t pinSoilSensor, uint8_t pinSoilVCC,
        uint8_t pinAirSensor,
        TimerRTC& rtc
    );
    
    // Основной цикл обработки (вызывается в loop())
    static void process();
    
    // Получение указателя на состояние системы для Display
    static const SystemState* getSystemState();
    
    // Команды управления (делегируются модулям)
    static void setManualWatering();
    static void setManualCleaning();
    static void setTraining();
    static void stopWatering();
    static void forceSensorCheck();
    
private:
    // Агрегация состояния из всех модулей
    static void _aggregateState();
    
    // Обновление данных датчиков
    static void _updateSensors();
    
    static SystemState _state;
    static bool _initialized;
};

} // namespace Irrigation
```

### Структура `SystemState`

```cpp
namespace Irrigation {

struct SystemState {
    // Состояние полива
    WateringState wateringState;
    bool pumpActive;
    uint32_t wateringRemainingSeconds;
    
    // Состояние цикла выращивания
    GrowingPeriod period;
    uint16_t currentDay;
    uint16_t totalDays;
    uint8_t currentHour;
    uint8_t currentMinute;
    
    // Состояние освещения
    bool lightOn;
    
    // Данные датчиков
    struct {
        uint8_t soilHumidity;        // 0-100%
        int airTemperature;          // десятые доли °C
        int airHumidity;             // десятые доли %
        bool sensorsValid;           // валидность данных
    } sensors;
    
    // Системный режим
    Settings::SystemMode systemMode;
    
    // Время последнего обновления (для отладки)
    unsigned long lastUpdateMillis;
};

} // namespace Irrigation
```

## Алгоритм работы

### Инициализация (`begin()`)
1. Инициализация `Settings::Manager` (загрузка из EEPROM)
2. Инициализация `TimerRTC`
3. Инициализация `GrowingCycle` с RTC
4. Инициализация `LightControl`
5. Инициализация `Watering`
6. Инициализация `SoilSensor`
7. Инициализация `AirSensor`
8. Установка флага `_initialized = true`

### Основной цикл (`process()`)
1. Вызов `_updateSensors()` (обновление датчиков через FSM)
2. Вызов `Watering::process(soilHumidity)` (FSM полива)
3. Вызов `LightControl::update()` (обновление освещения)
4. Вызов `_aggregateState()` (агрегация всех данных в `_state`)

### Агрегация состояния (`_aggregateState()`)
1. Копирование состояния полива:
   - `_state.wateringState = Watering::getState()`
   - `_state.pumpActive = Watering::isPumpOn()`
   - `_state.wateringRemainingSeconds = Watering::getRemainingSeconds()`
2. Копирование состояния цикла:
   - `_state.period = GrowingCycle::getPeriod()`
   - `_state.currentDay = GrowingCycle::getCurrentDay()`
   - `_state.totalDays = Settings::Manager::get(Key::CycleLengthDays)`
   - `_state.currentHour = GrowingCycle::getCurrentHour()`
   - `_state.currentMinute = GrowingCycle::getCurrentMinute()`
3. Копирование состояния освещения:
   - `_state.lightOn = LightControl::isLightOn()`
4. Копирование данных датчиков:
   - `_state.sensors.soilHumidity = SoilSensor::getSensorData()`
   - `_state.sensors.airTemperature/humidity = AirSensor::getSensorData()`
5. Копирование системного режима:
   - `_state.systemMode = Settings::Manager::get(Key::SystemMode)`

## Интеграция с Display

### Роль SystemState

`SystemState` является единым источником сведений о структуре данных, описывающих состояние системы:
- Используется `SystemController` для хранения данных о состоянии системы
- Используется `Display` для понимания структуры данных, переданных через указатель

### Передача данных в Display

При инициализации `Display` вызывающий модуль (`SystemController`) передает указатель на структуру `SystemState`:

```cpp
void setup() {
    // ... инициализация других модулей ...
    
    // Инициализация главного контроллера
    Irrigation::SystemController::begin(/* параметры */);
    
    // Получаем указатель на состояние системы
    const Irrigation::SystemState* systemState = 
        Irrigation::SystemController::getSystemState();
    
    // Передаем указатель при инициализации Display
    Display::MainScreen::begin(display, displayRefreshTimer, systemState);
}
```

### Использование в Display

`Display` использует заголовочный файл `system_state.h` для понимания структуры данных:

```cpp
// В main_screen.h
#include "../irrigation/system_state.h"

// В main_screen.cpp
void MainScreen::update() {
    if (!_refreshTimer.isReady() || !_systemState) {
        return;
    }
    
    // Прямой доступ к данным SystemState
    bool lightOn = _systemState->lightOn;
    uint8_t hour = _systemState->currentHour;
    WateringState ws = _systemState->wateringState;
    // ... и т.д.
    
    // Полная перерисовка экрана
    _renderFirstBlock();
    _renderSecondBlock();
    _renderThirdBlock();
    _renderBottomIcons();
}
```

## Интеграция с `loop()`

```cpp
// src/irrigation.ino

#include "irrigation/system_controller.h"
#include "display/main_screen.h"
#include "modules/timerRTC/timerRTC.h"

TimerRTC rtc;
U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0);
Timer displayRefreshTimer(500); // 500 мс = 2 Гц

void setup() {
    Serial.begin(9600);
    
    // Инициализация RTC
    rtc.begin(12, 4, 3); // ENA, CLK, DATA
    
    // Инициализация главного контроллера
    Irrigation::SystemController::begin(
        A4, true,  // PIN_WATER, pumpHighIsOn
        A3, true,  // PIN_LIGHT, lightHighIsOn
        A0, A1,    // PIN_SOIL_DATA, PIN_SOIL_VCC
        A2,        // PIN_AIR
        rtc
    );
    
    // Получаем указатель на состояние системы
    const Irrigation::SystemState* systemState = 
        Irrigation::SystemController::getSystemState();
    
    // Инициализация дисплея с указателем на SystemState (передается один раз)
    display.begin();
    Display::MainScreen::begin(display, displayRefreshTimer, systemState);
}

void loop() {
    // Обработка всех FSM, агрегация состояния системы
    Irrigation::SystemController::process();
    
    // Display читает данные через указатель и выполняет полную перерисовку
    Display::MainScreen::update();
    
    // Обработка кнопок (будущий модуль)
    // ...
}
```

## Преимущества архитектуры

1. **Единая точка управления**: все FSM вызываются из одного места
2. **Отсутствие дублирования**: данные читаются один раз и кэшируются в `SystemState`
3. **Чистое разделение ответственности**:
   - `SystemController` — агрегация и оркестрация
   - `Display` — только отрисовка (инструмент для SystemController)
   - Модули системы — только своя логика
4. **Единый источник сведений**: `SystemState` определяет структуру данных для всей системы
5. **Оптимизация ресурсов**: минимум вызовов геттеров, компактные структуры, нет промежуточных преобразований

## Зависимости

- `Settings::Manager` — настройки системы
- `TimerRTC` — время цикла
- `Irrigation::GrowingCycle` — расчёт дня/часа/периода
- `Irrigation::LightControl` — управление освещением
- `Irrigation::Watering` — управление поливом
- `SoilSensor` — датчик влажности почвы
- `AirSensor` — датчик температуры/влажности воздуха
- `Display::MainScreen` — инструмент для отображения состояния (получает указатель на SystemState)

## Следующие шаги

1. Реализовать `SystemController` с методами `begin()`, `process()`, `getSystemState()`
2. Интегрировать в `irrigation.ino`
3. Обновить документацию модулей
