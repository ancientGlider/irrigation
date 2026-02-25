/*
 * Icons for main screen display
 *
 * Generated from tools/display/main_screen_icons.json
 * Format: 1 bit per pixel, packed into bytes (LSB = left pixel for U8g2 drawXBMP)
 * All icons stored in PROGMEM to save SRAM
 */

#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

namespace Display::Icons {

enum class Icon : uint8_t {
    Watering,
    Settings,
    Check,
    OnOff,
    LightOn,
    LightOff,
    Drop,
    Water,
    Air,
    Earth,
    Thermometer,
    PumpOn,
    Magnifier
};

struct IconData {
    const uint8_t* data;
    uint8_t width;
    uint8_t height;
};

// Icon: watering (13x13)
static const uint8_t ICON_WATERING_DATA[] PROGMEM = {
    0xA0, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xB0, 0x01, 0x0C, 0x06, 0x02, 0x08, 0xFE, 0x0F, 0x00, 0x00,
    0x54, 0x05, 0x52, 0x09, 0x49, 0x12, 0x48, 0x02, 0x40, 0x00,
};
static const IconData ICON_WATERING = {
    ICON_WATERING_DATA,
    13,
    13
};

// Icon: settings (13x13)
static const uint8_t ICON_SETTINGS_DATA[] PROGMEM = {
    0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x6C, 0x03, 0xFC, 0x03, 0x98, 0x01, 0x0E, 0x07, 0x0E, 0x07,
    0x98, 0x01, 0xFC, 0x03, 0x6C, 0x03, 0x60, 0x00, 0x00, 0x00,
};
static const IconData ICON_SETTINGS = {
    ICON_SETTINGS_DATA,
    13,
    13
};

// Icon: check (13x13)
static const uint8_t ICON_CHECK_DATA[] PROGMEM = {
    0x00, 0x00, 0x38, 0x00, 0x44, 0x00, 0x82, 0x00, 0x82, 0x00, 0x82, 0x00, 0xC4, 0x00, 0xF8, 0x01,
    0x80, 0x03, 0x00, 0x07, 0x00, 0x0E, 0x00, 0x0C, 0x00, 0x00,
};
static const IconData ICON_CHECK = {
    ICON_CHECK_DATA,
    13,
    13
};

// Icon: on_off (13x13)
static const uint8_t ICON_ON_OFF_DATA[] PROGMEM = {
    0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x58, 0x03, 0x44, 0x04, 0x44, 0x04, 0x02, 0x08, 0x02, 0x08,
    0x02, 0x08, 0x04, 0x04, 0x04, 0x04, 0x18, 0x03, 0xE0, 0x00,
};
static const IconData ICON_ON_OFF = {
    ICON_ON_OFF_DATA,
    13,
    13
};

// Icon: light_on (7x7)
static const uint8_t ICON_LIGHT_ON_DATA[] PROGMEM = {
    0x08, 0x2A, 0x1C, 0x7F, 0x1C, 0x2A, 0x08,
};
static const IconData ICON_LIGHT_ON = {
    ICON_LIGHT_ON_DATA,
    7,
    7
};

// Icon: light_off (7x7)
static const uint8_t ICON_LIGHT_OFF_DATA[] PROGMEM = {
    0x3C, 0x0E, 0x07, 0x07, 0x07, 0x0E, 0x3C,
};
static const IconData ICON_LIGHT_OFF = {
    ICON_LIGHT_OFF_DATA,
    7,
    7
};

// Icon: drop (7x7)
static const uint8_t ICON_DROP_DATA[] PROGMEM = {
    0x08, 0x08, 0x1C, 0x14, 0x22, 0x22, 0x1C,
};
static const IconData ICON_DROP = {
    ICON_DROP_DATA,
    7,
    7
};

// Icon: water (7x7)
static const uint8_t ICON_WATER_DATA[] PROGMEM = {
    0x06, 0x49, 0x36, 0x49, 0x36, 0x49, 0x30,
};
static const IconData ICON_WATER = {
    ICON_WATER_DATA,
    7,
    7
};

// Icon: air (7x7)
static const uint8_t ICON_AIR_DATA[] PROGMEM = {
    0x00, 0x7F, 0x00, 0x7F, 0x00, 0x7F, 0x00,
};
static const IconData ICON_AIR = {
    ICON_AIR_DATA,
    7,
    7
};

// Icon: earth (7x7)
static const uint8_t ICON_EARTH_DATA[] PROGMEM = {
    0x00, 0x44, 0x2A, 0x11, 0x44, 0x2A, 0x11,
};
static const IconData ICON_EARTH = {
    ICON_EARTH_DATA,
    7,
    7
};

// Icon: thermometer (7x7)
static const uint8_t ICON_THERMOMETER_DATA[] PROGMEM = {
    0x22, 0x57, 0x22, 0x02, 0x02, 0x0A, 0x04,
};
static const IconData ICON_THERMOMETER = {
    ICON_THERMOMETER_DATA,
    7,
    7
};

// Icon: pump_on (7x7)
static const uint8_t ICON_PUMP_ON_DATA[] PROGMEM = {
    0x08, 0x3E, 0x00, 0x2A, 0x2A, 0x49, 0x49,
};
static const IconData ICON_PUMP_ON = {
    ICON_PUMP_ON_DATA,
    7,
    7
};

// Icon: magnifier (7x7)
static const uint8_t ICON_MAGNIFIER_DATA[] PROGMEM = {
    0x0E, 0x1B, 0x11, 0x1B, 0x3E, 0x70, 0x60,
};
static const IconData ICON_MAGNIFIER = {
    ICON_MAGNIFIER_DATA,
    7,
    7
};

/**
 * Рисует иконку на дисплее по enum.
 *
 * @param u8g2 ссылка на объект U8g2
 * @param icon enum иконки
 * @param x координата X
 * @param y координата Y
 */
inline void drawIconByEnum(U8G2& u8g2, Icon icon, int16_t x, int16_t y) {
    const IconData* iconData = nullptr;
    
    switch (icon) {
        case Icon::Watering: iconData = &ICON_WATERING; break;
        case Icon::Settings: iconData = &ICON_SETTINGS; break;
        case Icon::Check: iconData = &ICON_CHECK; break;
        case Icon::OnOff: iconData = &ICON_ON_OFF; break;
        case Icon::LightOn: iconData = &ICON_LIGHT_ON; break;
        case Icon::LightOff: iconData = &ICON_LIGHT_OFF; break;
        case Icon::Drop: iconData = &ICON_DROP; break;
        case Icon::Water: iconData = &ICON_WATER; break;
        case Icon::Air: iconData = &ICON_AIR; break;
        case Icon::Earth: iconData = &ICON_EARTH; break;
        case Icon::Thermometer: iconData = &ICON_THERMOMETER; break;
        case Icon::PumpOn: iconData = &ICON_PUMP_ON; break;
        case Icon::Magnifier: iconData = &ICON_MAGNIFIER; break;
        default: return;
    }
    
    // U8g2 drawXBMP автоматически работает с PROGMEM данными
    // Формат XBM: LSB-first (как у нас)
    u8g2.drawXBMP(x, y, iconData->width, iconData->height, iconData->data);
}

}  // namespace Display::Icons
