#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "../modules/timer/timer.h"
#include "../irrigation/system_state.h"

namespace Display {

/**
 * Главный экран дисплея.
 *
 * Display является инструментом для SystemController.
 * При инициализации получает указатель на SystemState от SystemController
 * и использует заголовочный файл system_state.h для понимания структуры данных.
 *
 * Использует внутренний таймер обновления для управления частотой перерисовки.
 */

class MainScreen {
public:
    MainScreen() = delete;

    /**
     * Инициализирует экран.
     * 
     * @param display ссылка на инициализированный объект U8g2.
     * @param systemState указатель на структуру данных состояния системы.
     *                    Передается SystemController при инициализации.
     *                    Display использует system_state.h для понимания структуры.
     */
    static void begin(U8G2& display, const Irrigation::SystemState* systemState);

    /**
     * Нужно вызывать в цикле. Метод читает данные из переданного указателя SystemState
     * и выполняет полную перерисовку экрана в режиме page buffer U8g2.
     */
    static void update();

private:
    static void _renderFirstBlock();
    static void _renderSecondBlock();
    static void _renderThirdBlock();
    static void _renderBottomIcons();
    
    // Подметоды для первого блока
    static void _renderMode();
    static void _renderLightAndTime();
    static void _renderAlerts();
    
    // Подметоды для второго блока
    static void _renderWateringState();
    static void _renderSensorCountdown();
    static void _renderSensorData();
    
    // Подметоды для третьего блока
    static void _renderCycleInfo();

    // Статические переменные состояния
    static U8G2* _display;
    static const Irrigation::SystemState* _systemState;
    static bool _needsFirstPage;
    static bool _blinkState;
    static Timer _blinkTimer;

    static constexpr unsigned long BLINK_PERIOD_MS = 500UL;
};

}  // namespace Display


