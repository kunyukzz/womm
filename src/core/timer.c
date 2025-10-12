#include "timer.h"
#include "platform/window.h"

void timer_start(timer_t *timer) {
    timer->start_time = get_abs_time();
    timer->elapsed = 0;
}

void timer_update(timer_t *timer) {
    if (timer->start_time != 0) {
        timer->elapsed = get_abs_time() - timer->start_time;
    }
}

void timer_stop(timer_t *timer) { timer->start_time = 0; }
