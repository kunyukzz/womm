#include "core/define.h"
#include "core/event.h"
#include "core/input.h"
#include "core/memory.h"
#include "platform/filesystem.h"
#include "renderer/frontend.h"
#include "module/geometry.h"
#include "game/game.h"
#include "core/math/maths.h"

#include <stdio.h>

// for all module system
typedef struct {
    arena_alloc_t persistent_arena;
    arena_alloc_t frame_arena;

    render_bundle_t bundle;

    file_system_t *fs;
    window_system_t *window;
    event_system_t *event;
    input_system_t *input;
    camera_system_t *camera;
    render_system_t *render;
    geometry_system_t *geo;
    game_system_t *game;
} system_t;

static system_t g_system;

bool game_on_input(event_system_t *event, uint32_t type, event_t *ev,
                   void *sender, void *recipient);
bool game_on_event(event_system_t *event, uint32_t type, event_t *ev,
                   void *sender, void *recipient);
bool game_on_resize(event_system_t *event, uint32_t type, event_t *ev,
                    void *sender, void *recipient);

#if DEBUG
static void system_log(void) {
    LOG_DEBUG("=== Memory Addresses ===");
    LOG_DEBUG("Window:    %p", g_system.window);
    LOG_DEBUG("Event:     %p", g_system.event);
    LOG_DEBUG("Input:     %p", g_system.input);
    LOG_DEBUG("Camera:    %p", g_system.camera);
    LOG_DEBUG("Render:    %p", g_system.render);
    LOG_DEBUG("Geometry:  %p", g_system.geo);

    uint64_t used = arena_used(&g_system.persistent_arena);
    uint64_t total = g_system.persistent_arena.total_size;
    float usage_percent = (float)used / (float)total * 100.0f;

    LOG_DEBUG("Arena Usage: %lu/%lu bytes (%.1f%%)", used, total,
              usage_percent);

    if (usage_percent < 50.0f) {
        LOG_WARN("Wasted space! Arena is only %.1f%% used", usage_percent);
    } else if (usage_percent > 90.0f) {
        LOG_WARN("Arena nearly full! %.1f%% used", usage_percent);
    }
}
#endif

static bool system_init(void) {
    // set memory system allocation 10Mb
    uint64_t estimated_memory = 10 * 1024 * 1024;
    if (!memory_system_init(estimated_memory)) {
        LOG_ERROR("Failed to init memory system with estimated size: %lu",
                  estimated_memory);
        return false;
    }

    arena_create(16 * 1024, &g_system.persistent_arena, NULL);
    arena_create(8 * 1024, &g_system.frame_arena, NULL);

    window_config_t config = {.name = "WOMM",
                              .width = 800,
                              .height = 600,
                              .is_resizeable = true};

    g_system.fs = filesys_init(&g_system.persistent_arena);
    g_system.window = window_system_init(config, &g_system.persistent_arena);
    g_system.event = event_system_init(&g_system.persistent_arena);
    g_system.input = input_system_init(&g_system.persistent_arena);

    g_system.camera = camera_system_init(&g_system.persistent_arena,
                                         &g_system.window->native_win);
    g_system.render = render_system_init(&g_system.persistent_arena,
                                         &g_system.window->native_win);

    g_system.geo = geo_system_init(&g_system.persistent_arena);
    g_system.game = game_init();

    // bundle initialize
    g_system.bundle.delta = g_system.game->delta;

    g_system.bundle.obj[0].geo = &g_system.geo->default_geo;
    g_system.bundle.obj[0].model =
        mat4_translate((vec3){{-5.0f, 0.0f, 0.0f, 0}});
    g_system.bundle.obj[0].diffuse_color = (vec4){{1.0f, 0.0f, 0.0f, 1.0f}};

    g_system.bundle.obj[1].geo = &g_system.geo->default_geo;
    g_system.bundle.obj[1].model =
        mat4_translate((vec3){{5.0f, 0.0f, 0.0f, 0}});
    g_system.bundle.obj[1].diffuse_color = (vec4){{0.0f, 1.0f, 0.0f, 1.0f}};

#if DEBUG
    system_log();
#endif

    event_reg(g_system.event, EVENT_QUIT, game_on_event, NULL);
    event_reg(g_system.event, EVENT_SUSPEND, game_on_event, NULL);
    event_reg(g_system.event, EVENT_RESUME, game_on_event, NULL);
    event_reg(g_system.event, EVENT_RESIZE, game_on_resize, NULL);
    event_reg(g_system.event, EVENT_KEY_PRESS, game_on_input, NULL);
    event_reg(g_system.event, EVENT_KEY_RELEASE, game_on_input, NULL);

    return true;
}

static void system_kill(void) {
    g_system.game->is_running = false;

    event_unreg(g_system.event, EVENT_QUIT, game_on_event, NULL);
    event_unreg(g_system.event, EVENT_SUSPEND, game_on_event, NULL);
    event_unreg(g_system.event, EVENT_RESUME, game_on_event, NULL);
    event_unreg(g_system.event, EVENT_RESIZE, game_on_resize, NULL);
    event_unreg(g_system.event, EVENT_KEY_PRESS, game_on_input, NULL);
    event_unreg(g_system.event, EVENT_KEY_RELEASE, game_on_input, NULL);

    geo_system_kill(g_system.geo);
    render_system_kill(g_system.render);
    camera_system_kill(g_system.camera);
    input_system_kill(g_system.input);
    event_system_kill(g_system.event);
    window_system_kill(g_system.window);
    filesys_kill(g_system.fs);

    arena_kill(&g_system.frame_arena);
    arena_kill(&g_system.persistent_arena);

    game_kill(g_system.game);
    memory_system_kill();
}

int main(void) {
    system_init();

    const double TARGET_FPS = 60.0;
    const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

    g_system.game->is_running = true;
    timer_start(&g_system.game->timer);
    timer_update(&g_system.game->timer);
    g_system.game->last_time = g_system.game->timer.elapsed;

    double runtime = 0;
    uint8_t frame_count = 0;
    const bool limit = true;

    LOG_INFO("%s", mem_debug_stat());
    LOG_INFO("%s", vram_status(g_system.render));

    while (g_system.game->is_running) {
        if (!window_system_pump(g_system.window, g_system.input,
                                g_system.event)) {
            g_system.game->is_running = false;
        };

        if (!g_system.game->is_suspend) {
            timer_update(&g_system.game->timer);
            double curr_time = g_system.game->timer.elapsed;
            g_system.game->delta =
                (float)(curr_time - g_system.game->last_time);

            g_system.game->last_time = curr_time;
            double frame_time_start = get_abs_time();

            input_system_update(g_system.input, g_system.game->delta,
                                &g_system.frame_arena);

            if (!game_update(g_system.game, g_system.game->delta)) {
                g_system.game->is_running = false;
                break;
            }
            if (!game_render(g_system.game, g_system.game->delta)) {
                g_system.game->is_running = false;
                break;
            }

            render_system_draw(g_system.render, &g_system.bundle);

            double next_frame_time = frame_time_start + TARGET_FRAME_TIME;
            double frame_time_end = get_abs_time();
            double frame_elapsed = frame_time_end - frame_time_start;
            runtime += frame_elapsed;

            if (limit && frame_time_end < next_frame_time) {
                get_sleep(next_frame_time);
            }
            frame_count++;

#if DEBUG
            static double fps_timer = 0.0;
            fps_timer += g_system.game->delta;

            if (fps_timer >= 1.0) {
                printf("FPS: %d\n", frame_count);
                frame_count = 0;
                fps_timer = 0.0;
            }
#endif
            (void)runtime;
        }
    }

    system_kill();
    return 0;
}

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
                render_system_resize(g_system.window->native_win.width,
                                     g_system.window->native_win.height);
            }
        }
    }
    return false;
}
