/*
 * Простой тест для проверки отрисовки режима
 * 
 * Минимальный скетч для диагностики проблем с координатами и выводом
 */

#define SERIAL_DEBUG

#include <Arduino.h>
#include <U8g2lib.h>
#include "../src/display/main_screen_labels.h"

// Инициализация дисплея (SPI 4-wire)
// U8G2_MIRROR для зеркального отображения (если дисплей установлен зеркально)
U8G2_SSD1306_128X64_NONAME_1_4W_SW_SPI display(
    U8G2_MIRROR,                // rotation (зеркальное отображение)
    /* clock=*/ 13,             // SCL (D13)
    /* data=*/ 11,              // SI (D11)
    /* cs=*/ 10,                // CS1 (D10)
    /* dc=*/ 9,                 // A0 (D9)
    /* reset=*/ 8               // RES (D8)
);

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    Serial.println(F("=== Simple Display Test ==="));
    
    // Инициализация дисплея
    display.begin();
    display.enableUTF8Print();
    display.setFont(u8g2_font_5x8_t_cyrillic);
    
    Serial.println(F("Display initialized"));
}

// Состояние теста
enum TestState {
    TEST_SIMPLE_BOX,      // Тест 1: Простой квадрат
    TEST_DIRECT_CYRILLIC, // Тест 2: Прямая кириллица
    TEST_PROGMEM,         // Тест 3: PROGMEM строка
    TEST_ALL_MODES        // Тест 4: Все режимы
};

static TestState currentTest = TEST_SIMPLE_BOX;
static TestState lastTest = TEST_SIMPLE_BOX;
static uint8_t currentMode = 0;
static unsigned long testStartTime = 0;
static const unsigned long TEST_DURATION = 3000;  // 3 секунды на тест

// Английские названия режимов для Serial
const char* MODE_NAMES[] = {
    "Growing",  // 0
    "Spring",   // 1
    "Summer",   // 2
    "Autumn"    // 3
};

void loop() {
    // Циклический вызов U8g2 (требуется для page buffer режима)
    display.firstPage();
    do {
        // Очищаем экран (белый фон)
        display.setDrawColor(0);
        display.drawBox(0, 0, 128, 64);
        
        switch (currentTest) {
            case TEST_SIMPLE_BOX: {
                // Тест 1: Простой черный квадрат в левом верхнем углу
                // Рисуем черный квадрат
                display.setDrawColor(1);
                display.drawBox(0, 0, 30, 8);
                
                // Рисуем белый текст внутри квадрата
                display.setDrawColor(0);
                display.setCursor(2, 7);
                display.print("TEST");
                break;
            }
            
            case TEST_DIRECT_CYRILLIC: {
                // Тест 2: Прямая кириллица (без PROGMEM)
                // Рисуем черный квадрат
                display.setDrawColor(1);
                display.drawBox(0, 0, 30, 8);
                
                // Рисуем белый текст с прямой кириллицей
                display.setDrawColor(0);
                display.setCursor(2, 7);
                display.print("РОСТ");  // Прямой вывод кириллицы
                break;
            }
            
            case TEST_PROGMEM: {
                // Тест 3: Проверка PROGMEM строк
                // Получаем строку из PROGMEM
                const char* label = Display::Labels::getModeLabel(0);  // Growing -> "РОСТ"
                if (label == nullptr) {
                    // Ошибка - выводим "ERR"
                    display.setDrawColor(1);
                    display.drawBox(0, 0, 30, 8);
                    display.setDrawColor(0);
                    display.setCursor(2, 7);
                    display.print("ERR");
                    break;
                }
                
                // Буфер достаточного размера для строк переменной длины
                char buffer[12] = {0};
                // Безопасное копирование из PROGMEM: копируем с ограничением размера буфера
                strncpy_P(buffer, label, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';  // Гарантируем нулевой байт в конце
                
                // Рисуем черный квадрат
                display.setDrawColor(1);
                display.drawBox(0, 0, 30, 8);
                
                // Рисуем текст
                display.setDrawColor(0);
                display.setCursor(2, 7);
                display.print(buffer);
                break;
            }
            
            case TEST_ALL_MODES: {
                // Тест 4: Все режимы по очереди
                const char* label = Display::Labels::getModeLabel(currentMode);
                if (label == nullptr) {
                    // Ошибка - выводим "ERR"
                    display.setDrawColor(1);
                    display.drawBox(0, 0, 30, 8);
                    display.setDrawColor(0);
                    display.setCursor(2, 7);
                    display.print("ERR");
                    break;
                }
                
                // Буфер достаточного размера для строк переменной длины
                char buffer[12] = {0};
                // Безопасное копирование из PROGMEM: копируем с ограничением размера буфера
                strncpy_P(buffer, label, sizeof(buffer) - 1);
                buffer[sizeof(buffer) - 1] = '\0';  // Гарантируем нулевой байт в конце
                
                // Рисуем черный квадрат
                display.setDrawColor(1);
                display.drawBox(0, 0, 30, 8);
                
                // Рисуем текст
                display.setDrawColor(0);
                display.setCursor(2, 7);
                display.print(buffer);
                break;
            }
        }
        
    } while (display.nextPage());
    
    // Управление переключением тестов
    unsigned long now = millis();
    
    // Инициализация времени начала теста
    if (testStartTime == 0) {
        testStartTime = now;
        Serial.println(F("--- Test 1: Simple Box ---"));
    }
    
    // Проверка истечения времени текущего теста
    if (now - testStartTime >= TEST_DURATION) {
        testStartTime = now;
        
        // Выводим информацию в Serial при смене теста (вне цикла отрисовки)
        switch (currentTest) {
            case TEST_SIMPLE_BOX:
                currentTest = TEST_DIRECT_CYRILLIC;
                Serial.println(F("--- Test 2: Direct Cyrillic ---"));
                break;
            case TEST_DIRECT_CYRILLIC:
                currentTest = TEST_PROGMEM;
                Serial.print(F("--- Test 3: PROGMEM String (Label for mode: "));
                Serial.print(MODE_NAMES[0]);
                Serial.println(F(") ---"));
                break;
            case TEST_PROGMEM:
                currentTest = TEST_ALL_MODES;
                currentMode = 0;
                Serial.println(F("--- Test 4: All Modes ---"));
                break;
            case TEST_ALL_MODES:
                currentMode++;
                if (currentMode >= 4) {
                    currentMode = 0;
                    currentTest = TEST_SIMPLE_BOX;  // Начинаем заново
                    Serial.println(F("--- Restarting tests ---"));
                    Serial.println(F("--- Test 1: Simple Box ---"));
                } else {
                    // Выводим информацию о новом режиме
                    Serial.print(F("Mode "));
                    Serial.print(currentMode);
                    Serial.print(F(": Label for mode: "));
                    Serial.println(MODE_NAMES[currentMode]);
                }
                break;
        }
        
        lastTest = currentTest;  // Обновляем после переключения
    }
    
    // Небольшая задержка для стабильности
    delay(10);
}

