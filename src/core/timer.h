#ifndef TIMER_H
#define TIMER_H

typedef struct {
    double start_time;
    double elapsed;
} timer_t;

void timer_start(timer_t *timer);
void timer_update(timer_t *timer);
void timer_stop(timer_t *timer);

#endif // TIMER_H
