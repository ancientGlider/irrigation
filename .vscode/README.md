# Конфигурация Cursor/VS Code для Arduino (macOS)

## Структура путей

| Компонент | Путь |
|-----------|------|
| arduino-cli | `/opt/homebrew/bin/arduino-cli` |
| Библиотеки | `~/Code/arduino/libraries/` |
| AVR Core | `~/Library/Arduino15/packages/arduino/hardware/avr/1.8.7/` |
| Компилятор | `~/Library/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7/bin/` |
| Конфиг CLI | `~/Library/Arduino15/arduino-cli.yaml` |

## Настроенные файлы

- `c_cpp_properties.json` — пути для IntelliSense
- `settings.json` — настройки редактора
- `tasks.json` — задачи компиляции и загрузки

## Использование

### Компиляция
- **Cmd+Shift+B** — компиляция проекта
- Результат в папке `build/`

### Загрузка на плату
1. Подключите Arduino Nano
2. Запустите задачу "List Boards" чтобы найти порт
3. Запустите задачу "Upload"

### Serial Monitor
```bash
arduino-cli monitor -p /dev/cu.usbserial-XXXX -c baudrate=9600
```

## Установка новых библиотек

```bash
# Через arduino-cli (рекомендуется)
arduino-cli lib install "LibraryName"

# Или вручную скопировать в ~/Code/arduino/libraries/
```

## Решение проблем

### IntelliSense не работает
1. Перезапустите Cursor
2. Выполните: `C/C++: Reset IntelliSense Database`

### Ошибка компиляции "library not found"
Проверьте наличие библиотеки:
```bash
ls ~/Code/arduino/libraries/
```
