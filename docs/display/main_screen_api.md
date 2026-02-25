## API главного экрана (`Display::MainScreen`)

### Область ответственности
Модуль отвечает за отрисовку основного экрана на дисплее SSD1306 (U8g2), принимает агрегированное состояние, управляет частичным обновлением и миганием элементов.

### Структуры данных

```cpp
enum class WateringViewState : uint8_t {
    AutoWatering,
    Waiting,
    Training,
    TrainingWait,
    Stop,
    NoWater
};

struct MainScreenAlerts {
    bool pumpActive;
    bool attention;
};

struct MainScreenTelemetry {
    uint16_t airTemperature; // температура * 10 (десятые доли градуса)
    uint8_t airHumidity;          // %
    uint8_t soilHumidity;         // %
    uint16_t sensorCountdown;     // сек до следующей проверки датчиков
};

enum class SeasonLabel : uint8_t { Spring, Summer, Autumn };

struct MainScreenCycle {
    SeasonLabel season;
    uint16_t currentDay;
    uint16_t totalDays;
};

struct MainScreenState {
    bool lightOn;
    uint8_t hour;
    uint8_t minute;
    WateringViewState wateringState;
    MainScreenTelemetry telemetry;
    MainScreenCycle cycle;
    MainScreenAlerts alerts;
};
```

### Публичный API

```cpp
namespace Display {
class MainScreen {
public:
    static void begin(U8G2& display, Timer& refreshTimer);
    static void setState(const MainScreenState& state);
    static void requestFullRefresh();
    static void update();
    static const MainScreenState& getState();
};
} // namespace Display
```

### Контракт обновления
- Частота вызова `update()` определяется внешним таймером (`Timer`), рекомендуемая периодичность — 500–1000 мс.
- Метод `setState()` принимают агрегированные данные из `MainScreenStateProvider` (см. задачи этапа 2).
- Модуль обеспечивает частичное обновление: каждый блок перерисовывается только при изменении данных либо при вызове `requestFullRefresh()`.
- Мигание элементов (двоеточие часов, `pumpActive`, `attention`) контролируется внутри `update()` на основе таймера.

### Зависимости
- `U8G2` — отрисовка.
- `Timer` — управление частотой обновления без блокирующих задержек.
- Агрегатор состояния (`MainScreenStateProvider`) будет предоставлять заполненную структуру `MainScreenState`.


