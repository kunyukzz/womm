#ifndef GAME_H
#define GAME_H

#include "core/define.h"
#include "core/timer.h"
#include "core/camera.h"

typedef struct {
    timer_t timer;
    camera_system_t *cam;

    bool is_running;
    bool is_suspend;
    double last_time;
    float delta;
} game_system_t;

game_system_t *game_init(void);
bool game_update(game_system_t *game, float delta);
bool game_render(game_system_t *game, float delta);
void game_kill(game_system_t *game);

bool game_is_running(game_system_t *game);

#endif // GAME_H
