#include "game.h"
#include "core/memory.h"

game_system_t *game_init(void) {
    game_system_t *game = WALLOC(sizeof(game_system_t), MEM_GAME);

    LOG_INFO("Welcome to WOMM!!");
    return game;
}

bool game_update(game_system_t *game, float delta) {
    (void)game;
    (void)delta;
    return true;
}

bool game_render(game_system_t *game, float delta) {
    (void)game;
    (void)delta;
    return true;
}

void game_kill(game_system_t *game) {
    if (game) {
        WFREE(game, 0, sizeof(game_system_t));
    }
    LOG_INFO("Goodbye WOMM!!");
}

bool game_is_running(game_system_t *game) {
    game->is_running = true;
    timer_start(&game->timer);
    timer_update(&game->timer);
    game->last_time = game->timer.elapsed;

    return true;
}
