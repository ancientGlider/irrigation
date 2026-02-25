# Конфигурация Cursor/VS Code для Arduino

## Что было настроено

### 1. Конфигурация IntelliSense (`.vscode/c_cpp_properties.json`)
- Настроены пути к библиотекам Arduino для Windows
- Определены макросы для Arduino Nano (ATmega328P)
- Настроен режим IntelliSense для AVR микроконтроллеров

### 2. Настройки редактора (`.vscode/settings.json`)
- Ассоциации файлов (`.ino` как C++)
- Исключения файлов из поиска (build, hex и т.д.)
- Кодировка UTF-8 для поддержки кириллицы

### 3. Конфигурация clangd (`.clangd`)
- Настройки компилятора для AVR
- Флаги компиляции для Arduino

## Что нужно сделать дополнительно

### 1. Добавить пути к библиотекам пользователя

Откройте `.vscode/c_cpp_properties.json` и добавьте пути к установленным библиотекам:

```json
"includePath": [
    ...
    "C:/Users/[ВАШЕ_ИМЯ]/Documents/Arduino/libraries/U8g2/src/**",
    "C:/Users/[ВАШЕ_ИМЯ]/Documents/Arduino/libraries/Ds1302/**"
]
```

### 2. Проверить работу IntelliSense

1. Откройте любой файл проекта
2. Добавьте `#include <Arduino.h>`
3. Начните вводить код - должно появиться автодополнение

### 3. Если IntelliSense не работает

1. Проверьте, что Arduino IDE установлен
2. Найдите путь к библиотекам Arduino (см. `docs/SETUP.md`)
3. Обновите пути в `c_cpp_properties.json`
4. Перезапустите Cursor
5. Выполните команду: `C/C++: Reset IntelliSense Database`

## Дополнительная информация

Подробные инструкции по настройке см. в [docs/SETUP.md](../docs/SETUP.md)

