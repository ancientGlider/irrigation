/*
 * Fonts and special characters for main screen display
 *
 * Обёртки над кастомной системой шрифтов для совместимости с существующим API.
 * Используют CustomFont вместо тяжёлых U8g2 шрифтов.
 */

#pragma once

#include <Arduino.h>
#include <U8g2lib.h>
#include "custom_font.h"

namespace Display::Fonts {

/**
 * Устанавливает основной шрифт (заглушка для совместимости).
 * Теперь используется CustomFont, поэтому эта функция не меняет шрифт U8g2.
 */
inline void setMainFont(U8G2& /* u8g2 */) {
    // Ничего не делаем - используем CustomFont::drawString() вместо u8g2.print()
}

/**
 * Устанавливает шрифт для заголовков (заглушка для совместимости).
 */
inline void setHeaderFont(U8G2& /* u8g2 */) {
    // Ничего не делаем
}

/**
 * Проверяет поддержку кириллицы.
 */
inline bool supportsCyrillic() {
    return true;
}

/**
 * Отображает двоеточие.
 * Используется для обратной совместимости в местах, где вызывается напрямую.
 */
inline void drawColon(U8G2& u8g2) {
    // Используем ASCII код двоеточия
    uint8_t glyphIdx = CustomFont::getGlyphIndex5x8(':');
    if (glyphIdx != 0xFF) {
        // Получаем текущую позицию курсора
        int16_t x = u8g2.tx;
        int16_t y = u8g2.ty;
        CustomFont::drawGlyph(u8g2, x, y, glyphIdx, CustomFont::FontSize::Small);
        u8g2.tx += CustomFont::FONT_5X8_WIDTH;
    }
}

/**
 * Форматирует и отображает время в формате HH:MM.
 */
inline void drawTime(U8G2& u8g2, int16_t x, int16_t y, uint8_t hour, uint8_t minute, bool showColon = true) {
    char buffer[6];
    
    // Форматируем время
    buffer[0] = '0' + (hour / 10);
    buffer[1] = '0' + (hour % 10);
    buffer[2] = showColon ? ':' : ' ';
    buffer[3] = '0' + (minute / 10);
    buffer[4] = '0' + (minute % 10);
    buffer[5] = '\0';
    
    CustomFont::drawString(u8g2, x, y, buffer, CustomFont::FontSize::Small);
}

/**
 * Форматирует и отображает время в формате MM:SS.
 */
inline void drawTimeMMSS(U8G2& u8g2, int16_t x, int16_t y, uint32_t totalSeconds) {
    char buffer[6];
    
    uint8_t minutes = totalSeconds / 60;
    uint8_t seconds = totalSeconds % 60;
    
    buffer[0] = '0' + (minutes / 10);
    buffer[1] = '0' + (minutes % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (seconds / 10);
    buffer[4] = '0' + (seconds % 10);
    buffer[5] = '\0';
    
    CustomFont::drawString(u8g2, x, y, buffer, CustomFont::FontSize::Small);
}

/**
 * Форматирует и отображает температуру в формате xx.x°C.
 */
inline void drawTemperature(U8G2& u8g2, int16_t x, int16_t y, int16_t temperatureDeciC) {
    char buffer[10];
    
    int16_t whole = temperatureDeciC / 10;
    int16_t fraction = temperatureDeciC % 10;
    if (fraction < 0) fraction = -fraction;
    
    // Форматируем: "XX.X°C"
    uint8_t idx = 0;
    
    // Целая часть с ведущим нулём
    if (whole < 0) {
        buffer[idx++] = '-';
        whole = -whole;
    }
    if (whole < 10) {
        buffer[idx++] = '0';
    }
    if (whole >= 10) {
        buffer[idx++] = '0' + (whole / 10);
    }
    buffer[idx++] = '0' + (whole % 10);
    
    // Точка
    buffer[idx++] = '.';
    
    // Дробная часть
    buffer[idx++] = '0' + fraction;
    
    // Градус и C (используем ° из нашего шрифта или просто 'C')
    buffer[idx++] = 'C';
    buffer[idx] = '\0';
    
    CustomFont::drawString(u8g2, x, y, buffer, CustomFont::FontSize::Small);
}

/**
 * Форматирует и отображает процент влажности.
 */
inline void drawHumidity(U8G2& u8g2, int16_t x, int16_t y, uint8_t humidity) {
    char buffer[5];
    
    buffer[0] = '0' + (humidity / 10);
    buffer[1] = '0' + (humidity % 10);
    buffer[2] = '%';
    buffer[3] = '\0';
    
    CustomFont::drawString(u8g2, x, y, buffer, CustomFont::FontSize::Small);
}

}  // namespace Display::Fonts
