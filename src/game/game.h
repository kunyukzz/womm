#ifndef GAME_H
#define GAME_H

#include "core/define.h" // IWYU pragma: keep
#include "core/camera.h"

typedef struct {
    camera_system_t *cam;

    int32_t mouse_x, mouse_y;
    float delta;
} game_system_t;

game_system_t *game_init(void);
bool game_update(game_system_t *game, float delta);
bool game_render(game_system_t *game, float delta);
void game_kill(game_system_t *game);

#endif // GAME_H
