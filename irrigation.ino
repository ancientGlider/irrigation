/*
 * Irrigation System - Menu Test
 * 
 * Тестовый скетч для проверки модуля меню.
 * Для тестирования других модулей замените этот файл соответствующим тестовым скетчем.
 * 
 * Управление:
 *   Кнопка OK (D2)     - Вход в меню / Выбор
 *   Кнопка UP (D5)     - Вверх / +
 *   Кнопка DOWN (D6)   - Вниз / -
 *   Кнопка CANCEL (D7) - Назад / Отмена
 * 
 * Конфигурация пинов:
 *   Реальное железо:              Wokwi (эмуляция):
 *   ─────────────────────────────────────────────────
 *   Дисплей SPI:                  Дисплей I2C:
 *     D8  - RESET                   A4 - SDA
 *     D9  - DC                      A5 - SCL
 *     D10 - CS
 *     D11 - DATA
 *     D13 - CLK
 *   ─────────────────────────────────────────────────
 *   Управление:                   Управление:
 *     A4 - Полив (PIN_WATER)        D10 - Полив (переназначен)
 *     A5 - Вентиляция               (вентиляция недоступна)
 * 
 * Для эмуляции в Wokwi раскомментируйте #define WOKWI ниже
 */

// #define WOKWI  // Раскомментировать для эмуляции в Wokwi (I2C дисплей)

#include <Arduino.h>
#include <U8g2lib.h>

// Модули проекта (пути относительно корня проекта)
#include "src/modules/button/button.h"
#include "src/modules/timer/timer.h"
#include "src/settings/settings.h"
#include "src/menu/menu.h"
#include "src/display/main_screen.h"
#include "src/irrigation/system_state.h"

// ============================================================================
// Пины
// ============================================================================

// Кнопки (одинаковые для обоих режимов)
#define PIN_BTN_OK     2
#define PIN_BTN_UP     5
#define PIN_BTN_DOWN   6
#define PIN_BTN_CANCEL 7

// Управление устройствами: разные пины для реального железа и Wokwi
#ifdef WOKWI
    // В Wokwi: A4/A5 заняты I2C дисплеем, полив переназначен на D10
    #define PIN_WATER       10  // Переназначен (был CS дисплея)
    // PIN_VENTILATION недоступен в режиме Wokwi
#else
    // Реальное железо: согласно PROJECT.md
    #define PIN_WATER       A4
    #define PIN_VENTILATION A5
#endif

// ============================================================================
// Глобальные объекты
// ============================================================================

// Дисплей: выбор между SPI (реальное железо) и I2C (Wokwi)
// Используем 1 (одностраничный буфер 128 байт) для экономии RAM
// Требует использования firstPage()/nextPage()
#ifdef WOKWI
    // I2C дисплей для эмуляции в Wokwi
    U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#else
    // SPI дисплей для реального железа
    U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI display(
        U8G2_MIRROR,            // зеркальное отображение
        /* clock=*/ 13,         // SCL (D13)
        /* data=*/ 11,          // SI (D11)
        /* cs=*/ 10,            // CS1 (D10)
        /* dc=*/ 9,             // A0 (D9)
        /* reset=*/ 8           // RES (D8)
    );
#endif

// Кнопки
Button btnOK(PIN_BTN_OK);
Button btnUp(PIN_BTN_UP);
Button btnDown(PIN_BTN_DOWN);
Button btnCancel(PIN_BTN_CANCEL);

// Мок состояния системы (для MainScreen, пока SystemController не используется)
Irrigation::SystemState mockSystemState;

// ============================================================================
// Callback для live-значения датчика (мок)
// ============================================================================

int16_t getMockSoilRawValue() {
    // Имитируем показания датчика с небольшим шумом
    static int16_t value = 500;
    value += random(-10, 11);
    if (value < 0) value = 0;
    if (value > 1023) value = 1023;
    return value;
}

// ============================================================================
// Setup
// ============================================================================

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    Serial.println(F("=== Irrigation System - Menu Test ==="));
    Serial.println();
    
    // Инициализация генератора случайных чисел
    randomSeed(analogRead(A0));
    
    // Инициализация дисплея
    Serial.println(F("Initializing display..."));
    display.begin();
    // НЕ включаем enableUTF8Print() - используем кастомные шрифты
    Serial.println(F("Display OK"));
    
    // Инициализация кнопок
    Serial.println(F("Initializing buttons..."));
    btnOK.begin();
    btnUp.begin();
    btnDown.begin();
    btnCancel.begin();
    Serial.println(F("Buttons OK"));
    
    // Инициализация моковых данных для MainScreen
    Serial.println(F("Setting up mock system state..."));
    mockSystemState.systemMode = Settings::SystemMode::Growing;
    mockSystemState.lightOn = true;
    mockSystemState.currentHour = 14;
    mockSystemState.currentMinute = 30;
    mockSystemState.pumpActive = false;
    mockSystemState.wateringState = Irrigation::WateringState::Idle;
    mockSystemState.period = Irrigation::GrowingPeriod::Spring;
    mockSystemState.currentDay = 24;
    mockSystemState.totalDays = 120;
    mockSystemState.sensors.soilHumidity = 65;
    mockSystemState.sensors.airTemperature = 235;  // 23.5°C
    mockSystemState.sensors.airHumidity = 580;     // 58.0%
    mockSystemState.sensors.sensorsValid = true;
    Serial.println(F("Mock state OK"));
    
    // Инициализация MainScreen
    Serial.println(F("Initializing MainScreen..."));
    Display::MainScreen::begin(display, &mockSystemState);
    Serial.println(F("MainScreen OK"));
    
    // Инициализация меню
    Serial.println(F("Initializing Menu..."));
    Menu::Manager::begin(display);
    Menu::Manager::setLiveValueCallback(getMockSoilRawValue);
    Serial.println(F("Menu OK"));
    
    Serial.println();
    Serial.println(F("=== Ready! ==="));
    Serial.println(F("Press OK button to open menu"));
    Serial.println(F("Or use Serial commands:"));
    Serial.println(F("  'm' - open Menu"));
    Serial.println(F("  'c' - close Menu"));
    Serial.println();
}

// ============================================================================
// Loop
// ============================================================================

void loop() {
    // 1. Опрос кнопок (FSM антидребезга)
    btnOK.check();
    btnUp.check();
    btnDown.check();
    btnCancel.check();
    
    // 2. Считываем состояния один раз
    uint8_t sOK     = btnOK.getState();
    uint8_t sUp     = btnUp.getState();
    uint8_t sDown   = btnDown.getState();
    uint8_t sCancel = btnCancel.getState();
    
    // 3. Обработка Serial команд (для отладки)
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 'm':
            case 'M':
                if (!Menu::Manager::isOpen()) {
                    Serial.println(F("Opening menu via Serial..."));
                    Menu::Manager::open();
                }
                break;
            case 'c':
            case 'C':
                if (Menu::Manager::isOpen()) {
                    Serial.println(F("Closing menu via Serial..."));
                    Menu::Manager::close();
                }
                break;
        }
        // Очищаем буфер
        while (Serial.available()) Serial.read();
    }
    
    // 4. UI (в зависимости от состояния меню)
    if (Menu::Manager::isOpen()) {
        // Передаём состояния в меню
        Menu::Manager::update(sOK, sUp, sDown, sCancel);
        Menu::Manager::render();
    } else {
        // Основной экран
        Display::MainScreen::update();
        
        // Кнопка OK открывает меню
        if (sOK == BUTTON_PRESSED) {
            Serial.println(F("Opening menu..."));
            Menu::Manager::open();
        }
    }
    
    // 5. Debug: вывод при изменении состояния кнопок
    static uint8_t lastStates[4] = {0, 0, 0, 0};
    bool changed = false;
    
    if (sOK != lastStates[0]) {
        if (sOK == BUTTON_PRESSED) Serial.println(F("BTN: OK pressed"));
        else if (sOK == BUTTON_LONGPRESSED) Serial.println(F("BTN: OK long"));
        lastStates[0] = sOK;
        changed = true;
    }
    if (sUp != lastStates[1]) {
        if (sUp == BUTTON_PRESSED) Serial.println(F("BTN: UP pressed"));
        else if (sUp == BUTTON_LONGPRESSED) Serial.println(F("BTN: UP long"));
        lastStates[1] = sUp;
        changed = true;
    }
    if (sDown != lastStates[2]) {
        if (sDown == BUTTON_PRESSED) Serial.println(F("BTN: DOWN pressed"));
        else if (sDown == BUTTON_LONGPRESSED) Serial.println(F("BTN: DOWN long"));
        lastStates[2] = sDown;
        changed = true;
    }
    if (sCancel != lastStates[3]) {
        if (sCancel == BUTTON_PRESSED) Serial.println(F("BTN: CANCEL pressed"));
        else if (sCancel == BUTTON_LONGPRESSED) Serial.println(F("BTN: CANCEL long"));
        lastStates[3] = sCancel;
        changed = true;
    }
}
