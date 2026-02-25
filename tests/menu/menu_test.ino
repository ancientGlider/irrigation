/*
 * Тестовый скетч для модуля меню
 * 
 * Демонстрирует интеграцию Menu::Manager с главным экраном.
 * 
 * Управление:
 *   Кнопка OK (D2)     - Вход/выбор
 *   Кнопка UP (D5)     - Вверх / +
 *   Кнопка DOWN (D6)   - Вниз / -
 *   Кнопка CANCEL (D7) - Назад / Отмена
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

#include <Arduino.h>
#include <U8g2lib.h>

// Модули проекта
#include "../../src/modules/button/button.h"
#include "../../src/modules/timer/timer.h"
#include "../../src/settings/settings.h"
#include "../../src/menu/menu.h"
#include "../../src/display/main_screen.h"
#include "../../src/irrigation/system_state.h"

// ============================================================================
// Пины
// ============================================================================

#define PIN_BTN_OK     2
#define PIN_BTN_UP     5
#define PIN_BTN_DOWN   6
#define PIN_BTN_CANCEL 7

#ifdef WOKWI
    #define PIN_WATER 10
#else
    #define PIN_WATER A4
#endif

// ============================================================================
// Глобальные объекты
// ============================================================================

// Дисплей: выбор между SPI (реальное железо) и I2C (Wokwi)
#ifdef WOKWI
    U8G2_SSD1306_128X64_NONAME_1_HW_I2C display(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#else
    U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI display(
        U8G2_MIRROR,
        /* clock=*/ 13,
        /* data=*/ 11,
        /* cs=*/ 10,
        /* dc=*/ 9,
        /* reset=*/ 8
    );
#endif

// Кнопки
Button btnOK(PIN_BTN_OK);
Button btnUp(PIN_BTN_UP);
Button btnDown(PIN_BTN_DOWN);
Button btnCancel(PIN_BTN_CANCEL);

// Мок состояния системы (для MainScreen)
Irrigation::SystemState mockSystemState;

// ============================================================================
// Callback для live-значения датчика (мок)
// ============================================================================

int16_t getMockSoilRawValue() {
    // Возвращаем случайное значение для демонстрации
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
    
    Serial.println(F("=== Menu Test ==="));
    Serial.println();
    
    // Инициализация дисплея
    Serial.println(F("Initializing display..."));
    display.begin();
    display.enableUTF8Print();
    
    // Инициализация кнопок
    Serial.println(F("Initializing buttons..."));
    btnOK.begin();
    btnUp.begin();
    btnDown.begin();
    btnCancel.begin();
    
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
    mockSystemState.sensors.airTemperature = 235;
    mockSystemState.sensors.airHumidity = 580;
    mockSystemState.sensors.sensorsValid = true;
    
    // Инициализация MainScreen
    Serial.println(F("Initializing MainScreen..."));
    Display::MainScreen::begin(display, &mockSystemState);
    
    // Инициализация меню
    Serial.println(F("Initializing Menu..."));
    Menu::Manager::begin(display);
    Menu::Manager::setLiveValueCallback(getMockSoilRawValue);
    
    Serial.println(F("Ready!"));
    Serial.println(F("Press OK button to open menu"));
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
    
    // 3. UI (в зависимости от состояния меню)
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
    
    // Debug: вывод при изменении состояния кнопок
    static uint8_t lastStates[4] = {0, 0, 0, 0};
    if (sOK != lastStates[0] || sUp != lastStates[1] || 
        sDown != lastStates[2] || sCancel != lastStates[3]) {
        
        if (sOK != lastStates[0] && sOK != BUTTON_UNPRESSED) {
            Serial.print(F("OK: ")); Serial.println(sOK);
        }
        if (sUp != lastStates[1] && sUp != BUTTON_UNPRESSED) {
            Serial.print(F("UP: ")); Serial.println(sUp);
        }
        if (sDown != lastStates[2] && sDown != BUTTON_UNPRESSED) {
            Serial.print(F("DOWN: ")); Serial.println(sDown);
        }
        if (sCancel != lastStates[3] && sCancel != BUTTON_UNPRESSED) {
            Serial.print(F("CANCEL: ")); Serial.println(sCancel);
        }
        
        lastStates[0] = sOK;
        lastStates[1] = sUp;
        lastStates[2] = sDown;
        lastStates[3] = sCancel;
    }
}
