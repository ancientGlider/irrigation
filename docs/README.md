# Система автоматического полива растений

## Быстрый старт

### Требования
- Arduino IDE 1.8.x или PlatformIO
- Библиотеки:
  - U8g2lib (для дисплея SSD1306)
  - Ds1302 (для часов реального времени DS1302)

### Структура проекта

```
src/
├── irrigation.ino          # Главный файл (точка входа)
├── modules/                # Модули системы
│   ├── soil_sensor/       # Датчик влажности почвы
│   ├── air_sensor/        # Датчик температуры и влажности воздуха
│   ├── button/            # Обработка кнопок
│   ├── timer/             # Системный таймер
│   └── timerRTC/          # Таймер на базе RTC
├── display/               # Управление дисплеем
├── irrigation/            # Логика полива
├── settings/              # Управление настройками и EEPROM
└── menu/                  # Меню настроек
```

### Настройка Arduino IDE

1. Установите библиотеки через Library Manager:
   - U8g2 (by olikraus)
   - Ds1302 (by Andrew Wickert)

2. Откройте файл `src/irrigation.ino` в Arduino IDE

3. Выберите плату: Tools → Board → Arduino Nano

4. Выберите процессор: Tools → Processor → ATmega328P (Old Bootloader)

5. Выберите порт: Tools → Port → [ваш COM порт]

### Настройка Cursor/VS Code

Конфигурация уже настроена в `.vscode/` и `.clangd`.

Для работы IntelliSense может потребоваться указать путь к библиотекам Arduino в `.vscode/c_cpp_properties.json`.

### Компиляция и загрузка

Используйте Arduino IDE для компиляции и загрузки прошивки на Arduino Nano.

## Документация

Подробное описание проекта см. в [PROJECT.md](../PROJECT.md)

