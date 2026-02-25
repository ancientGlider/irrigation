/*
 * Menu module API
 *
 * Модуль меню настроек для OLED-дисплея 128×64.
 * Архитектура описана в docs/MENU_ARCHITECTURE.md
 */

#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "menu_data.h"
#include "../modules/timer/timer.h"

namespace Menu {

// ============================================================================
// Индексы кнопок (порядок соответствует параметрам update())
// ============================================================================

enum ButtonIndex : uint8_t {
    BTN_IDX_OK     = 0,
    BTN_IDX_UP     = 1,
    BTN_IDX_DOWN   = 2,
    BTN_IDX_CANCEL = 3
};

// ============================================================================
// Состояния FSM меню
// ============================================================================

enum class MenuFSMState : uint8_t {
    Closed,         // Меню закрыто
    MainMenu,       // Главное меню
    Submenu,        // Подменю
    Editing,        // Режим редактирования параметра
    Confirmation    // Диалог подтверждения
};

// ============================================================================
// Состояние навигации (RAM)
// ============================================================================

struct MenuState {
    uint8_t level;            // 0 = главное меню, 1 = подменю
    uint8_t menuIndex;        // Индекс текущего подменю (для level=1)
    uint8_t selectedItem;     // Индекс выбранного пункта
    uint8_t scrollOffset;     // Смещение прокрутки
    bool editing;             // Режим редактирования
    int16_t editValue;        // Временное значение при редактировании
    ParamKey currentKey;      // Текущий редактируемый ключ
};

// ============================================================================
// Тип callback для live-значения
// ============================================================================

typedef int16_t (*LiveValueCallback)();

// ============================================================================
// Класс менеджера меню
// ============================================================================

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
    static void setLiveValueCallback(LiveValueCallback cb);

private:
    // FSM и навигация
    static void _processMainMenuButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel);
    static void _processSubmenuButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel);
    static void _processEditButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel);
    static void _processConfirmButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel);
    
    // Навигация
    static void _navigateUp();
    static void _navigateDown();
    static void _selectItem();
    static void _goBack();
    
    // Редактирование
    static void _enterEditMode(ParamKey key);
    static void _changeValue(int8_t direction, uint8_t multiplier = 1);
    static void _saveValue();
    static void _cancelEdit();
    
    // Actions
    static void _executeAction(ActionCode code);
    static void _confirmReset();
    
    // Helpers
    static uint8_t _getCurrentItemCount();
    static void _getMenuItem(uint8_t index, MenuItem& out);
    static void _getParamDescriptorWithDynamicLimits(ParamKey key, ParamDescriptor& out);
    static int16_t _getParamValue(ParamKey key);
    static bool _setParamValue(ParamKey key, int16_t value);
    
    // Отрисовка (реализация в menu_render.cpp)
    static void _renderMainMenu();
    static void _renderSubmenu();
    static void _renderEditScreen();
    static void _renderConfirmScreen();
    static void _renderMenuList(const char* title, uint8_t itemCount);
    static void _formatValue(int16_t value, DisplayMode mode, char* buf, uint8_t bufSize);
    
    // Состояние
    static U8G2* _display;
    static MenuFSMState _fsmState;
    static MenuState _state;
    
    // Автоповтор кнопок
    static Timer _repeatTimer;
    static uint8_t _heldButtonIdx;
    static uint8_t _repeatCount;
    static constexpr uint8_t NO_BUTTON = 0xFF;
    static constexpr unsigned long REPEAT_PERIOD_MS = 100;
    static constexpr uint8_t FAST_AFTER_REPEATS = 15;
    
    // Callback для live-значения
    static LiveValueCallback _liveValueCallback;
    
    // Подтверждение сброса
    static bool _confirmYes;
    
    // Флаг пропуска первого цикла после открытия
    static bool _justOpened;
    
    // Page buffer FSM
    static bool _needsFirstPage;
    
    // Предыдущие состояния кнопок (для обработки переходов)
    static uint8_t _prevButtonOK;
    static uint8_t _prevButtonUp;
    static uint8_t _prevButtonDown;
    static uint8_t _prevButtonCancel;
};

} // namespace Menu
