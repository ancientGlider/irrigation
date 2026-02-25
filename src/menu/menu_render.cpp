/*
 * Menu rendering implementation
 *
 * Реализация отрисовки экранов меню на OLED 128×64.
 * Использует кастомную систему шрифтов для экономии памяти.
 */

#include "menu.h"
#include "../display/custom_font.h"

namespace Menu {

// ============================================================================
// Константы отрисовки
// ============================================================================

constexpr uint8_t HEADER_HEIGHT = 10;
constexpr uint8_t LINE_HEIGHT = 9;
constexpr uint8_t VISIBLE_ITEMS = 5;
constexpr uint8_t LIST_START_Y = 12;
constexpr uint8_t SCROLL_INDICATOR_X = 0;  // Символы прокрутки слева
constexpr uint8_t MENU_TEXT_X = 8;  // Начало текста пунктов меню (с отступом для символов прокрутки)
constexpr uint8_t VALUE_COLUMN_X = 90;
constexpr uint8_t FOOTER_Y = 56;

// ============================================================================
// Основная функция render()
// ============================================================================

void Manager::render() {
    if (!_display || _fsmState == MenuFSMState::Closed) {
        return;
    }
    
    // Page buffer FSM (как в MainScreen)
    if (_needsFirstPage) {
        _display->firstPage();
    }
    
    switch (_fsmState) {
        case MenuFSMState::MainMenu:
            _renderMainMenu();
            break;
            
        case MenuFSMState::Submenu:
            _renderSubmenu();
            break;
            
        case MenuFSMState::Editing:
            _renderEditScreen();
            break;
            
        case MenuFSMState::Confirmation:
            _renderConfirmScreen();
            break;
            
        default:
            break;
    }
    
    _needsFirstPage = !_display->nextPage();
}

// ============================================================================
// Отрисовка главного меню
// ============================================================================

void Manager::_renderMainMenu() {
    // Заголовок "MENU" по центру, инверсия
    _display->setDrawColor(1);
    _display->drawBox(0, 0, 128, HEADER_HEIGHT);
    _display->setDrawColor(0);
    CustomFont::drawString(*_display, 50, 8, "MENU", CustomFont::FontSize::Medium);
    _display->setDrawColor(1);
    
    _renderMenuList(nullptr, MAIN_MENU_COUNT);
}

// ============================================================================
// Отрисовка подменю
// ============================================================================

void Manager::_renderSubmenu() {
    Submenu sub;
    memcpy_P(&sub, &SUBMENUS[_state.menuIndex], sizeof(Submenu));
    
    // Заголовок с инверсией
    char titleBuf[16];
    strcpy_P(titleBuf, sub.title);  // sub.title уже указатель на PROGMEM
    
    _display->setDrawColor(1);
    _display->drawBox(0, 0, 128, HEADER_HEIGHT);
    _display->setDrawColor(0);
    
    // Центрируем заголовок
    uint8_t titleWidth = CustomFont::getStringWidth(titleBuf, CustomFont::FontSize::Medium);
    CustomFont::drawString(*_display, (128 - titleWidth) / 2, 8, titleBuf, CustomFont::FontSize::Medium);
    _display->setDrawColor(1);
    
    _renderMenuList(titleBuf, sub.itemCount);
}

// ============================================================================
// Общая отрисовка списка меню
// ============================================================================

void Manager::_renderMenuList(const char* /* title */, uint8_t itemCount) {
    // Отображаем до VISIBLE_ITEMS пунктов
    for (uint8_t i = 0; i < VISIBLE_ITEMS && (i + _state.scrollOffset) < itemCount; i++) {
        uint8_t itemIndex = i + _state.scrollOffset;
        uint8_t y = LIST_START_Y + i * LINE_HEIGHT;
        
        MenuItem item;
        _getMenuItem(itemIndex, item);
        
        // Формируем строку с префиксом
        char lineBuf[24];
        char labelBuf[16];
        strcpy_P(labelBuf, item.label);  // item.label уже указатель на PROGMEM
        
        // Добавляем спецсимвол в зависимости от типа
        if (item.type == MenuItemType::Action) {
            lineBuf[0] = SYMBOL_ACTION;
            lineBuf[1] = ' ';
            strcpy(lineBuf + 2, labelBuf);
        } else if (item.type == MenuItemType::Submenu) {
            lineBuf[0] = SYMBOL_SUBMENU;
            lineBuf[1] = ' ';
            strcpy(lineBuf + 2, labelBuf);
        } else {
            lineBuf[0] = ' ';
            lineBuf[1] = ' ';
            strcpy(lineBuf + 2, labelBuf);
        }
        
        // Инверсия для выбранного пункта (рисуем ПОСЛЕ текста, чтобы перекрыть)
        // Добавляем 2 пикселя сверху, уменьшаем на 1 снизу
        bool selected = (itemIndex == _state.selectedItem);
        if (selected) {
            _display->drawBox(0, y - 2, 128, LINE_HEIGHT);
            _display->setDrawColor(0);  // Инвертированный цвет для текста
        }
        
        CustomFont::drawString(*_display, MENU_TEXT_X, y + 7, lineBuf, CustomFont::FontSize::Medium);
        
        // Для Value-пунктов показываем текущее значение справа
        if (item.type == MenuItemType::Value) {
            ParamKey key = static_cast<ParamKey>(item.data);
            int16_t value = _getParamValue(key);
            
            ParamDescriptor desc;
            getParamDescriptor(key, desc);
            
            char valueBuf[12];
            _formatValue(value, desc.displayMode, valueBuf, sizeof(valueBuf));
            
            uint8_t valueWidth = CustomFont::getStringWidth(valueBuf, CustomFont::FontSize::Medium);
            CustomFont::drawString(*_display, 126 - valueWidth, y + 7, valueBuf, CustomFont::FontSize::Medium);
        }
        
        if (selected) {
            _display->setDrawColor(1);  // Возвращаем нормальный цвет
        }
    }
    
    // Индикатор прокрутки (стрелки вверх/вниз) - слева
    if (_state.scrollOffset > 0) {
        CustomFont::drawString(*_display, SCROLL_INDICATOR_X, LIST_START_Y + 7, "^", CustomFont::FontSize::Medium);
    }
    if (_state.scrollOffset + VISIBLE_ITEMS < itemCount) {
        CustomFont::drawString(*_display, SCROLL_INDICATOR_X, LIST_START_Y + (VISIBLE_ITEMS - 1) * LINE_HEIGHT + 7, "v", CustomFont::FontSize::Medium);
    }
}

// ============================================================================
// Отрисовка экрана редактирования
// ============================================================================

void Manager::_renderEditScreen() {
    ParamDescriptor desc;
    _getParamDescriptorWithDynamicLimits(_state.currentKey, desc);
    
    // Заголовок
    _display->setDrawColor(1);
    _display->drawBox(0, 0, 128, HEADER_HEIGHT);
    _display->setDrawColor(0);
    
    // Получаем название параметра из текущего пункта меню
    MenuItem item;
    _getMenuItem(_state.selectedItem, item);
    char labelBuf[16];
    strcpy_P(labelBuf, item.label);  // item.label уже указатель на PROGMEM
    
    uint8_t labelWidth = CustomFont::getStringWidth(labelBuf, CustomFont::FontSize::Medium);
    CustomFont::drawString(*_display, (128 - labelWidth) / 2, 8, labelBuf, CustomFont::FontSize::Medium);
    _display->setDrawColor(1);
    
    // Значение по центру (Large для чисел, Medium для текста)
    char valueBuf[12];
    _formatValue(_state.editValue, desc.displayMode, valueBuf, sizeof(valueBuf));
    
    // Для AsEnum используем Medium (там кириллица), для остальных - Large
    CustomFont::FontSize valueFontSize = (desc.displayMode == DisplayMode::AsEnum) 
                                         ? CustomFont::FontSize::Medium 
                                         : CustomFont::FontSize::Large;
    
    uint8_t valueWidth = CustomFont::getStringWidth(valueBuf, valueFontSize);
    uint8_t arrowWidth = CustomFont::getStringWidth("<", valueFontSize);
    uint8_t valueY = desc.hasLiveValue ? 28 : 35;
    
    // Стрелки < > (используем тот же размер шрифта)
    // Отступы: стрелки по краям, значение по центру
    uint8_t leftArrowX = 2;
    uint8_t valueX = (128 - valueWidth) / 2;
    uint8_t rightArrowX = 128 - arrowWidth - 2;
    
    // Проверяем, не перекрываются ли стрелки и значение
    if (leftArrowX + arrowWidth + 2 > valueX) {
        // Слишком близко - уменьшаем отступы
        leftArrowX = valueX - arrowWidth - 2;
    }
    if (valueX + valueWidth + 2 > rightArrowX) {
        // Слишком близко - корректируем
        valueX = rightArrowX - valueWidth - 2;
        if (valueX < leftArrowX + arrowWidth + 2) {
            // Не влезает - используем минимальные отступы
            leftArrowX = 0;
            valueX = arrowWidth + 2;
            rightArrowX = 128 - arrowWidth;
        }
    }
    
    CustomFont::drawString(*_display, leftArrowX, valueY, "<", valueFontSize);
    CustomFont::drawString(*_display, valueX, valueY, valueBuf, valueFontSize);
    CustomFont::drawString(*_display, rightArrowX, valueY, ">", valueFontSize);
    
    // Live-значение датчика (если есть)
    if (desc.hasLiveValue && _liveValueCallback) {
        int16_t liveValue = _liveValueCallback();
        char liveBuf[20];
        
        // Форматируем "Curr: XXX"
        strcpy(liveBuf, "Curr: ");
        itoa(liveValue, liveBuf + 6, 10);
        
        uint8_t liveWidth = CustomFont::getStringWidth(liveBuf, CustomFont::FontSize::Medium);
        CustomFont::drawString(*_display, (128 - liveWidth) / 2, 42, liveBuf, CustomFont::FontSize::Medium);
    }
    
    // Границы min/max (используем тот же формат, что и значение)
    char minBuf[16], maxBuf[16];
    strcpy(minBuf, "min:");
    char minValueBuf[12];
    _formatValue(desc.minValue, desc.displayMode, minValueBuf, sizeof(minValueBuf));
    strcat(minBuf, minValueBuf);
    
    strcpy(maxBuf, "max:");
    char maxValueBuf[12];
    _formatValue(desc.maxValue, desc.displayMode, maxValueBuf, sizeof(maxValueBuf));
    strcat(maxBuf, maxValueBuf);
    
    // Проверяем ширину и размещаем на разных строках, если не влезает
    uint8_t minWidth = CustomFont::getStringWidth(minBuf, CustomFont::FontSize::Small);
    uint8_t maxWidth = CustomFont::getStringWidth(maxBuf, CustomFont::FontSize::Small);
    
    if (minWidth + maxWidth + 4 > 128) {
        // Не влезает - на разных строках
        CustomFont::drawString(*_display, 2, 52, minBuf, CustomFont::FontSize::Small);
        CustomFont::drawString(*_display, 2, 60, maxBuf, CustomFont::FontSize::Small);
    } else {
        // Влезает - на одной строке
        CustomFont::drawString(*_display, 2, 52, minBuf, CustomFont::FontSize::Small);
        CustomFont::drawString(*_display, 128 - maxWidth, 52, maxBuf, CustomFont::FontSize::Small);
    }
}

// ============================================================================
// Отрисовка экрана подтверждения
// ============================================================================

void Manager::_renderConfirmScreen() {
    // Заголовок
    _display->setDrawColor(1);
    _display->drawBox(0, 0, 128, HEADER_HEIGHT);
    _display->setDrawColor(0);
    CustomFont::drawString(*_display, 30, 8, "CONFIRM", CustomFont::FontSize::Medium);
    _display->setDrawColor(1);
    
    // Вопрос
    char questionBuf[16];
    strcpy_P(questionBuf, STR_RESET_CONFIRM);
    uint8_t qWidth = CustomFont::getStringWidth(questionBuf, CustomFont::FontSize::Medium);
    CustomFont::drawString(*_display, (128 - qWidth) / 2, 28, questionBuf, CustomFont::FontSize::Medium);
    
    // Варианты ДА / НЕТ
    char yesBuf[8], noBuf[8];
    strcpy_P(yesBuf, STR_YES);
    strcpy_P(noBuf, STR_NO);
    
    // Подсветка выбранного
    if (_confirmYes) {
        _display->drawBox(20, 38, 40, 12);
        _display->setDrawColor(0);
        CustomFont::drawString(*_display, 30, 48, yesBuf, CustomFont::FontSize::Medium);
        _display->setDrawColor(1);
        CustomFont::drawString(*_display, 80, 48, noBuf, CustomFont::FontSize::Medium);
    } else {
        CustomFont::drawString(*_display, 30, 48, yesBuf, CustomFont::FontSize::Medium);
        _display->drawBox(68, 38, 40, 12);
        _display->setDrawColor(0);
        CustomFont::drawString(*_display, 80, 48, noBuf, CustomFont::FontSize::Medium);
        _display->setDrawColor(1);
    }
}

// ============================================================================
// Форматирование значений
// ============================================================================

void Manager::_formatValue(int16_t value, DisplayMode mode, char* buf, uint8_t bufSize) {
    switch (mode) {
        case DisplayMode::AsNumber:
            itoa(value, buf, 10);
            break;
            
        case DisplayMode::AsMinutes:
            // Секунды → минуты
            itoa(value / 60, buf, 10);
            strcat(buf, "m");
            break;
            
        case DisplayMode::AsPercent:
            itoa(value, buf, 10);
            strcat(buf, "%");
            break;
            
        case DisplayMode::AsEnum:
            // Для SystemMode
            if (value >= 0 && value < MODE_NAMES_COUNT) {
                const char* modeNamePtr;
                memcpy_P(&modeNamePtr, &MODE_NAMES[value], sizeof(const char*));
                strcpy_P(buf, modeNamePtr);
            } else {
                buf[0] = '?';
                itoa(value, buf + 1, 10);
            }
            break;
            
        case DisplayMode::AsTimeHHMM:
            // Минуты → HH:MM
            {
                uint8_t hours = value / 60;
                uint8_t mins = value % 60;
                buf[0] = '0' + (hours / 10);
                buf[1] = '0' + (hours % 10);
                buf[2] = ':';
                buf[3] = '0' + (mins / 10);
                buf[4] = '0' + (mins % 10);
                buf[5] = '\0';
            }
            break;
            
        default:
            itoa(value, buf, 10);
            break;
    }
}

} // namespace Menu
