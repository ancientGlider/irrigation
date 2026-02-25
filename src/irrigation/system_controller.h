/*
 * System Controller
 *
 * Главный контроллер системы, который:
 * - Управляет вызовами всех FSM контроллеров
 * - Агрегирует состояние всех модулей в SystemState
 * - Предоставляет единую точку доступа к состоянию системы
 */

#pragma once

#include <Arduino.h>
#include "system_state.h"
#include "watering/watering.h"
#include "growing_cycle.h"
#include "light_control.h"
#include "../modules/timerRTC/timerRTC.h"
#include "../modules/soil_sensor/soil_sensor.h"
#include "../modules/air_sensor/air_sensor.h"

namespace Irrigation {

class SystemController {
public:
    SystemController() = delete;

    /**
     * Инициализирует все модули системы.
     * 
     * @param pinPump Пин управления помпой
     * @param pumpHighIsOn true, если HIGH включает помпу
     * @param pinLight Пин управления освещением
     * @param lightHighIsOn true, если HIGH включает свет
     * @param pinSoilSensor Аналоговый пин датчика влажности почвы
     * @param pinSoilVCC Пин управления питанием датчика почвы
     * @param pinAirSensor Цифровой пин датчика температуры/влажности воздуха
     * @param rtc Ссылка на инициализированный TimerRTC
     */
    static void begin(
        uint8_t pinPump, bool pumpHighIsOn,
        uint8_t pinLight, bool lightHighIsOn,
        uint8_t pinSoilSensor, uint8_t pinSoilVCC,
        uint8_t pinAirSensor,
        TimerRTC& rtc
    );

    /**
     * Основной цикл обработки. Вызывается в loop().
     * 
     * SystemController является главным процессором системы.
     * Осуществляет циклический опрос всех модулей, получает данные о состоянии
     * системы от датчиков и конечных автоматов, на основании которых принимает
     * решения по управлению системой.
     * 
     * Выполняет:
     * 1. Обновление датчиков (FSM внутри модулей)
     * 2. Обработку полива (FSM)
     * 3. Обновление освещения
     * 4. Агрегацию состояния системы в _state
     */
    static void process();

    /**
     * Возвращает указатель на структуру данных состояния системы.
     * 
     * Используется для передачи указателя модулю Display при инициализации.
     * Display использует заголовочный файл system_state.h для понимания структуры.
     * 
     * @return const указатель на SystemState (не копируется)
     */
    static const SystemState* getSystemState();

    // Команды управления (делегируются модулям)

    /**
     * Запускает ручной полив.
     */
    static void setManualWatering();

    /**
     * Запускает режим очистки.
     */
    static void setManualCleaning();

    /**
     * Запускает режим тренировки.
     */
    static void setTraining();

    /**
     * Останавливает полив.
     */
    static void stopWatering();

    /**
     * Принудительно запускает проверку датчиков.
     */
    static void forceSensorCheck();

    /**
     * Возвращает сырое значение датчика почвы (0-1023).
     * Используется для отображения live-значения в меню калибровки.
     * @param forceCheck Если true, принудительно запускает чтение датчика
     * @return Сырое значение АЦП (0-1023) или 0 если датчик не инициализирован
     */
    static int16_t getSoilRawValue(bool forceCheck = false);

private:
    /**
     * Агрегирует состояние из всех модулей в _state.
     */
    static void _aggregateState();

    /**
     * Обновляет данные датчиков (вызывает FSM модулей).
     */
    static void _updateSensors();

    static SystemState _state;
    static bool _initialized;
    
    // Указатели на объекты датчиков (создаются в begin())
    static SoilSensor* _soilSensor;
    static AirSensor* _airSensor;
};

} // namespace Irrigation

