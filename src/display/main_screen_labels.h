#pragma once

#include <Arduino.h>

namespace Display::Labels {

struct Caption {
    const char* line1;
    const char* line2;
};

// Строки для WATERING_CAPTIONS в PROGMEM
const char WATERING_STOP[] PROGMEM = "СТОП";
const char WATERING_CONTROL[] PROGMEM = "КОНТРОЛЬ";
const char WATERING_AUTO[] PROGMEM = "АВТО";
const char WATERING_POLIV[] PROGMEM = "ПОЛИВ";
const char WATERING_OZHIDANIE[] PROGMEM = "ОЖИДАНИЕ";
const char WATERING_TRENIROVKI[] PROGMEM = "ТРЕНИРОВКИ";
const char WATERING_TRENIROVKA[] PROGMEM = "ТРЕНИРОВКА";
const char WATERING_NET[] PROGMEM = "НЕТ";
const char WATERING_VODY[] PROGMEM = "ВОДЫ";

const Caption WATERING_CAPTIONS[] PROGMEM = {
    /* Stop         */ {WATERING_STOP, nullptr},
    /* Control      */ {WATERING_CONTROL, nullptr},
    /* AutoWatering */ {WATERING_AUTO, WATERING_POLIV},
    /* TrainingWait */ {WATERING_OZHIDANIE, WATERING_TRENIROVKI},
    /* Training     */ {WATERING_TRENIROVKA, nullptr},
    /* NoWater      */ {WATERING_NET, WATERING_VODY}
};

// Единый массив всех уникальных подписей для экономии памяти
// Используем массив указателей на строки переменной длины в PROGMEM
// Каждая строка явно оканчивается нулевым байтом
const char LABEL_0[] PROGMEM = "РОСТ";
const char LABEL_1[] PROGMEM = "ВЕСНА";
const char LABEL_2[] PROGMEM = "ЛЕТО";
const char LABEL_3[] PROGMEM = "ОСЕНЬ";
const char LABEL_4[] PROGMEM = "РОСТОК";
const char LABEL_5[] PROGMEM = "КОНЕЦ";

const char* const LABELS[] PROGMEM = {
    LABEL_0,  // 0 - для SystemMode::Growing
    LABEL_1,  // 1 - для SystemMode::Spring и SeasonLabel::Spring
    LABEL_2,  // 2 - для SystemMode::Summer и SeasonLabel::Summer
    LABEL_3,  // 3 - для SystemMode::Autumn и SeasonLabel::Autumn
    LABEL_4,  // 4 - для SeasonLabel::Germination
    LABEL_5   // 5 - для SeasonLabel::Completed
};

// Индексы в LABELS для режимов системы (SystemMode)
constexpr uint8_t MODE_LABEL_INDICES[] = {
    0,  // SystemMode::Growing  -> "РОСТ"
    1,  // SystemMode::Spring   -> "ВЕСНА"
    2,  // SystemMode::Summer   -> "ЛЕТО"
    3   // SystemMode::Autumn    -> "ОСЕНЬ"
};

// Индексы в LABELS для сезонов (SeasonLabel)
constexpr uint8_t SEASON_LABEL_INDICES[] = {
    4,  // SeasonLabel::Germination -> "РОСТОК"
    1,  // SeasonLabel::Spring      -> "ВЕСНА"
    2,  // SeasonLabel::Summer      -> "ЛЕТО"
    3,  // SeasonLabel::Autumn      -> "ОСЕНЬ"
    5   // SeasonLabel::Completed   -> "КОНЕЦ"
};

// Функции-геттеры для удобного доступа к подписям
// Возвращают указатель на строку в PROGMEM
// Используем pgm_read_ptr для чтения указателя из PROGMEM
inline const char* getModeLabel(uint8_t mode) {
    if (mode >= sizeof(MODE_LABEL_INDICES) / sizeof(MODE_LABEL_INDICES[0])) {
        return nullptr;
    }
    uint8_t index = MODE_LABEL_INDICES[mode];
    return (const char*)pgm_read_ptr(&LABELS[index]);
}

inline const char* getSeasonLabel(uint8_t season) {
    if (season >= sizeof(SEASON_LABEL_INDICES) / sizeof(SEASON_LABEL_INDICES[0])) {
        return nullptr;
    }
    uint8_t index = SEASON_LABEL_INDICES[season];
    return (const char*)pgm_read_ptr(&LABELS[index]);
}

}  // namespace Display::Labels


