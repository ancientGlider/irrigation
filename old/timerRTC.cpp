#include "TimerRTC.h"

//Ds1302::DateTime TimerRTC::_dateTime = {0, 1, 1, 0, 0, 0, 1};
//unsigned long TimerRTC::_seconds = 0;
//Ds1302* TimerRTC::_RTC = nullptr;
//Timer TimerRTC::_timer = Timer(DEFAULT_RTC_POLLING_PERIOD);
//const uint16_t TimerRTC::_daysBefore[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

/* основной конструктор, если указан период срабатывания, устанавливает его */
TimerRTC::TimerRTC(unsigned long period = 0) {
    setPeriod(period);
    if (_RTC) _initialize();
}

void TimerRTC::begin(uint8_t pin_ena, uint8_t pin_clk, uint8_t pin_dat) {
    if (!_RTC) _RTC = new Ds1302(pin_ena, pin_clk, pin_dat);
    RTC().init();
    if (RTC().isHalted()) RTC().start();
}

void TimerRTC::end() {
    if (_RTC) {
        delete _RTC;
        _RTC = nullptr;
    }
}

/* возвращает количество секунд, прошедших с момента предыдущего сброса таймера */
unsigned long TimerRTC::getTime() {
    if (!_initialized) _initialize();
    else if (_timer.isReady()) _updateTime();
    return (_timer.getTime() / 1000UL + _seconds - _initTime) % 3155760000UL;  // текущее время системы минус сохранённое
}

void TimerRTC::setTime(unsigned long seconds, bool dropTimer = false) {
    _initTime = (_seconds - seconds % 3155760000UL) % 3155760000UL;
    if (dropTimer) drop();
}

unsigned long TimerRTC::getInitTime() {
    return _initTime;
}

void TimerRTC::setInitTime(unsigned long initTime) {
    _initTime = initTime % 3155760000UL;
}

/* при прохождении временного периода возвращает true и сбрасывает таймер */
bool TimerRTC::isReady() {
    if (getTime() >= _period) {    // если время с момента запуска таймера больше или равно периоду,
        drop();                     // то сбросить таймер
        return true;                // и вернуть true
    }
    return false;                   // иначе вернуть false
}

/* устанавливает period, при этом если с момента предыдущего запуска таймера уже прошёл period,
то is_ready() сразу вернёт true
чтобы этого избежать необходимо дополнительно вызвать drop() */
void TimerRTC::setPeriod(unsigned long period, bool dropTimer = false) {
    _period = period % 3155760000UL;
    if (dropTimer) drop();    
}

/* возвращает текущий period */
unsigned long TimerRTC::getPeriod() {
    return _period;
}

/* сбрасывает таймер (осуществляет его перезапуск) */
void TimerRTC::drop() {
    _initTime = _seconds % 3155760000UL;
}
/*
Ds1302::DateTime TimerRTC::getDateTime() {
    return _dateTime;
}
*/
Ds1302& TimerRTC::RTC()  {
    return *_RTC;
}

void TimerRTC::_updateTime() {
    RTC().getDateTime(&_dateTime);

    _seconds = (unsigned long)((uint16_t)_dateTime.year * 365 +
        _leapsBefore(_dateTime.year) +
        _daysBefore[_dateTime.month - 1] +
        (_dateTime.month > 2 && _isLeap(_dateTime.year) ? 1 : 0) +
        _dateTime.day - 1) * 86400UL +
        _dateTime.hour * 3600UL +
        _dateTime.minute * 60UL +
        _dateTime.second;
}
/*/
void TimerRTC::_convertTimeFromSeconds() {
    unsigned long totalDays, remainingSeconds;
    totalDays = _seconds / 86400UL;
    remainingSeconds = _seconds % 86400UL;
    _dateTime.second = (uint8_t)(remainingSeconds % 60);
    remainingSeconds /= 60;
    _dateTime.minute = (uint8_t)(remainingSeconds % 60);
    _dateTime.hour = (uint8_t)(remainingSeconds / 60);
    _dateTime.year = 0;
    while (totalDays >= (_isLeap(_dateTime.year) ? 366UL : 365UL)) {
        totalDays -= (_isLeap(_dateTime.year) ? 366 : 365);
        _dateTime.year++;
        
    }
    for (_dateTime.month = 1; _dateTime.month < 12; _dateTime.month++) {
        if (totalDays < (_dateTime.month == 1 ? 31 : (_isLeap(_dateTime.year) ? _daysBefore[_dateTime.month] + 1 : _daysBefore[_dateTime.month]))) break; 
    }
    _dateTime.day = (_dateTime.month == 1 ? totalDays : (totalDays - (_isLeap(_dateTime.year) ? _daysBefore[_dateTime.month] + 1 : _daysBefore[_dateTime.month])));
}
*/
inline bool TimerRTC::_isLeap(uint8_t year) {
    return year % 4 == 0;
}

inline uint16_t TimerRTC::_leapsBefore(uint8_t year) {
    return (year + 3) / 4;
}

void TimerRTC::_initialize() {
    _initialized = true;
    _updateTime();
    drop();
}
