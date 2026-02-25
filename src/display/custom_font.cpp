/*
 * Custom Font Implementation
 *
 * Реализация системы кастомных шрифтов.
 */

#include "custom_font.h"

namespace CustomFont {

// ============================================================================
// UTF-8 декодирование
// ============================================================================

uint16_t decodeUtf8(const char*& str) {
    if (!str || !*str) return 0;
    
    uint8_t c = static_cast<uint8_t>(*str++);
    
    // ASCII (0x00-0x7F)
    if (c < 0x80) {
        return c;
    }
    
    // 2-byte sequence (0xC0-0xDF)
    if ((c & 0xE0) == 0xC0) {
        uint8_t c2 = static_cast<uint8_t>(*str++);
        return ((c & 0x1F) << 6) | (c2 & 0x3F);
    }
    
    // 3-byte sequence (0xE0-0xEF)
    if ((c & 0xF0) == 0xE0) {
        uint8_t c2 = static_cast<uint8_t>(*str++);
        uint8_t c3 = static_cast<uint8_t>(*str++);
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    
    // Skip 4-byte sequences and invalid bytes
    return '?';
}

uint16_t decodeUtf8P(const char*& strP) {
    if (!strP) return 0;
    
    uint8_t c = pgm_read_byte(strP++);
    if (c == 0) return 0;
    
    // ASCII (0x00-0x7F)
    if (c < 0x80) {
        return c;
    }
    
    // 2-byte sequence (0xC0-0xDF)
    if ((c & 0xE0) == 0xC0) {
        uint8_t c2 = pgm_read_byte(strP++);
        return ((c & 0x1F) << 6) | (c2 & 0x3F);
    }
    
    // 3-byte sequence (0xE0-0xEF)
    if ((c & 0xF0) == 0xE0) {
        uint8_t c2 = pgm_read_byte(strP++);
        uint8_t c3 = pgm_read_byte(strP++);
        return ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    }
    
    return '?';
}

// ============================================================================
// Lookup функции
// ============================================================================

uint8_t getGlyphIndex5x8(uint16_t codepoint) {
    // ASCII (0x20-0x7F)
    if (codepoint >= 0x20 && codepoint < 0x80) {
        return pgm_read_byte(&FONT_5X8_ASCII_MAP[codepoint - 0x20]);
    }
    
    // Кириллица заглавная А-Я (0x0410-0x042F)
    if (codepoint >= 0x0410 && codepoint < 0x0430) {
        return pgm_read_byte(&FONT_5X8_CYR_UPPER_MAP[codepoint - 0x0410]);
    }
    
    // Кириллица строчная а-я (0x0430-0x0450)
    if (codepoint >= 0x0430 && codepoint < 0x0450) {
        return pgm_read_byte(&FONT_5X8_CYR_LOWER_MAP[codepoint - 0x0430]);
    }
    
    // Специальные символы
    #ifdef FONT_5X8_SPECIAL_COUNT
    for (uint8_t i = 0; i < FONT_5X8_SPECIAL_COUNT; i++) {
        SpecialCharMapping mapping;
        memcpy_P(&mapping, &FONT_5X8_SPECIAL_MAP[i], sizeof(mapping));
        if (mapping.codepoint == codepoint) {
            return mapping.glyphIndex;
        }
    }
    #endif
    
    return 0xFF; // Символ не найден
}

uint8_t getGlyphIndex6x10(uint16_t codepoint) {
    // ASCII (0x20-0x7F)
    if (codepoint >= 0x20 && codepoint < 0x80) {
        return pgm_read_byte(&FONT_6X10_ASCII_MAP[codepoint - 0x20]);
    }
    
    // Кириллица заглавная А-Я (0x0410-0x042F)
    if (codepoint >= 0x0410 && codepoint < 0x0430) {
        return pgm_read_byte(&FONT_6X10_CYR_UPPER_MAP[codepoint - 0x0410]);
    }
    
    // Кириллица строчная а-я (0x0430-0x0450)
    if (codepoint >= 0x0430 && codepoint < 0x0450) {
        return pgm_read_byte(&FONT_6X10_CYR_LOWER_MAP[codepoint - 0x0430]);
    }
    
    // Специальные символы
    #ifdef FONT_6X10_SPECIAL_COUNT
    for (uint8_t i = 0; i < FONT_6X10_SPECIAL_COUNT; i++) {
        SpecialCharMapping mapping;
        memcpy_P(&mapping, &FONT_6X10_SPECIAL_MAP[i], sizeof(mapping));
        if (mapping.codepoint == codepoint) {
            return mapping.glyphIndex;
        }
    }
    #endif
    
    return 0xFF;
}

uint8_t getGlyphIndex10x20(uint16_t codepoint) {
    // ASCII (0x20-0x7F)
    if (codepoint >= 0x20 && codepoint < 0x80) {
        return pgm_read_byte(&FONT_10X20_ASCII_MAP[codepoint - 0x20]);
    }
    
    // Кириллица заглавная А-Я (0x0410-0x042F)
    if (codepoint >= 0x0410 && codepoint < 0x0430) {
        return pgm_read_byte(&FONT_10X20_CYR_UPPER_MAP[codepoint - 0x0410]);
    }
    
    // Кириллица строчная а-я (0x0430-0x0450)
    if (codepoint >= 0x0430 && codepoint < 0x0450) {
        return pgm_read_byte(&FONT_10X20_CYR_LOWER_MAP[codepoint - 0x0430]);
    }
    
    return 0xFF;
}

// ============================================================================
// Отрисовка глифов
// ============================================================================

uint8_t drawGlyph(U8G2& display, int16_t x, int16_t y, uint8_t glyphIndex, FontSize size) {
    const uint8_t* bitmap = nullptr;
    uint8_t width = 0;
    uint8_t height = 0;
    
    switch (size) {
        case FontSize::Small:
            if (glyphIndex >= FONT_5X8_GLYPH_COUNT) return 0;
            bitmap = reinterpret_cast<const uint8_t*>(pgm_read_ptr(&FONT_5X8_GLYPHS[glyphIndex]));
            width = FONT_5X8_WIDTH;
            height = FONT_5X8_HEIGHT;
            break;
            
        case FontSize::Medium:
            if (glyphIndex >= FONT_6X10_GLYPH_COUNT) return 0;
            bitmap = reinterpret_cast<const uint8_t*>(pgm_read_ptr(&FONT_6X10_GLYPHS[glyphIndex]));
            width = FONT_6X10_WIDTH;
            height = FONT_6X10_HEIGHT;
            break;
            
        case FontSize::Large:
            if (glyphIndex >= FONT_10X20_GLYPH_COUNT) return 0;
            bitmap = reinterpret_cast<const uint8_t*>(pgm_read_ptr(&FONT_10X20_GLYPHS[glyphIndex]));
            width = FONT_10X20_WIDTH;
            height = FONT_10X20_HEIGHT;
            break;
            
        default:
            return 0;
    }
    
    if (!bitmap) return 0;
    
    // Отрисовка bitmap
    // Координата Y — базовая линия (нижний край), нужно сместить вверх
    // drawXBMP рисует от верхнего левого угла
    int16_t drawY = y - height + 1;
    
    // Используем U8g2 drawXBMP для отрисовки из PROGMEM
    display.drawXBMP(x, drawY, width, height, bitmap);
    
    return width;
}

// ============================================================================
// Отрисовка строк
// ============================================================================

uint8_t drawString(U8G2& display, int16_t x, int16_t y, const char* str, FontSize size) {
    if (!str) return 0;
    
    int16_t startX = x;
    uint8_t fontWidth = getFontWidth(size);
    // Интервал между символами: 1 для Small/Medium, 0 для Large (большие символы и так широкие)
    uint8_t spacing = (size == FontSize::Large) ? 0 : 1;
    
    const char* strPtr = str;
    bool firstChar = true;
    while (*strPtr) {
        uint16_t codepoint = decodeUtf8(strPtr);
        if (codepoint == 0) break;
        
        uint8_t glyphIdx = 0xFF;
        switch (size) {
            case FontSize::Small:  glyphIdx = getGlyphIndex5x8(codepoint); break;
            case FontSize::Medium: glyphIdx = getGlyphIndex6x10(codepoint); break;
            case FontSize::Large:  glyphIdx = getGlyphIndex10x20(codepoint); break;
        }
        
        if (glyphIdx != 0xFF) {
            drawGlyph(display, x, y, glyphIdx, size);
        }
        
        // Для первого символа не добавляем интервал, для остальных - добавляем
        if (!firstChar) {
            x += spacing;
        }
        x += fontWidth;
        firstChar = false;
    }
    
    return static_cast<uint8_t>(x - startX);
}

uint8_t drawStringP(U8G2& display, int16_t x, int16_t y, const char* strP, FontSize size) {
    if (!strP) return 0;
    
    int16_t startX = x;
    uint8_t fontWidth = getFontWidth(size);
    // Интервал между символами: 1 для Small/Medium, 0 для Large
    uint8_t spacing = (size == FontSize::Large) ? 0 : 1;
    
    bool firstChar = true;
    while (true) {
        uint16_t codepoint = decodeUtf8P(strP);
        if (codepoint == 0) break;
        
        uint8_t glyphIdx = 0xFF;
        switch (size) {
            case FontSize::Small:  glyphIdx = getGlyphIndex5x8(codepoint); break;
            case FontSize::Medium: glyphIdx = getGlyphIndex6x10(codepoint); break;
            case FontSize::Large:  glyphIdx = getGlyphIndex10x20(codepoint); break;
        }
        
        if (glyphIdx != 0xFF) {
            drawGlyph(display, x, y, glyphIdx, size);
        }
        
        // Для первого символа не добавляем интервал, для остальных - добавляем
        if (!firstChar) {
            x += spacing;
        }
        x += fontWidth;
        firstChar = false;
    }
    
    return static_cast<uint8_t>(x - startX);
}

// ============================================================================
// Вычисление ширины
// ============================================================================

uint8_t getStringWidth(const char* str, FontSize size) {
    if (!str) return 0;
    
    uint8_t count = 0;
    const char* strPtr = str;
    while (*strPtr) {
        uint16_t codepoint = decodeUtf8(strPtr);
        if (codepoint == 0) break;
        count++;
    }
    
    if (count == 0) return 0;
    
    uint8_t fontWidth = getFontWidth(size);
    // Интервал между символами: 1 для Small/Medium, 0 для Large
    uint8_t spacing = (size == FontSize::Large) ? 0 : 1;
    
    // Ширина = (количество символов - 1) * интервал + количество символов * ширина
    return (count - 1) * spacing + count * fontWidth;
}

uint8_t getStringWidthP(const char* strP, FontSize size) {
    if (!strP) return 0;
    
    uint8_t count = 0;
    const char* strPtr = strP;
    while (true) {
        uint16_t codepoint = decodeUtf8P(strPtr);
        if (codepoint == 0) break;
        count++;
    }
    
    if (count == 0) return 0;
    
    uint8_t fontWidth = getFontWidth(size);
    // Интервал между символами: 1 для Small/Medium, 0 для Large
    uint8_t spacing = (size == FontSize::Large) ? 0 : 1;
    
    // Ширина = (количество символов - 1) * интервал + количество символов * ширина
    return (count - 1) * spacing + count * fontWidth;
}

} // namespace CustomFont
