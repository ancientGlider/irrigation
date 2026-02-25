/*
 * Custom Font API
 *
 * Система кастомных шрифтов для проекта Irrigation.
 * Содержит только используемые символы для экономии Flash-памяти.
 *
 * Поддерживает:
 * - ASCII символы (0x20-0x7F)
 * - Кириллицу (А-Я, а-я)
 * - Специальные символы (°, и др.)
 */

#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

namespace CustomFont {

/**
 * Структура для маппинга специальных символов Unicode
 */
struct SpecialCharMapping {
    uint16_t codepoint;
    uint8_t glyphIndex;
};

} // namespace CustomFont

// Данные шрифтов (после определения SpecialCharMapping)
#include "custom_font_5x8.h"
#include "custom_font_6x10.h"
#include "custom_font_10x20.h"

namespace CustomFont {

/**
 * Размеры шрифтов
 */
enum class FontSize : uint8_t {
    Small,   // 5×8 — основной текст (MainScreen)
    Medium,  // 6×10 — текст меню
    Large    // 10×20 — крупные числа в редактировании
};

/**
 * Получает индекс глифа по Unicode codepoint для шрифта 5x8.
 * @param codepoint Unicode codepoint символа
 * @return Индекс глифа или 0xFF если символ не найден
 */
uint8_t getGlyphIndex5x8(uint16_t codepoint);

/**
 * Получает индекс глифа по Unicode codepoint для шрифта 6x10.
 */
uint8_t getGlyphIndex6x10(uint16_t codepoint);

/**
 * Получает индекс глифа по Unicode codepoint для шрифта 10x20.
 */
uint8_t getGlyphIndex10x20(uint16_t codepoint);

/**
 * Декодирует один UTF-8 символ из строки.
 * @param str Указатель на строку (будет сдвинут на длину символа)
 * @return Unicode codepoint или 0 при ошибке
 */
uint16_t decodeUtf8(const char*& str);

/**
 * Декодирует один UTF-8 символ из PROGMEM строки.
 * @param strP Указатель на PROGMEM строку (будет сдвинут)
 * @return Unicode codepoint или 0 при ошибке
 */
uint16_t decodeUtf8P(const char*& strP);

/**
 * Отрисовывает один глиф на дисплее.
 * @param display Объект U8G2
 * @param x Координата X (левый край глифа)
 * @param y Координата Y (базовая линия)
 * @param glyphIndex Индекс глифа в таблице
 * @param size Размер шрифта
 * @return Ширина отрисованного глифа
 */
uint8_t drawGlyph(U8G2& display, int16_t x, int16_t y, uint8_t glyphIndex, FontSize size);

/**
 * Отрисовывает строку на дисплее.
 * @param display Объект U8G2
 * @param x Координата X (левый край)
 * @param y Координата Y (базовая линия)
 * @param str UTF-8 строка в RAM
 * @param size Размер шрифта
 * @return Ширина отрисованной строки
 */
uint8_t drawString(U8G2& display, int16_t x, int16_t y, const char* str, FontSize size);

/**
 * Отрисовывает строку из PROGMEM на дисплее.
 * @param display Объект U8G2
 * @param x Координата X (левый край)
 * @param y Координата Y (базовая линия)
 * @param strP UTF-8 строка в PROGMEM
 * @param size Размер шрифта
 * @return Ширина отрисованной строки
 */
uint8_t drawStringP(U8G2& display, int16_t x, int16_t y, const char* strP, FontSize size);

/**
 * Вычисляет ширину строки в пикселях.
 * @param str UTF-8 строка в RAM
 * @param size Размер шрифта
 * @return Ширина строки в пикселях
 */
uint8_t getStringWidth(const char* str, FontSize size);

/**
 * Вычисляет ширину PROGMEM строки в пикселях.
 * @param strP UTF-8 строка в PROGMEM
 * @param size Размер шрифта
 * @return Ширина строки в пикселях
 */
uint8_t getStringWidthP(const char* strP, FontSize size);

/**
 * Возвращает ширину символа шрифта.
 * @param size Размер шрифта
 * @return Ширина символа (все шрифты моноширинные)
 */
inline uint8_t getFontWidth(FontSize size) {
    switch (size) {
        case FontSize::Small:  return FONT_5X8_WIDTH;
        case FontSize::Medium: return FONT_6X10_WIDTH;
        case FontSize::Large:  return FONT_10X20_WIDTH;
        default: return FONT_5X8_WIDTH;
    }
}

/**
 * Возвращает высоту символа шрифта.
 * @param size Размер шрифта
 * @return Высота символа
 */
inline uint8_t getFontHeight(FontSize size) {
    switch (size) {
        case FontSize::Small:  return FONT_5X8_HEIGHT;
        case FontSize::Medium: return FONT_6X10_HEIGHT;
        case FontSize::Large:  return FONT_10X20_HEIGHT;
        default: return FONT_5X8_HEIGHT;
    }
}

} // namespace CustomFont
