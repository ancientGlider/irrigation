/*
 * Main screen renderer implementation
 *
 * Реализация рендерера главного экрана.
 * Использует кастомную систему шрифтов для экономии памяти.
 */

#include "main_screen.h"
#include "main_screen_icons.h"
#include "main_screen_labels.h"
#include "main_screen_fonts.h"
#include "custom_font.h"
#include <stddef.h>  // для offsetof

namespace Display {

// Определение статических переменных-членов класса
U8G2* MainScreen::_display = nullptr;
const Irrigation::SystemState* MainScreen::_systemState = nullptr;
bool MainScreen::_needsFirstPage = true;
bool MainScreen::_blinkState = true;
Timer MainScreen::_blinkTimer = Timer();

void MainScreen::begin(U8G2& display, const Irrigation::SystemState* systemState) {
    _display = &display;
    _systemState = systemState;
    _blinkTimer.setPeriod(BLINK_PERIOD_MS);
    
    // Устанавливаем цвет по умолчанию (1 - черный на белом фоне)
    _display->setDrawColor(1);
    
    #ifdef SERIAL_DEBUG
    Serial.println(F("MainScreen: initialized"));
    #endif
}

void MainScreen::update() {
    if (!_display || !_systemState) {
        return;
    }
    
    #ifdef SERIAL_DEBUG
    Serial.println(F("MainScreen: update() called"));
    #endif
    
    // Вычисляем состояние мигания двоеточия ДО начала цикла отрисовки
    if (_blinkTimer.isReady()) {
        _blinkState = !_blinkState;
    }
    
    // Используем page buffer режим U8g2
    if (_needsFirstPage) {
        _display->firstPage();
    }
    
    // Полная перерисовка всех блоков
    _renderFirstBlock();
    _renderSecondBlock();
    _renderThirdBlock();
    _renderBottomIcons();
        
    _needsFirstPage = !_display->nextPage();
}

void MainScreen::_renderFirstBlock() {
    // Отрисовка режима работы системы
    _renderMode();
    
    // Отрисовка иконки освещения и времени
    _renderLightAndTime();
    
    // Отрисовка оповещений (помпа, внимание)
    _renderAlerts();
    
    // Разделительная линия
    _display->drawHLine(0, 9, 128);
}

void MainScreen::_renderMode() {
    _display->setDrawColor(1);
    
    uint8_t modeIndex = static_cast<uint8_t>(_systemState->systemMode);
    if (modeIndex > 3) {
        modeIndex = 0;
    }
    
    const char* modeLabel = Labels::getModeLabel(modeIndex);
    if (modeLabel == nullptr) {
        // Ошибка - выводим "ERR"
        _display->drawBox(0, 0, 30, 8);
        _display->setDrawColor(0);
        CustomFont::drawString(*_display, 2, 7, "ERR", CustomFont::FontSize::Small);
        _display->setDrawColor(1);
        return;
    }
    
    // Рисуем залитый прямоугольник (инвертированный фон)
    _display->drawBox(0, 0, 30, 8);
    _display->setDrawColor(0);
    CustomFont::drawStringP(*_display, 2, 7, modeLabel, CustomFont::FontSize::Small);
    _display->setDrawColor(1);
}

void MainScreen::_renderLightAndTime() {
    Icons::Icon lightIcon = _systemState->lightOn ? Icons::Icon::LightOn : Icons::Icon::LightOff;
    Icons::drawIconByEnum(*_display, lightIcon, 44, 0);
    Fonts::drawTime(*_display, 65, 7, _systemState->currentHour, _systemState->currentMinute, _blinkState);
}

void MainScreen::_renderAlerts() {
    // Отрисовка иконки помпы при активной помпе
    if (_systemState->pumpActive) {
        Icons::drawIconByEnum(*_display, Icons::Icon::PumpOn, 113, 0);
    }
    
    // Определяем наличие предупреждения из состояния полива
    bool hasAttention = (_systemState->wateringState == Irrigation::WateringState::TrainingReady ||
                         _systemState->wateringState == Irrigation::WateringState::OutOfWater);
    
    // Отрисовка мигающего "!" при наличии предупреждения
    if (hasAttention) {
        if (_blinkState) {
            _display->drawBox(122, 0, 6, 8);
            _display->setDrawColor(0);
            CustomFont::drawString(*_display, 122, 7, "!", CustomFont::FontSize::Small);
            _display->setDrawColor(1);
        } else {
            _display->setDrawColor(1);
            CustomFont::drawString(*_display, 122, 7, "!", CustomFont::FontSize::Small);
        }
    }
}

void MainScreen::_renderWateringState() {
    // Маппинг WateringState в индекс WATERING_CAPTIONS
    uint8_t captionIndex = 1;
    
    switch (_systemState->wateringState) {
        case Irrigation::WateringState::Stopping:
            captionIndex = 0;
            break;
        case Irrigation::WateringState::Idle:
            captionIndex = 1;
            break;
        case Irrigation::WateringState::AutoPause:
        case Irrigation::WateringState::AutoWatering:
            captionIndex = 2;
            break;
        case Irrigation::WateringState::TrainingWaiting:
            captionIndex = 3;
            break;
        case Irrigation::WateringState::TrainingReady:
            captionIndex = 4;
            break;
        case Irrigation::WateringState::OutOfWater:
            captionIndex = 5;
            break;
        default:
            captionIndex = 1;
            break;
    }
    
    if (captionIndex >= sizeof(Labels::WATERING_CAPTIONS) / sizeof(Labels::WATERING_CAPTIONS[0])) {
        captionIndex = 1;
    }
    
    const Labels::Caption* captionPtr = &Labels::WATERING_CAPTIONS[captionIndex];
    
    const char* line1Ptr = (const char*)pgm_read_ptr((const void*)((uintptr_t)captionPtr + offsetof(Labels::Caption, line1)));
    const char* line2Ptr = (const char*)pgm_read_ptr((const void*)((uintptr_t)captionPtr + offsetof(Labels::Caption, line2)));
    
    // Отображаем первую строку
    if (line1Ptr) {
        CustomFont::drawStringP(*_display, 2, 18, line1Ptr, CustomFont::FontSize::Small);
    }
    
    // Отображаем вторую строку (если есть)
    if (line2Ptr) {
        CustomFont::drawStringP(*_display, 2, 27, line2Ptr, CustomFont::FontSize::Small);
    }
}

void MainScreen::_renderSensorCountdown() {
    uint32_t remainingSeconds = _systemState->wateringRemainingSeconds;
    
    Icons::drawIconByEnum(*_display, Icons::Icon::Magnifier, 2, 29);
    
    uint16_t totalMinutes = static_cast<uint16_t>(remainingSeconds / 60UL);
    if (totalMinutes > 9999) {
        totalMinutes = 9999;
    }
    uint8_t seconds = remainingSeconds % 60;
    
    // Форматируем время MM:SS
    char buffer[8];
    uint8_t idx = 0;
    
    // Минуты
    if (totalMinutes < 10) {
        buffer[idx++] = '0';
    }
    if (totalMinutes >= 1000) {
        buffer[idx++] = '0' + (totalMinutes / 1000);
        totalMinutes %= 1000;
        buffer[idx++] = '0' + (totalMinutes / 100);
        totalMinutes %= 100;
    } else if (totalMinutes >= 100) {
        buffer[idx++] = '0' + (totalMinutes / 100);
        totalMinutes %= 100;
    }
    buffer[idx++] = '0' + (totalMinutes / 10);
    buffer[idx++] = '0' + (totalMinutes % 10);
    
    buffer[idx++] = ':';
    
    // Секунды
    buffer[idx++] = '0' + (seconds / 10);
    buffer[idx++] = '0' + (seconds % 10);
    buffer[idx] = '\0';
    
    CustomFont::drawString(*_display, 18, 36, buffer, CustomFont::FontSize::Small);
}

void MainScreen::_renderSensorData() {
    // Температура воздуха
    Icons::drawIconByEnum(*_display, Icons::Icon::Thermometer, 66, 11);
    Icons::drawIconByEnum(*_display, Icons::Icon::Air, 76, 11);
    Fonts::drawTemperature(*_display, 88, 18, _systemState->sensors.airTemperature);
    
    // Влажность воздуха
    Icons::drawIconByEnum(*_display, Icons::Icon::Drop, 66, 20);
    Icons::drawIconByEnum(*_display, Icons::Icon::Air, 76, 20);
    
    uint8_t airHumidityPercent = _systemState->sensors.airHumidity / 10;
    char buffer[5];
    buffer[0] = '0' + (airHumidityPercent / 10);
    buffer[1] = '0' + (airHumidityPercent % 10);
    buffer[2] = '%';
    buffer[3] = '\0';
    CustomFont::drawString(*_display, 88, 27, buffer, CustomFont::FontSize::Small);
    
    // Влажность почвы
    Icons::drawIconByEnum(*_display, Icons::Icon::Drop, 66, 29);
    Icons::drawIconByEnum(*_display, Icons::Icon::Earth, 76, 29);
    
    buffer[0] = '0' + (_systemState->sensors.soilHumidity / 10);
    buffer[1] = '0' + (_systemState->sensors.soilHumidity % 10);
    buffer[2] = '%';
    buffer[3] = '\0';
    CustomFont::drawString(*_display, 88, 36, buffer, CustomFont::FontSize::Small);
    
    _display->setDrawColor(1);
}

void MainScreen::_renderSecondBlock() {
    _renderWateringState();
    _renderSensorCountdown();
    _renderSensorData();
    _display->drawHLine(0, 38, 128);
}

void MainScreen::_renderCycleInfo() {
    if (_systemState->systemMode != Settings::SystemMode::Growing) {
        return;
    }
    
    uint8_t periodIndex = static_cast<uint8_t>(_systemState->period);
    if (periodIndex > 4) {
        periodIndex = 0;
    }
    
    const char* seasonLabel = Labels::getSeasonLabel(periodIndex);
    if (seasonLabel == nullptr) {
        return;
    }
    
    // Отображаем название сезона
    CustomFont::drawStringP(*_display, 0, 47, seasonLabel, CustomFont::FontSize::Small);
    
    // "ДЕНЬ:" и счётчик дней
    // Используем кириллицу из наших шрифтов
    CustomFont::drawString(*_display, 63, 47, "ДЕНЬ:", CustomFont::FontSize::Small);
    
    // Форматируем день/всего
    char buffer[10];
    uint16_t currentDay = _systemState->currentDay;
    uint16_t totalDays = _systemState->totalDays;
    
    uint8_t idx = 0;
    
    // currentDay (с выравниванием)
    if (currentDay < 10) buffer[idx++] = ' ';
    if (currentDay < 100) buffer[idx++] = ' ';
    if (currentDay >= 100) buffer[idx++] = '0' + (currentDay / 100);
    if (currentDay >= 10) buffer[idx++] = '0' + ((currentDay / 10) % 10);
    buffer[idx++] = '0' + (currentDay % 10);
    
    buffer[idx++] = '/';
    
    // totalDays
    if (totalDays >= 100) buffer[idx++] = '0' + (totalDays / 100);
    if (totalDays >= 10) buffer[idx++] = '0' + ((totalDays / 10) % 10);
    buffer[idx++] = '0' + (totalDays % 10);
    buffer[idx] = '\0';
    
    CustomFont::drawString(*_display, 93, 47, buffer, CustomFont::FontSize::Small);
    
    _display->setDrawColor(1);
}

void MainScreen::_renderBottomIcons() {
    Icons::drawIconByEnum(*_display, Icons::Icon::Settings, 0, 51);
    Icons::drawIconByEnum(*_display, Icons::Icon::Watering, 39, 51);
    Icons::drawIconByEnum(*_display, Icons::Icon::Check, 77, 51);
    Icons::drawIconByEnum(*_display, Icons::Icon::OnOff, 115, 51);
    _display->setDrawColor(1);
}

void MainScreen::_renderThirdBlock() {
    _renderCycleInfo();
    _display->drawHLine(0, 49, 128);
    _renderBottomIcons();
}

}  // namespace Display
