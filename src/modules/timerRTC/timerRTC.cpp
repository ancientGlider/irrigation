#include "timerRTC.h"

// Инициализация статических переменных
Ds1302::DateTime TimerRTC::_dateTime = {0, 1, 1, 0, 0, 0, 1};
unsigned long TimerRTC::_seconds = 0;
Ds1302* TimerRTC::_RTC = nullptr;
Timer TimerRTC::_timer(DEFAULT_RTC_POLLING_PERIOD);
const uint16_t TimerRTC::_daysBefore[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

TimerRTC::TimerRTC(unsigned long period) : _initialized(false), _initTime(0), _period(0) {
    // Используем setPeriod() для соблюдения принципа DRY
    setPeriod(period);
    if (_RTC) {
        // Если RTC уже инициализирован глобально, сразу синхронизируемся.
        _initialize();
    }
}

void TimerRTC::begin(uint8_t pin_ena, uint8_t pin_clk, uint8_t pin_dat) {
    if (!_RTC) {
        // Создаём экземпляр RTC только один раз, чтобы разделять аппаратные ресурсы
        // даже между несколькими объектами TimerRTC.
        _RTC = new Ds1302(pin_ena, pin_clk, pin_dat);
    }
    RTC().init();
    if (RTC().isHalted()) {
        // Если часы были остановлены (например, из-за выключения питания), запускаем их.
        RTC().start();
    }
}

void TimerRTC::end() {
    if (_RTC) {
        delete _RTC;
        _RTC = nullptr;
    }
}

unsigned long TimerRTC::getTime() {
    if (!_initialized) {
        // Первое обращение — синхронизируемся с RTC и устанавливаем начальное время.
        _initialize();
    } else if (_timer.isReady()) {
        // Период синхронизации истёк — обновляем данные из аппаратных часов.
        _updateTime();
    }
    
    // Используем модульную арифметику для корректной обработки переполнения
    // Формула: (текущее время в секундах - время инициализации) % MAX_SECONDS
    return (_timer.getTime() / 1000UL + _seconds - _initTime) % MAX_SECONDS;
}

void TimerRTC::setTime(unsigned long seconds, bool dropTimer) {
    // Смещаем initTime так, чтобы getTime() возвращал новое значение seconds.
    _initTime = (_seconds - seconds % MAX_SECONDS) % MAX_SECONDS;
    if (dropTimer) {
        drop();
    }
}

unsigned long TimerRTC::getInitTime() const {
    return _initTime;
}

void TimerRTC::setInitTime(unsigned long initTime) {
    _initTime = initTime % MAX_SECONDS;
}

bool TimerRTC::isReady() {
    if (getTime() >= _period) {
        // Таймер сработал — сбрасываем время отсчёта и уведомляем вызывающий код.
        drop();
        return true;
    }
    return false;
}

void TimerRTC::setPeriod(unsigned long period, bool dropTimer) {
    _period = period % MAX_SECONDS;
    if (dropTimer) {
        // После смены периода можно сразу начать отсчёт заново (по умолчанию true).
        drop();
    }
}

void TimerRTC::drop() {
    if (_initialized) {
        // Привязываем начало отсчёта к текущему времени RTC.
        _initTime = (_timer.getTime() / 1000UL + _seconds) % MAX_SECONDS;
    } else {
        _initTime = 0;
    }
}

void TimerRTC::_updateTime() {
    RTC().getDateTime(&_dateTime);
    
    // Преобразуем дату/время RTC в количество секунд с начала эпохи (01.01.00)
    _seconds = (unsigned long)((uint16_t)_dateTime.year * 365UL +
        _leapsBefore(_dateTime.year) +
        _daysBefore[_dateTime.month - 1] +
        (_dateTime.month > 2 && _isLeap(_dateTime.year) ? 1UL : 0UL) +
        _dateTime.day - 1) * 86400UL +
        _dateTime.hour * 3600UL +
        _dateTime.minute * 60UL +
        _dateTime.second;
}

void TimerRTC::_initialize() {
    _initialized = true;
    _updateTime();
    // После синхронизации привязываем initTime к текущему времени.
    drop();
}

