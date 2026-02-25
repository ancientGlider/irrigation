# Настройка среды разработки для Arduino

## Настройка Cursor/VS Code для работы с Arduino C++

### 1. Автоматическое определение путей

Конфигурация в `.vscode/c_cpp_properties.json` уже содержит стандартные пути к библиотекам Arduino. Если IntelliSense не работает, выполните следующие шаги:

### 2. Поиск путей к библиотекам Arduino

Пути к библиотекам Arduino зависят от способа установки:

#### Arduino IDE (стандартная установка):
- Windows: `C:\Program Files (x86)\Arduino\hardware\arduino\avr\**`
- Windows (портативная): `[путь к Arduino]\hardware\arduino\avr\**`
- Linux: `~/.arduino15/packages/arduino/hardware/avr/**`
- macOS: `~/Library/Arduino15/packages/arduino/hardware/avr/**`

#### Arduino CLI / Arduino IDE 2.x:
- Windows: `C:\Users\[USERNAME]\AppData\Local\Arduino15\packages\arduino\hardware\avr\**`
- Linux: `~/.arduino15/packages/arduino/hardware/avr/**`
- macOS: `~/Library/Arduino15/packages/arduino/hardware/avr/**`

### 3. Обновление конфигурации

Откройте `.vscode/c_cpp_properties.json` и обновите массив `includePath` с правильными путями для вашей системы.

Пример для Windows с Arduino IDE:
```json
"includePath": [
    "${workspaceFolder}/**",
    "${workspaceFolder}/src/**",
    "${workspaceFolder}/lib/**",
    "C:/Program Files (x86)/Arduino/hardware/arduino/avr/cores/arduino",
    "C:/Program Files (x86)/Arduino/hardware/arduino/avr/variants/eightanaloginputs",
    "C:/Users/[USERNAME]/Documents/Arduino/libraries/**"
]
```

### 4. Пути к внешним библиотекам

Добавьте пути к установленным библиотекам (U8g2lib, Ds1302 и др.):

```json
"includePath": [
    ...
    "C:/Users/[USERNAME]/Documents/Arduino/libraries/U8g2/src/**",
    "C:/Users/[USERNAME]/Documents/Arduino/libraries/Ds1302/**"
]
```

### 5. Проверка работы IntelliSense

1. Откройте любой `.ino` или `.cpp` файл
2. Добавьте `#include <Arduino.h>`
3. Начните вводить код, например `digitalWrite`
4. Должно появиться автодополнение

### 6. Альтернатива: использование PlatformIO

Если возникают проблемы с настройкой путей, рассмотрите использование PlatformIO:

1. Установите расширение PlatformIO для VS Code/Cursor
2. Создайте файл `platformio.ini` в корне проекта
3. PlatformIO автоматически настроит все пути

Пример `platformio.ini`:
```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328
framework = arduino
lib_deps = 
    olikraus/U8g2@^2.35.19
    # Добавьте библиотеку Ds1302 через PlatformIO или локально
```

## Решение проблем

### IntelliSense не работает
1. Проверьте пути в `c_cpp_properties.json`
2. Перезапустите Cursor/VS Code
3. Выполните команду: `C/C++: Reset IntelliSense Database`

### Ошибки компиляции в IDE
- Убедитесь, что библиотеки установлены через Arduino Library Manager
- Проверьте версии библиотек на совместимость

### Проблемы с кодировкой
- Убедитесь, что файлы сохранены в UTF-8
- Для кириллицы используйте `u8g2.enableUTF8Print()` в коде

