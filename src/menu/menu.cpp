/*
 * Menu module implementation
 *
 * Реализация FSM навигации и логики редактирования.
 * Отрисовка вынесена в menu_render.cpp
 */

#include "menu.h"
#include "../settings/settings.h"
#include "../irrigation/growing_cycle.h"
#include "../irrigation/system_controller.h"
#include "../modules/button/button.h"

namespace Menu {

// ============================================================================
// Статические переменные
// ============================================================================

U8G2* Manager::_display = nullptr;
MenuFSMState Manager::_fsmState = MenuFSMState::Closed;
MenuState Manager::_state = {};

Timer Manager::_repeatTimer(0);  // Инициализируем в begin()
uint8_t Manager::_heldButtonIdx = NO_BUTTON;
uint8_t Manager::_repeatCount = 0;

LiveValueCallback Manager::_liveValueCallback = nullptr;
bool Manager::_confirmYes = false;
bool Manager::_justOpened = false;  // Флаг для пропуска первого цикла
bool Manager::_needsFirstPage = true;  // Page buffer FSM
uint8_t Manager::_prevButtonOK = BUTTON_UNPRESSED;
uint8_t Manager::_prevButtonUp = BUTTON_UNPRESSED;
uint8_t Manager::_prevButtonDown = BUTTON_UNPRESSED;
uint8_t Manager::_prevButtonCancel = BUTTON_UNPRESSED;

// ============================================================================
// Публичный API
// ============================================================================

void Manager::begin(U8G2& display) {
    _display = &display;
    _fsmState = MenuFSMState::Closed;
    _state = {};
    _heldButtonIdx = NO_BUTTON;
    _repeatCount = 0;
    _repeatTimer.setPeriod(REPEAT_PERIOD_MS);  // Инициализируем таймер здесь
}

void Manager::open() {
    if (_fsmState != MenuFSMState::Closed) return;
    
    _fsmState = MenuFSMState::MainMenu;
    _state.level = 0;
    _state.menuIndex = 0;
    _state.selectedItem = 0;
    _state.scrollOffset = 0;
    _state.editing = false;
    _heldButtonIdx = NO_BUTTON;
    _repeatCount = 0;
    _justOpened = true;
    _needsFirstPage = true;  // Начать с firstPage при открытии
    
    // Сбрасываем предыдущие состояния кнопок
    _prevButtonOK = BUTTON_UNPRESSED;
    _prevButtonUp = BUTTON_UNPRESSED;
    _prevButtonDown = BUTTON_UNPRESSED;
    _prevButtonCancel = BUTTON_UNPRESSED;
}

void Manager::close() {
    _fsmState = MenuFSMState::Closed;
    _state.editing = false;
    _heldButtonIdx = NO_BUTTON;
}

bool Manager::isOpen() {
    return _fsmState != MenuFSMState::Closed;
}

void Manager::update(uint8_t stateOK, uint8_t stateUp, uint8_t stateDown, uint8_t stateCancel) {
    // Пропускаем первый цикл после открытия (кнопка OK ещё активна)
    if (_justOpened) {
        _justOpened = false;
        return;
    }
    
    switch (_fsmState) {
        case MenuFSMState::Closed:
            break;
            
        case MenuFSMState::MainMenu:
            _processMainMenuButtons(stateOK, stateUp, stateDown, stateCancel);
            break;
            
        case MenuFSMState::Submenu:
            _processSubmenuButtons(stateOK, stateUp, stateDown, stateCancel);
            break;
            
        case MenuFSMState::Editing:
            _processEditButtons(stateOK, stateUp, stateDown, stateCancel);
            break;
            
        case MenuFSMState::Confirmation:
            _processConfirmButtons(stateOK, stateUp, stateDown, stateCancel);
            break;
    }
}

void Manager::setLiveValueCallback(LiveValueCallback cb) {
    _liveValueCallback = cb;
}

// ============================================================================
// Обработка кнопок по состояниям
// ============================================================================

void Manager::_processMainMenuButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel) {
    // OK — выбор пункта (только на переходе UNPRESSED -> PRESSED)
    if (sOK == BUTTON_PRESSED && _prevButtonOK == BUTTON_UNPRESSED) {
        _selectItem();
    }
    
    // CANCEL — выход из меню
    if (sCancel == BUTTON_PRESSED && _prevButtonCancel == BUTTON_UNPRESSED) {
        close();
    }
    
    // UP/DOWN — навигация
    if (sUp == BUTTON_PRESSED && _prevButtonUp == BUTTON_UNPRESSED) {
        _navigateUp();
    }
    if (sDown == BUTTON_PRESSED && _prevButtonDown == BUTTON_UNPRESSED) {
        _navigateDown();
    }
    
    // Сохраняем состояния
    _prevButtonOK = sOK;
    _prevButtonUp = sUp;
    _prevButtonDown = sDown;
    _prevButtonCancel = sCancel;
}

void Manager::_processSubmenuButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel) {
    // OK — выбор пункта (вход в редактирование)
    if (sOK == BUTTON_PRESSED && _prevButtonOK == BUTTON_UNPRESSED) {
        _selectItem();
    }
    
    // CANCEL — возврат в главное меню
    if (sCancel == BUTTON_PRESSED && _prevButtonCancel == BUTTON_UNPRESSED) {
        _goBack();
    }
    
    // UP/DOWN — навигация
    if (sUp == BUTTON_PRESSED && _prevButtonUp == BUTTON_UNPRESSED) {
        _navigateUp();
    }
    if (sDown == BUTTON_PRESSED && _prevButtonDown == BUTTON_UNPRESSED) {
        _navigateDown();
    }
    
    // Сохраняем состояния
    _prevButtonOK = sOK;
    _prevButtonUp = sUp;
    _prevButtonDown = sDown;
    _prevButtonCancel = sCancel;
}

void Manager::_processEditButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel) {
    // OK — сохранение (только на переходе)
    if (sOK == BUTTON_PRESSED && _prevButtonOK == BUTTON_UNPRESSED) {
        _saveValue();
        _prevButtonOK = sOK;
        _prevButtonUp = sUp;
        _prevButtonDown = sDown;
        _prevButtonCancel = sCancel;
        return;
    }
    
    // CANCEL — отмена (только на переходе)
    if (sCancel == BUTTON_PRESSED && _prevButtonCancel == BUTTON_UNPRESSED) {
        _cancelEdit();
        _prevButtonOK = sOK;
        _prevButtonUp = sUp;
        _prevButtonDown = sDown;
        _prevButtonCancel = sCancel;
        return;
    }
    
    // UP/DOWN — изменение значения с автоповтором
    uint8_t activeIdx = NO_BUTTON;
    uint8_t activeState = BUTTON_UNPRESSED;
    int8_t direction = 0;
    
    if (sUp > BUTTON_UNPRESSED) {
        activeIdx = BTN_IDX_UP;
        activeState = sUp;
        direction = 1;
    } else if (sDown > BUTTON_UNPRESSED) {
        activeIdx = BTN_IDX_DOWN;
        activeState = sDown;
        direction = -1;
    }
    
    // Защита от двух кнопок одновременно
    if (_heldButtonIdx != NO_BUTTON && activeIdx != NO_BUTTON && activeIdx != _heldButtonIdx) {
        uint8_t heldState = (_heldButtonIdx == BTN_IDX_UP) ? sUp : sDown;
        if (heldState == BUTTON_UNPRESSED) {
            _heldButtonIdx = NO_BUTTON;
            _repeatCount = 0;
        } else {
            return; // Игнорируем вторую кнопку
        }
    }
    
    // Обработка событий (только на переходе UNPRESSED -> PRESSED для начала)
    if (activeState == BUTTON_PRESSED && _heldButtonIdx == NO_BUTTON) {
        uint8_t prevState = (activeIdx == BTN_IDX_UP) ? _prevButtonUp : _prevButtonDown;
        if (prevState == BUTTON_UNPRESSED) {
            // Момент нажатия — однократное изменение
            _changeValue(direction);
            _heldButtonIdx = activeIdx;
            _repeatCount = 0;
            _repeatTimer.setPeriod(REPEAT_PERIOD_MS);
            _repeatTimer.drop();  // Сбрасываем таймер
        }
    }
    else if (activeState == BUTTON_LONGPRESSED && _heldButtonIdx == activeIdx) {
        // Переход в режим удержания — запускаем таймер для автоповтора
        if (_repeatCount == 0) {
            // Первый раз в LONGPRESSED - запускаем таймер
            _repeatTimer.setPeriod(REPEAT_PERIOD_MS);
            _repeatTimer.drop();
        }
    }
    else if (activeIdx == NO_BUTTON && _heldButtonIdx != NO_BUTTON) {
        // Кнопка отпущена — сброс
        _heldButtonIdx = NO_BUTTON;
        _repeatCount = 0;
    }
    
    // Автоповтор (если кнопка удерживается - PRESSED или LONGPRESSED)
    if (_heldButtonIdx != NO_BUTTON && activeState > BUTTON_UNPRESSED && _repeatTimer.isReady()) {
        _repeatCount++;
        int8_t dir = (_heldButtonIdx == BTN_IDX_UP) ? 1 : -1;
        uint8_t multiplier = (_repeatCount > FAST_AFTER_REPEATS) ? 10 : 1;
        _changeValue(dir, multiplier);
    }
    
    // Сохраняем состояния
    _prevButtonOK = sOK;
    _prevButtonCancel = sCancel;
    _prevButtonUp = sUp;
    _prevButtonDown = sDown;
}

void Manager::_processConfirmButtons(uint8_t sOK, uint8_t sUp, uint8_t sDown, uint8_t sCancel) {
    // UP/DOWN — переключение ДА/НЕТ (только на переходе)
    if ((sUp == BUTTON_PRESSED && _prevButtonUp == BUTTON_UNPRESSED) ||
        (sDown == BUTTON_PRESSED && _prevButtonDown == BUTTON_UNPRESSED)) {
        _confirmYes = !_confirmYes;
    }
    
    // OK — подтверждение (только на переходе)
    if (sOK == BUTTON_PRESSED && _prevButtonOK == BUTTON_UNPRESSED) {
        if (_confirmYes) {
            _confirmReset();
        }
        // Возврат в главное меню
        _fsmState = MenuFSMState::MainMenu;
        _confirmYes = false;
        // Сбрасываем состояния кнопок
        _prevButtonOK = BUTTON_UNPRESSED;
        _prevButtonUp = BUTTON_UNPRESSED;
        _prevButtonDown = BUTTON_UNPRESSED;
        _prevButtonCancel = BUTTON_UNPRESSED;
        return;
    }
    
    // CANCEL — отмена (только на переходе)
    if (sCancel == BUTTON_PRESSED && _prevButtonCancel == BUTTON_UNPRESSED) {
        _fsmState = MenuFSMState::MainMenu;
        _confirmYes = false;
        // Сбрасываем состояния кнопок
        _prevButtonOK = BUTTON_UNPRESSED;
        _prevButtonUp = BUTTON_UNPRESSED;
        _prevButtonDown = BUTTON_UNPRESSED;
        _prevButtonCancel = BUTTON_UNPRESSED;
        return;
    }
    
    // Сохраняем состояния
    _prevButtonOK = sOK;
    _prevButtonUp = sUp;
    _prevButtonDown = sDown;
    _prevButtonCancel = sCancel;
}

// ============================================================================
// Навигация
// ============================================================================

void Manager::_navigateUp() {
    if (_state.selectedItem > 0) {
        _state.selectedItem--;
        // Прокрутка вверх если нужно
        if (_state.selectedItem < _state.scrollOffset) {
            _state.scrollOffset = _state.selectedItem;
        }
    }
}

void Manager::_navigateDown() {
    uint8_t maxItem = _getCurrentItemCount() - 1;
    if (_state.selectedItem < maxItem) {
        _state.selectedItem++;
        // Прокрутка вниз если нужно (5 видимых строк)
        if (_state.selectedItem >= _state.scrollOffset + 5) {
            _state.scrollOffset = _state.selectedItem - 4;
        }
    }
}

void Manager::_selectItem() {
    MenuItem item;
    _getMenuItem(_state.selectedItem, item);
    
    switch (item.type) {
        case MenuItemType::Submenu:
            // Вход в подменю
            _state.level = 1;
            _state.menuIndex = static_cast<uint8_t>(item.data);
            _state.selectedItem = 0;
            _state.scrollOffset = 0;
            _fsmState = MenuFSMState::Submenu;
            // Сбрасываем состояния кнопок при переходе
            _prevButtonOK = BUTTON_UNPRESSED;
            _prevButtonUp = BUTTON_UNPRESSED;
            _prevButtonDown = BUTTON_UNPRESSED;
            _prevButtonCancel = BUTTON_UNPRESSED;
            break;
            
        case MenuItemType::Value:
            // Вход в режим редактирования
            _enterEditMode(static_cast<ParamKey>(item.data));
            // Сбрасываем состояния кнопок при переходе
            _prevButtonOK = BUTTON_UNPRESSED;
            _prevButtonUp = BUTTON_UNPRESSED;
            _prevButtonDown = BUTTON_UNPRESSED;
            _prevButtonCancel = BUTTON_UNPRESSED;
            break;
            
        case MenuItemType::Action:
            // Выполнение действия
            _executeAction(static_cast<ActionCode>(item.data));
            break;
    }
}

void Manager::_goBack() {
    if (_state.level == 1) {
        // Из подменю в главное меню
        _state.level = 0;
        _state.selectedItem = 0;
        _state.scrollOffset = 0;
        _fsmState = MenuFSMState::MainMenu;
        // Сбрасываем состояния кнопок при переходе
        _prevButtonOK = BUTTON_UNPRESSED;
        _prevButtonUp = BUTTON_UNPRESSED;
        _prevButtonDown = BUTTON_UNPRESSED;
        _prevButtonCancel = BUTTON_UNPRESSED;
    } else {
        // Из главного меню — закрыть
        close();
    }
}

// ============================================================================
// Редактирование
// ============================================================================

void Manager::_enterEditMode(ParamKey key) {
    _state.editing = true;
    _state.currentKey = key;
    _state.editValue = _getParamValue(key);
    _fsmState = MenuFSMState::Editing;
    _heldButtonIdx = NO_BUTTON;
    _repeatCount = 0;
}

void Manager::_changeValue(int8_t direction, uint8_t multiplier) {
    ParamDescriptor desc;
    _getParamDescriptorWithDynamicLimits(_state.currentKey, desc);
    
    int16_t newValue = _state.editValue + (direction * desc.step * multiplier);
    
    // Ограничение границами
    if (newValue < desc.minValue) newValue = desc.minValue;
    if (newValue > desc.maxValue) newValue = desc.maxValue;
    
    _state.editValue = newValue;
}

void Manager::_saveValue() {
    _setParamValue(_state.currentKey, _state.editValue);
    _cancelEdit();
}

void Manager::_cancelEdit() {
    _state.editing = false;
    _fsmState = MenuFSMState::Submenu;
    _heldButtonIdx = NO_BUTTON;
    _repeatCount = 0;
    // Сбрасываем состояния кнопок при выходе из редактирования
    _prevButtonOK = BUTTON_UNPRESSED;
    _prevButtonUp = BUTTON_UNPRESSED;
    _prevButtonDown = BUTTON_UNPRESSED;
    _prevButtonCancel = BUTTON_UNPRESSED;
}

// ============================================================================
// Actions
// ============================================================================

void Manager::_executeAction(ActionCode code) {
    switch (code) {
        case ActionCode::StartTraining:
            Irrigation::SystemController::setTraining();
            close(); // Закрываем меню после запуска
            break;
            
        case ActionCode::StartCleaning:
            Irrigation::SystemController::setManualCleaning();
            close();
            break;
            
        case ActionCode::ResetSettings:
            // Переход в диалог подтверждения
            _confirmYes = false;
            _fsmState = MenuFSMState::Confirmation;
            break;
    }
}

void Manager::_confirmReset() {
    // Сброс настроек к значениям по умолчанию
    // TODO: Добавить Settings::resetToDefaults() если отсутствует
    // Пока устанавливаем критические параметры вручную
    
    // Закрываем меню после сброса
    close();
}

// ============================================================================
// Helpers
// ============================================================================

uint8_t Manager::_getCurrentItemCount() {
    if (_state.level == 0) {
        return MAIN_MENU_COUNT;
    } else {
        Submenu sub;
        memcpy_P(&sub, &SUBMENUS[_state.menuIndex], sizeof(Submenu));
        return sub.itemCount;
    }
}

void Manager::_getMenuItem(uint8_t index, MenuItem& out) {
    if (_state.level == 0) {
        memcpy_P(&out, &MAIN_MENU_ITEMS[index], sizeof(MenuItem));
    } else {
        Submenu sub;
        memcpy_P(&sub, &SUBMENUS[_state.menuIndex], sizeof(Submenu));
        memcpy_P(&out, &sub.items[index], sizeof(MenuItem));
    }
}

void Manager::_getParamDescriptorWithDynamicLimits(ParamKey key, ParamDescriptor& out) {
    if (!getParamDescriptor(key, out)) {
        return;
    }
    
    // Динамические ограничения
    if (out.hasDynamicMax) {
        switch (key) {
            case ParamKey::TrainingMoisturePercent:
                // max = SoilMoistureStartPercent - 1
                out.maxValue = static_cast<int16_t>(
                    Settings::Manager::get(Settings::Key::SoilMoistureStartPercent)
                ) - 1;
                if (out.maxValue < 0) out.maxValue = 0;
                break;
                
            case ParamKey::SoilMoistureStartPercent:
                // max = SoilMoistureStopPercent - 1
                out.maxValue = static_cast<int16_t>(
                    Settings::Manager::get(Settings::Key::SoilMoistureStopPercent)
                ) - 1;
                break;
                
            default:
                break;
        }
    }
    
    if (out.hasDynamicMin) {
        switch (key) {
            case ParamKey::SoilMoistureStartPercent:
                // min = TrainingMoisturePercent + 1
                out.minValue = static_cast<int16_t>(
                    Settings::Manager::get(Settings::Key::TrainingMoisturePercent)
                ) + 1;
                if (out.minValue > 99) out.minValue = 99;
                break;
                
            case ParamKey::SoilMoistureStopPercent:
                // min = SoilMoistureStartPercent + 1
                out.minValue = static_cast<int16_t>(
                    Settings::Manager::get(Settings::Key::SoilMoistureStartPercent)
                ) + 1;
                break;
                
            default:
                break;
        }
    }
}

int16_t Manager::_getParamValue(ParamKey key) {
    if (isVirtualParam(key)) {
        // Виртуальные параметры через GrowingCycle
        switch (key) {
            case ParamKey::CurrentDay:
                return static_cast<int16_t>(Irrigation::GrowingCycle::getCurrentDay());
                
            case ParamKey::CurrentHour:
                // Возвращаем минуты от начала дня
                return static_cast<int16_t>(
                    Irrigation::GrowingCycle::getCurrentHour() * 60 +
                    Irrigation::GrowingCycle::getCurrentMinute()
                );
                
            default:
                return 0;
        }
    }
    
    // Обычные параметры через Settings
    return static_cast<int16_t>(Settings::Manager::get(toSettingsKey(key)));
}

bool Manager::_setParamValue(ParamKey key, int16_t value) {
    if (isVirtualParam(key)) {
        // Виртуальные параметры через GrowingCycle
        switch (key) {
            case ParamKey::CurrentDay:
                return Irrigation::GrowingCycle::setCurrentDay(static_cast<uint16_t>(value));
                
            case ParamKey::CurrentHour:
                // Значение в минутах, устанавливаем час
                return Irrigation::GrowingCycle::setCurrentHour(static_cast<uint8_t>(value / 60));
                
            default:
                return false;
        }
    }
    
    // Обычные параметры через Settings
    return Settings::Manager::set(toSettingsKey(key), static_cast<uint32_t>(value));
}

} // namespace Menu
