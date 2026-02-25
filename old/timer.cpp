#include "timer.h"

/* основной конструктор,  сбрасывет таймер */
Timer::Timer(unsigned long period = 0, bool dropTimer = true) {
    setPeriod(period);
}

/* возвращает количество миллисекунд, прошедших с момента предыдущего сброса таймера */
unsigned long Timer::getTime() {
    return millis() - _initTime;  // текущее время системы минус сохранённое
}

/* при прохождении временного периода возвращает true и сбрасывает таймер */
bool Timer::isReady() {
    if (getTime() >= _period) {    // если время с момента запуска таймера больше или равно периоду,
        drop();                     // то сбросить таймер
        return true;                // и вернуть true
    }
    return false;                   // иначе вернуть false
}

/* устанавливает period, при этом если с момента предыдущего запуска таймера уже прошёл period,
то is_ready() сразу вернёт true
чтобы этого избежать необходимо указать dropTimer = true */
void Timer::setPeriod(unsigned long period, bool dropTimer = true) {
    _period = period;
    if (dropTimer) drop();
}

/* возвращает текущий period */
unsigned long Timer::getPeriod() {
    return _period;
}

/* сбрасывает таймер (осуществляет его перезапуск) */
void Timer::drop() {
    _initTime = millis();
}
