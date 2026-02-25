/*
 * Тестовый скетч для проверки отрисовки главного экрана
 * 
 * Тестирует:
 * - Задача 2.1: Отрисовка режима работы системы
 * - Задача 2.2: Отрисовка иконки освещения и времени
 * - Задача 2.3: Отрисовка оповещений (помпа, внимание)
 * - Задача 3.1: Отрисовка состояния полива (левая колонка)
 * - Задача 3.2: Отрисовка таймера проверки датчиков
 * - Задача 3.3: Отрисовка данных датчиков (правая колонка)
 * 
 * Конфигурация пинов:
 *   Реальное железо:              Wokwi (эмуляция):
 *   ─────────────────────────────────────────────────
 *   Дисплей SPI:                  Дисплей I2C:
 *     D8-RESET, D9-DC, D10-CS       A4-SDA, A5-SCL
 *     D11-DATA, D13-CLK
 *   Полив: A4                     Полив: D10 (переназначен)
 */

// #define WOKWI  // Раскомментировать для эмуляции в Wokwi (I2C дисплей)
#define SERIAL_DEBUG  // Включить отладочный вывод

#include <Arduino.h>
#include <U8g2lib.h>
#include "../src/display/main_screen.h"
#include "../src/irrigation/system_state.h"
#include "../src/irrigation/watering/watering.h"
#include "../src/irrigation/growing_cycle.h"
#include "../src/settings/settings_data.h"

// Дисплей: выбор между SPI (реальное железо) и I2C (Wokwi)
#ifdef WOKWI
    U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#else
    U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI display(
        U8G2_MIRROR,                // rotation (зеркальное отображение)
        /* clock=*/ 13,             // SCL (D13)
        /* data=*/ 11,              // SI (D11)
        /* cs=*/ 10,                // CS1 (D10)
        /* dc=*/ 9,                 // A0 (D9)
        /* reset=*/ 8               // RES (D8)
    );
#endif

// Моковые данные состояния системы
Irrigation::SystemState mockSystemState;

// Текущий режим для тестирования
uint8_t currentModeIndex = 0;
const uint8_t MODE_COUNT = 4;

void setup() {
    Serial.begin(9600);
    delay(1000);  // Даём время на инициализацию Serial
    
    Serial.println(F("=== Main Screen Display Test ==="));
    Serial.println();
    
    // Инициализация дисплея
    Serial.println(F("Initializing display..."));
    display.begin();
    display.enableUTF8Print();  // Включаем поддержку UTF-8 для кириллицы
    Serial.println(F("Display initialized"));
    
    // Инициализация моковых данных
    Serial.println(F("Setting up mock data..."));
    
    // Устанавливаем начальный режим
    mockSystemState.systemMode = Settings::SystemMode::Growing;
    mockSystemState.lightOn = true;
    mockSystemState.currentHour = 14;
    mockSystemState.currentMinute = 30;
    mockSystemState.pumpActive = false;
    mockSystemState.wateringState = Irrigation::WateringState::Stopping;
    mockSystemState.wateringRemainingSeconds = 0;
    
    // Данные цикла выращивания
    mockSystemState.period = Irrigation::GrowingPeriod::Spring;
    mockSystemState.currentDay = 24;
    mockSystemState.totalDays = 120;
    
    // Данные датчиков
    mockSystemState.sensors.soilHumidity = 65;
    mockSystemState.sensors.airTemperature = 235;  // 23.5°C
    mockSystemState.sensors.airHumidity = 580;     // 58.0%
    mockSystemState.sensors.sensorsValid = true;
    
    // Данные цикла
    mockSystemState.period = Irrigation::GrowingPeriod::Spring;
    mockSystemState.currentDay = 24;
    mockSystemState.totalDays = 120;
    
    Serial.println(F("Mock data initialized"));
    Serial.println();
    
    // Инициализация главного экрана
    Serial.println(F("Initializing MainScreen..."));
    Display::MainScreen::begin(display, &mockSystemState);
    Serial.println(F("MainScreen initialized"));
    Serial.println();
    
    // Выводим инструкции
    Serial.println(F("=== Test Instructions ==="));
    Serial.println(F("Send commands via Serial Monitor:"));
    Serial.println(F("  '0' - Growing mode"));
    Serial.println(F("  '1' - Spring mode"));
    Serial.println(F("  '2' - Summer mode"));
    Serial.println(F("  '3' - Autumn mode"));
    Serial.println(F("  'l' - Toggle light"));
    Serial.println(F("  'p' - Toggle pump"));
    Serial.println(F("  't' - Change time (+1 hour)"));
    Serial.println(F("  'm' - Change minutes (+1 minute)"));
    Serial.println(F("  'a' - Toggle attention (TrainingReady/OutOfWater)"));
    Serial.println(F("  'w' - Cycle watering state"));
    Serial.println(F("  's' - Change sensor countdown (+30 seconds)"));
    Serial.println(F("  'S' - Set sensor countdown to 1440 minutes (24 hours)"));
    Serial.println(F("  'r' - Change air temperature (+0.5°C)"));
    Serial.println(F("  'u' - Change air humidity (+1%)"));
    Serial.println(F("  'o' - Change soil humidity (+1%)"));
    Serial.println(F("  'g' - Cycle growing period (season)"));
    Serial.println();
    Serial.println(F("Display will update on each loop iteration"));
    Serial.println(F("Colon in time blinks every 500ms"));
    Serial.println(F("Attention alert '!' blinks (normal/inverted) every 500ms"));
    Serial.println();
}

void loop() {
    // Обработка команд из Serial
    if (Serial.available()) {
        char cmd = Serial.read();
        
        switch (cmd) {
            case '0':
                mockSystemState.systemMode = Settings::SystemMode::Growing;
                Serial.println(F("Mode: Growing"));
                break;
            case '1':
                mockSystemState.systemMode = Settings::SystemMode::Spring;
                Serial.println(F("Mode: Spring"));
                break;
            case '2':
                mockSystemState.systemMode = Settings::SystemMode::Summer;
                Serial.println(F("Mode: Summer"));
                break;
            case '3':
                mockSystemState.systemMode = Settings::SystemMode::Autumn;
                Serial.println(F("Mode: Autumn"));
                break;
            case 'l':
            case 'L':
                mockSystemState.lightOn = !mockSystemState.lightOn;
                Serial.print(F("Light: "));
                Serial.println(mockSystemState.lightOn ? F("ON") : F("OFF"));
                break;
            case 'p':
            case 'P':
                mockSystemState.pumpActive = !mockSystemState.pumpActive;
                Serial.print(F("Pump: "));
                Serial.println(mockSystemState.pumpActive ? F("ON") : F("OFF"));
                break;
            case 't':
            case 'T':
                mockSystemState.currentHour = (mockSystemState.currentHour + 1) % 24;
                Serial.print(F("Time: "));
                Serial.print(mockSystemState.currentHour);
                Serial.print(F(":"));
                if (mockSystemState.currentMinute < 10) Serial.print(F("0"));
                Serial.println(mockSystemState.currentMinute);
                break;
            case 'm':
            case 'M':
                mockSystemState.currentMinute = (mockSystemState.currentMinute + 1) % 60;
                Serial.print(F("Time: "));
                Serial.print(mockSystemState.currentHour);
                Serial.print(F(":"));
                if (mockSystemState.currentMinute < 10) Serial.print(F("0"));
                Serial.println(mockSystemState.currentMinute);
                break;
            case 'a':
            case 'A':
                // Переключаем между TrainingReady и OutOfWater для тестирования оповещения
                if (mockSystemState.wateringState == Irrigation::WateringState::TrainingReady) {
                    mockSystemState.wateringState = Irrigation::WateringState::OutOfWater;
                    Serial.println(F("Attention: OutOfWater"));
                } else if (mockSystemState.wateringState == Irrigation::WateringState::OutOfWater) {
                    mockSystemState.wateringState = Irrigation::WateringState::Idle;
                    Serial.println(F("Attention: OFF"));
                } else {
                    mockSystemState.wateringState = Irrigation::WateringState::TrainingReady;
                    Serial.println(F("Attention: TrainingReady"));
                }
                break;
            case 'w':
            case 'W':
                // Циклическое переключение всех состояний полива для тестирования отображения
                switch (mockSystemState.wateringState) {
                    case Irrigation::WateringState::Stopping:
                        mockSystemState.wateringState = Irrigation::WateringState::Idle;
                        Serial.println(F("WateringState: Idle -> CONTROL"));
                        break;
                    case Irrigation::WateringState::Idle:
                        mockSystemState.wateringState = Irrigation::WateringState::AutoPause;
                        Serial.println(F("WateringState: AutoPause -> AUTO POLIV"));
                        break;
                    case Irrigation::WateringState::AutoPause:
                        mockSystemState.wateringState = Irrigation::WateringState::AutoWatering;
                        Serial.println(F("WateringState: AutoWatering -> AUTO POLIV"));
                        break;
                    case Irrigation::WateringState::AutoWatering:
                        mockSystemState.wateringState = Irrigation::WateringState::TrainingWaiting;
                        Serial.println(F("WateringState: TrainingWaiting -> OZHIDANIE TRENIROVKI"));
                        break;
                    case Irrigation::WateringState::TrainingWaiting:
                        mockSystemState.wateringState = Irrigation::WateringState::TrainingReady;
                        Serial.println(F("WateringState: TrainingReady -> TRENIROVKA"));
                        break;
                    case Irrigation::WateringState::TrainingReady:
                        mockSystemState.wateringState = Irrigation::WateringState::OutOfWater;
                        Serial.println(F("WateringState: OutOfWater -> NET VODY"));
                        break;
                    case Irrigation::WateringState::OutOfWater:
                        mockSystemState.wateringState = Irrigation::WateringState::Stopping;
                        Serial.println(F("WateringState: Stopping -> STOP"));
                        break;
                    default:
                        mockSystemState.wateringState = Irrigation::WateringState::Stopping;
                        Serial.println(F("WateringState: Stopping -> STOP"));
                        break;
                }
                break;
            case 's':
                // Изменяем время ожидания проверки датчиков (+30 секунд)
                // В режимах TrainingReady и OutOfWater может быть до суток (86400 секунд = 1440 минут)
                mockSystemState.wateringRemainingSeconds = (mockSystemState.wateringRemainingSeconds + 30) % 86400;  // Максимум 23:59:59
                Serial.print(F("Sensor countdown: "));
                {
                    uint32_t totalMinutes = mockSystemState.wateringRemainingSeconds / 60;
                    uint8_t seconds = mockSystemState.wateringRemainingSeconds % 60;
                    Serial.print(totalMinutes);
                    Serial.print(F(":"));
                    if (seconds < 10) Serial.print(F("0"));
                    Serial.println(seconds);
                }
                break;
            case 'S':
                // Устанавливаем время ожидания на 1440 минут (24 часа) для тестирования corner case
                mockSystemState.wateringRemainingSeconds = 1440UL * 60UL;  // 86400 секунд
                Serial.println(F("Sensor countdown set to 1440 minutes (24 hours)"));
                break;
            case 'r':
            case 'R':
                // Изменяем температуру воздуха (+0.5°C)
                mockSystemState.sensors.airTemperature += 5;  // +0.5°C (в десятых долях)
                if (mockSystemState.sensors.airTemperature > 500) {  // Максимум 50.0°C
                    mockSystemState.sensors.airTemperature = -500;  // Минимум -50.0°C
                }
                {
                    float temp = mockSystemState.sensors.airTemperature / 10.0f;
                    Serial.print(F("Air temperature: "));
                    Serial.print(temp, 1);
                    Serial.println(F("°C"));
                }
                break;
            case 'u':
            case 'U':
                // Изменяем влажность воздуха (+1%)
                mockSystemState.sensors.airHumidity += 10;  // +1.0% (в десятых долях)
                if (mockSystemState.sensors.airHumidity > 1000) {  // Максимум 100.0%
                    mockSystemState.sensors.airHumidity = 0;  // Минимум 0.0%
                }
                {
                    float hum = mockSystemState.sensors.airHumidity / 10.0f;
                    Serial.print(F("Air humidity: "));
                    Serial.print(hum, 1);
                    Serial.println(F("%"));
                }
                break;
            case 'o':
            case 'O':
                // Изменяем влажность почвы (+1%)
                mockSystemState.sensors.soilHumidity = (mockSystemState.sensors.soilHumidity + 1) % 101;  // 0-100%
                Serial.print(F("Soil humidity: "));
                Serial.print(mockSystemState.sensors.soilHumidity);
                Serial.println(F("%"));
                break;
            case 'g':
            case 'G':
                // Переключаем период выращивания (сезон)
                {
                    uint8_t periodIndex = static_cast<uint8_t>(mockSystemState.period);
                    periodIndex = (periodIndex + 1) % 5;  // 0-4: Germination, Spring, Summer, Autumn, Completed
                    mockSystemState.period = static_cast<Irrigation::GrowingPeriod>(periodIndex);
                    const char* periodNames[] = {"Germination", "Spring", "Summer", "Autumn", "Completed"};
                    Serial.print(F("Growing period: "));
                    Serial.println(periodNames[periodIndex]);
                }
                break;
            default:
                if (cmd != '\n' && cmd != '\r') {
                    Serial.print(F("Unknown command: '"));
                    Serial.print(cmd);
                    Serial.println(F("'"));
                }
                break;
        }
        
        // Очищаем буфер Serial
        while (Serial.available()) {
            Serial.read();
        }
    }
    
    // Обновление дисплея
    // Вызываем update() в каждом цикле - он сам управляет частотой обновления
    Display::MainScreen::update();
    
    // Периодический вывод состояния для отладки
    static unsigned long lastDebug = 0;
    if (millis() - lastDebug > 2000) {
        lastDebug = millis();
        Serial.print(F("State: mode="));
        Serial.print(static_cast<uint8_t>(mockSystemState.systemMode));
        Serial.print(F(", light="));
        Serial.print(mockSystemState.lightOn ? F("ON") : F("OFF"));
        Serial.print(F(", pump="));
        Serial.print(mockSystemState.pumpActive ? F("ON") : F("OFF"));
        Serial.print(F(", wateringState="));
        Serial.print(static_cast<uint8_t>(mockSystemState.wateringState));
        Serial.print(F(", countdown="));
        uint32_t totalMinutes = mockSystemState.wateringRemainingSeconds / 60;
        uint8_t seconds = mockSystemState.wateringRemainingSeconds % 60;
        Serial.print(totalMinutes);
        Serial.print(F(":"));
        if (seconds < 10) Serial.print(F("0"));
        Serial.print(seconds);
        Serial.print(F(", temp="));
        Serial.print(mockSystemState.sensors.airTemperature / 10.0f, 1);
        Serial.print(F("°C, airHum="));
        Serial.print(mockSystemState.sensors.airHumidity / 10.0f, 1);
        Serial.print(F("%, soilHum="));
        Serial.print(mockSystemState.sensors.soilHumidity);
        Serial.print(F("%, time="));
        Serial.print(mockSystemState.currentHour);
        Serial.print(F(":"));
        if (mockSystemState.currentMinute < 10) Serial.print(F("0"));
        Serial.println(mockSystemState.currentMinute);
    }
}

