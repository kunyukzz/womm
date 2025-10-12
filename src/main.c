#include "core/define.h"
#include "core/arena.h"
#include "core/event.h"
#include "core/input.h"
#include "core/memory.h"

#include "platform/window.h"

#include "renderer/frontend.h"

#include <stdio.h>
#include <string.h>

// TODO: Temporary code!!
typedef struct {
    double start_time;
    double elapsed;
} time_clock_t;

void clock_start(time_clock_t *clock) {
    clock->start_time = get_abs_time();
    clock->elapsed = 0;
}

void clock_update(time_clock_t *clock) {
    if (clock->start_time != 0) {
        clock->elapsed = get_abs_time() - clock->start_time;
    }
}

void clock_stop(time_clock_t *clock) { clock->start_time = 0; }

typedef struct {
    arena_alloc_t *arena;
    time_clock_t time_clock;

    bool is_running;
    bool is_suspend;
    double last_time;
    float delta;
} game_system_t;

game_system_t *game_init(arena_alloc_t *arena) {
    game_system_t *game = arena_alloc(arena, sizeof(game_system_t));
    memset(game, 0, sizeof(game_system_t));
    game->arena = arena;

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

void game_shutdown(game_system_t *game) {
    if (game) memset(game, 0, sizeof(game_system_t));
    LOG_INFO("Goodbye WOMM!!");
}

// for all module system
typedef struct {
    window_system_t *window;
    event_system_t *event;
    input_system_t *input;
    render_system_t *render;
    game_system_t *game;
} system_t;

static system_t g_system;

// Event handlers
bool game_on_input(event_system_t *event, uint32_t type, event_t *ev,
                   void *sender, void *recipient) {
    (void)sender;
    (void)recipient;
    if (type == EVENT_KEY_PRESS) {
        uint32_t kc = ev->data.keys.keycode;
        if (kc == INPUT_KEY_ESCAPE) {
            event_t evquit;
            event_push(event, EVENT_QUIT, &evquit, NULL);
            return true;
        } else {
            LOG_DEBUG("'%s' pressed in window", keycode_to_str(kc));
        }
    } else if (type == EVENT_KEY_RELEASE) {
        uint32_t kc = ev->data.keys.keycode;
        LOG_DEBUG("'%s' released", keycode_to_str(kc));
    }
    return false;
}

bool game_on_event(event_system_t *event, uint32_t type, event_t *ev,
                   void *sender, void *recipient) {
    (void)event;
    (void)ev;
    (void)sender;
    (void)recipient;
    switch (type) {
        case EVENT_QUIT: {
            LOG_INFO("EVENT_QUIT received. Shutdown...");
            g_system.game->is_running = false;
            return true;
        } break;
        case EVENT_SUSPEND: {
            LOG_INFO("EVENT_SUSPEND received. Suspend...");
            g_system.game->is_suspend = true;
            return true;
        } break;
        case EVENT_RESUME: {
            LOG_INFO("EVENT_RESUME received. Resume...");
            g_system.game->is_suspend = false;
            g_system.game->is_running = true;
            return true;
        } break;
    }
    return false;
}

bool game_on_resize(event_system_t *event, uint32_t type, event_t *ev,
                    void *sender, void *recipient) {
    (void)event;
    (void)sender;
    (void)recipient;
    if (type == EVENT_RESIZE) {
        uint32_t w = ev->data.resize.width;
        uint32_t h = ev->data.resize.height;

        if (w != g_system.window->native_win.width ||
            h != g_system.window->native_win.height) {
            g_system.window->native_win.width = w;
            g_system.window->native_win.height = h;

            if (w == 0 || h == 0) {
                g_system.game->is_suspend = true;
                return true;
            } else {
                if (g_system.game->is_suspend) {
                    g_system.game->is_suspend = false;
                }
                // TODO: renderer_system_resize(w, h);
            }
        }
    }
    return false;
}

int main(void) {
    uint64_t estimated_memory = 10 * 1024 * 1024;
    if (!memory_system_init(estimated_memory)) {
        LOG_ERROR("Failed to init memory system with estimated size: %lu",
                  estimated_memory);
        return -1;
    }

    g_system.game = WALLOC(sizeof(game_system_t), MEM_GAME);

    arena_alloc_t persistent_arena, frame_arena;
    arena_create(128 * 1024, &persistent_arena);
    arena_create(64 * 1024, &frame_arena);

    window_config_t config = {.name = "WOMM",
                              .width = 800,
                              .height = 600,
                              .is_resizeable = true};

    g_system.window = window_system_init(config, &persistent_arena);
    g_system.event = event_system_init(&persistent_arena);
    g_system.input = input_system_init(&persistent_arena);
    g_system.render =
        render_system_init(&persistent_arena, &g_system.window->native_win);
    // g_system.game = game_init(&persistent_arena);

    event_reg(g_system.event, EVENT_QUIT, game_on_event, NULL);
    event_reg(g_system.event, EVENT_SUSPEND, game_on_event, NULL);
    event_reg(g_system.event, EVENT_RESUME, game_on_event, NULL);
    event_reg(g_system.event, EVENT_RESIZE, game_on_resize, NULL);
    event_reg(g_system.event, EVENT_KEY_PRESS, game_on_input, NULL);
    event_reg(g_system.event, EVENT_KEY_RELEASE, game_on_input, NULL);

    const double TARGET_FPS = 60.0;
    const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

    g_system.game->is_running = true;
    clock_start(&g_system.game->time_clock);
    clock_update(&g_system.game->time_clock);
    g_system.game->last_time = g_system.game->time_clock.elapsed;

    double runtime = 0;
    uint8_t frame_count = 0;
    const bool limit = false;

    LOG_INFO("%s", mem_debug_stat());
    LOG_INFO("%s", vram_status(g_system.render));

    while (g_system.game->is_running) {
        if (!window_system_pump(g_system.window, g_system.input,
                                g_system.event)) {
            g_system.game->is_running = false;
        };

        if (!g_system.game->is_suspend) {
            clock_update(&g_system.game->time_clock);
            double curr_time = g_system.game->time_clock.elapsed;
            g_system.game->delta =
                (float)(curr_time - g_system.game->last_time);

            g_system.game->last_time = curr_time;
            double frame_time_start = get_abs_time();

            input_system_update(g_system.input, g_system.game->delta,
                                &frame_arena);

            if (!game_update(g_system.game, g_system.game->delta)) {
                g_system.game->is_running = false;
                break;
            }
            if (!game_render(g_system.game, g_system.game->delta)) {
                g_system.game->is_running = false;
                break;
            }

            // TODO: renderer system draw

            // printf("Tech Debt Game Runs!!\n");
            double next_frame_time = frame_time_start + TARGET_FRAME_TIME;
            double frame_time_end = get_abs_time();
            double frame_elapsed = frame_time_end - frame_time_start;
            runtime += frame_elapsed;

            if (limit && frame_time_end < next_frame_time) {
                get_sleep(next_frame_time);
            }

            // print FPS
            static double fps_timer = 0.0;
            frame_count++;
            fps_timer += g_system.game->delta;

            if (fps_timer >= 1.0) {
                printf("FPS: %d\n", frame_count);
                frame_count = 0;
                fps_timer = 0.0;
            }
            (void)runtime;
        }
    }
    g_system.game->is_running = false;

    event_unreg(g_system.event, EVENT_QUIT, game_on_event, NULL);
    event_unreg(g_system.event, EVENT_SUSPEND, game_on_event, NULL);
    event_unreg(g_system.event, EVENT_RESUME, game_on_event, NULL);
    event_unreg(g_system.event, EVENT_RESIZE, game_on_resize, NULL);
    event_unreg(g_system.event, EVENT_KEY_PRESS, game_on_input, NULL);
    event_unreg(g_system.event, EVENT_KEY_RELEASE, game_on_input, NULL);

    game_shutdown(g_system.game);
    render_system_kill(g_system.render);
    input_system_kill(g_system.input);
    event_system_kill(g_system.event);
    window_system_kill(g_system.window);

    arena_kill(&frame_arena);
    arena_kill(&persistent_arena);

    memory_system_kill();
    return 0;
}
