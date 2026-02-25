/*
 * Menu strings (PROGMEM)
 *
 * Все строки меню хранятся во Flash для экономии RAM.
 * Кириллица в кодировке UTF-8.
 */

#pragma once

#include <Arduino.h>

namespace Menu {

// === Названия разделов главного меню ===
// Строка "ОЧИСТКА" используется дважды: для Action и Submenu
const char STR_MENU_TRAIN[]  PROGMEM = "ТРЕНИРОВКА";
const char STR_MENU_CLEAN[]  PROGMEM = "ОЧИСТКА";
const char STR_MENU_CYCLE[]  PROGMEM = "ЦИКЛ";
const char STR_MENU_LIGHT[]  PROGMEM = "СВЕТ";
const char STR_MENU_WATER[]  PROGMEM = "ПОЛИВ";
const char STR_MENU_SENSOR[] PROGMEM = "ДАТЧИК";
const char STR_MENU_RESET[]  PROGMEM = "СБРОС";

// === Названия параметров ===

// Подменю ЦИКЛ
const char STR_PARAM_MODE[]   PROGMEM = "Режим";
const char STR_PARAM_DAY[]    PROGMEM = "День";
const char STR_PARAM_HOUR[]   PROGMEM = "Час";
const char STR_PARAM_GERM[]   PROGMEM = "Росток";
const char STR_PARAM_SPRING[] PROGMEM = "Весна";
const char STR_PARAM_SUMMER[] PROGMEM = "Лето";
const char STR_PARAM_AUTUMN[] PROGMEM = "Осень";

// Подменю ПОЛИВ
const char STR_PARAM_DURATION[]  PROGMEM = "Время";
const char STR_PARAM_PAUSE[]     PROGMEM = "Пауза";
const char STR_PARAM_MIN_PCT[]   PROGMEM = "Мин.%";
const char STR_PARAM_MAX_PCT[]   PROGMEM = "Макс.%";
const char STR_PARAM_ATTEMPTS[]  PROGMEM = "Попыток";
const char STR_PARAM_TRAIN_PCT[] PROGMEM = "Тренир.%";

// Подменю ОЧИСТКА
const char STR_PARAM_CLEAN_DUR[]    PROGMEM = "Время";
const char STR_PARAM_CLEAN_CYCLES[] PROGMEM = "Циклов";
const char STR_PARAM_CLEAN_PAUSE[]  PROGMEM = "Пауза";

// Подменю ДАТЧИК
const char STR_PARAM_CHECK_PERIOD[] PROGMEM = "Период";
const char STR_PARAM_CALIB_DRY[]    PROGMEM = "Сухо";
const char STR_PARAM_CALIB_WET[]    PROGMEM = "Влажно";

// === Названия режимов (для SystemMode) ===
const char STR_MODE_GROW[]   PROGMEM = "ЦИКЛ";
const char STR_MODE_SPRING[] PROGMEM = "ВЕСНА";
const char STR_MODE_SUMMER[] PROGMEM = "ЛЕТО";
const char STR_MODE_AUTUMN[] PROGMEM = "ОСЕНЬ";

// Массив указателей на строки режимов (для DisplayMode::AsEnum)
const char* const MODE_NAMES[] PROGMEM = {
    STR_MODE_GROW,
    STR_MODE_SPRING,
    STR_MODE_SUMMER,
    STR_MODE_AUTUMN
};
constexpr uint8_t MODE_NAMES_COUNT = 4;

// === Служебные строки ===
const char STR_TITLE_EDIT[]    PROGMEM = "РЕДАКТИРОВАНИЕ";
const char STR_TITLE_CONFIRM[] PROGMEM = "ПОДТВЕРЖДЕНИЕ";
const char STR_CURRENT[]       PROGMEM = "Текущее:";
const char STR_MIN[]           PROGMEM = "мин:";
const char STR_MAX[]           PROGMEM = "макс:";
const char STR_YES[]           PROGMEM = "ДА";
const char STR_NO[]            PROGMEM = "НЕТ";
const char STR_RESET_CONFIRM[] PROGMEM = "Сбросить?";

// === Спецсимволы (добавляются программно) ===
const char SYMBOL_ACTION  = '>';  // Стрелка вправо
const char SYMBOL_SUBMENU = '*';  // Звёздочка

} // namespace Menu
