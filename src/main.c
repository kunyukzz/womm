#include "core/define.h"
#include "core/event.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/timer.h"
#include "core/math/maths.h"
#include "platform/filesystem.h"
#include "renderer/frontend.h"
#include "module/geometry.h"
#include "module/material.h"
#include "module/texture.h"
#include "game/game.h"

#include <stdio.h>

// for all module system
typedef struct {
    arena_alloc_t persistent_arena;
    arena_alloc_t frame_arena;

    timer_t timer;
    render_bundle_t bundle;

    file_system_t *fs;
    window_system_t *window;
    event_system_t *event;
    input_system_t *input;
    camera_system_t *camera;
    render_system_t *render;
    geometry_system_t *geo;
    texture_system_t *tex;
    material_system_t *mat;
    game_system_t *game;

    float delta;
    double last_time;
    bool is_running;
    bool is_suspend;
} system_t;

static system_t g_system;

bool game_on_input(uint32_t type, event_t *ev, void *sender, void *recipient);
bool game_on_event(uint32_t type, event_t *ev, void *sender, void *recipient);
bool game_on_resize(uint32_t type, event_t *ev, void *sender, void *recipient);

// TODO: Temporary code!!
static float rot_angle_y1 = 0.0f;
static float rot_angle_y2 = 0.0f;
static float rot_speed_y1 = 0.0f;
static float rot_speed_y2 = 0.0f;

void init_random_rotation() {
    float min = -0.5f;
    float max = 0.5f;
    rot_speed_y1 = frandom_in_range(min, max);
    rot_speed_y2 = frandom_in_range(min, max);
}

void update_cube_rotation(float delta_time) {
    rot_angle_y1 += rot_speed_y1 * delta_time;
    rot_angle_y2 += rot_speed_y2 * delta_time;

    mat4 cube_tr1 = mat4_translate((vec3){{-5.0f, -2.5f, 0.0f, 0}});
    mat4 cube_rot1 = mat4_euler_y(rot_angle_y1);
    g_system.bundle.obj[0].model = mat4_column_multi(cube_tr1, cube_rot1);

    mat4 cube_tr2 = mat4_translate((vec3){{5.0f, -2.5f, 0.0f, 0}});
    mat4 cube_rot2 = mat4_euler_y(rot_angle_y2);
    g_system.bundle.obj[1].model = mat4_column_multi(cube_tr2, cube_rot2);
}
// TODO: Temporary code end!!

#if DEBUG
static void system_log(void) {
    // LOG_DEBUG("=== Memory Addresses ===");
    // LOG_DEBUG("Event:     %p", g_system.event);
    // LOG_DEBUG("Input:     %p", g_system.input);
    // LOG_DEBUG("Window:    %p", g_system.window);
    // LOG_DEBUG("Camera:    %p", g_system.camera);
    // LOG_DEBUG("Render:    %p", g_system.render);
    // LOG_DEBUG("Geometry:  %p", g_system.geo);
    // LOG_DEBUG("Texture:   %p", g_system.tex);
    // LOG_DEBUG("Material:  %p", g_system.mat);

    uint64_t used = arena_used(&g_system.persistent_arena);
    uint64_t total = g_system.persistent_arena.total_size;
    float usage_percent = (float)used / (float)total * 100.0f;

    LOG_TRACE("Arena Usage: %lu/%lu bytes (%.1f%%)", used, total,
              usage_percent);

    if (usage_percent > 90.0f) {
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
    g_system.event = event_system_init(&g_system.persistent_arena);
    g_system.input = input_system_init(&g_system.persistent_arena);

    g_system.window = window_system_init(config, &g_system.persistent_arena);
    g_system.camera = camera_system_init(&g_system.persistent_arena,
                                         &g_system.window->native_win);
    g_system.render = render_system_init(&g_system.persistent_arena,
                                         &g_system.window->native_win);

    g_system.geo = geo_system_init(&g_system.persistent_arena);
    g_system.tex = texture_system_init(&g_system.persistent_arena);
    g_system.mat = material_system_init(&g_system.persistent_arena);
    g_system.game = game_init();

    // bundle initialize
    // g_system.bundle.delta = g_system.delta;

    // TODO: all of this was temporary code!!!
    g_system.bundle.obj[0].geo = &g_system.geo->cube;
    g_system.bundle.obj[0].model =
        mat4_translate((vec3){.comp1.x = -5.0f, -2.5f, 0.0f, 0});
    g_system.bundle.obj[0].material.diffuse_color =
        (vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
    g_system.bundle.obj[0].material.tex = &g_system.tex->gear_base;
    g_system.bundle.obj[0].material.texture_index = 0;

    g_system.bundle.obj[1].geo = &g_system.geo->cube;
    g_system.bundle.obj[1].model =
        mat4_translate((vec3){.comp1.x = 5.0f, -2.5f, 0.0f, 0});
    g_system.bundle.obj[1].material.diffuse_color =
        (vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
    g_system.bundle.obj[1].material.tex = &g_system.tex->vulkan_logo;
    g_system.bundle.obj[1].material.texture_index = 1;

    g_system.bundle.obj[2].geo = &g_system.geo->plane;
    vec3 scale = (vec3){{3.0f, 1.0f, 3.0f, 0}};
    g_system.bundle.obj[2].model = mat4_scale(scale);
    g_system.bundle.obj[2].material.diffuse_color =
        (vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
    g_system.bundle.obj[2].material.tex = &g_system.tex->memes;
    g_system.bundle.obj[2].material.texture_index = 2;

    g_system.bundle.ui_obj.geo = &g_system.geo->plane2D;
    g_system.bundle.ui_obj.model = mat4_translate((vec3){{10.0f, 10.0f, 0, 0}});
    g_system.bundle.ui_obj.material.diffuse_color =
        (vec4){{1.0f, 1.0f, 1.0f, 1.0f}};
    g_system.bundle.ui_obj.material.tex = &g_system.tex->debugUI;

    g_system.bundle.world_obj_count = 3;
    g_system.bundle.debug_ui_count = 1;

#if DEBUG
    system_log();
#endif

    event_reg(EVENT_RESIZE, game_on_resize, NULL);
    event_reg(EVENT_RESUME, game_on_event, NULL);
    event_reg(EVENT_SUSPEND, game_on_event, NULL);
    event_reg(EVENT_QUIT, game_on_event, NULL);
    event_reg(EVENT_KEY_PRESS, game_on_input, NULL);
    event_reg(EVENT_KEY_RELEASE, game_on_input, NULL);

    // init_random_rotation();

    return true;
}

static void system_kill(void) {
    g_system.is_running = false;

    event_unreg(EVENT_KEY_RELEASE, game_on_input, NULL);
    event_unreg(EVENT_KEY_PRESS, game_on_input, NULL);
    event_unreg(EVENT_QUIT, game_on_event, NULL);
    event_unreg(EVENT_SUSPEND, game_on_event, NULL);
    event_unreg(EVENT_RESUME, game_on_event, NULL);
    event_unreg(EVENT_RESIZE, game_on_resize, NULL);

    material_system_kill(g_system.mat);
    texture_system_kill(g_system.tex);
    geo_system_kill(g_system.geo);
    render_system_kill(g_system.render);
    camera_system_kill(g_system.camera);
    window_system_kill(g_system.window);
    input_system_kill(g_system.input);
    event_system_kill(g_system.event);
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

    g_system.is_running = true;
    timer_start(&g_system.timer);
    timer_update(&g_system.timer);
    g_system.last_time = g_system.timer.elapsed;

    double runtime = 0;
    uint8_t frame_count = 0;
    const bool limit = false;

    LOG_INFO("%s", mem_debug_stat());
    LOG_INFO("%s", vram_status(g_system.render));

    while (g_system.is_running) {
        if (!window_system_pump()) {
            g_system.is_running = false;
        };

        if (!g_system.is_suspend) {
            timer_update(&g_system.timer);
            double curr_time = g_system.timer.elapsed;
            float delta = (float)(curr_time - g_system.last_time);

            g_system.last_time = curr_time;
            double frame_time_start = get_abs_time();

            input_system_update(delta);

            if (!game_update(g_system.game, delta)) {
                g_system.is_running = false;
                break;
            }
            if (!game_render(g_system.game, delta)) {
                g_system.is_running = false;
                break;
            }

            // update_cube_rotation(g_system.game->delta);
            g_system.bundle.delta = delta;
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
            fps_timer += delta;

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

bool game_on_input(uint32_t type, event_t *ev, void *sender, void *recipient) {
    (void)sender;
    (void)recipient;
    if (type == EVENT_KEY_PRESS) {
        uint32_t kc = ev->data.keys.keycode;
        if (kc == INPUT_KEY_ESCAPE) {
            event_t evquit = {};
            event_push(EVENT_QUIT, &evquit, NULL);
            return true;
        } else {
            // LOG_DEBUG("'%s' pressed in window", keycode_to_str(kc));
        }
    } else if (type == EVENT_KEY_RELEASE) {
        uint32_t kc = ev->data.keys.keycode;
        (void)kc;
        // LOG_DEBUG("'%s' released", keycode_to_str(kc));
    }
    return false;
}

bool game_on_event(uint32_t type, event_t *ev, void *sender, void *recipient) {
    (void)ev;
    (void)sender;
    (void)recipient;
    switch (type) {
        case EVENT_QUIT: {
            LOG_INFO("EVENT_QUIT received. Shutdown...");
            g_system.is_running = false;
            return true;
        } break;
        case EVENT_SUSPEND: {
            LOG_INFO("EVENT_SUSPEND received. Suspend...");
            g_system.is_suspend = true;
            return true;
        } break;
        case EVENT_RESUME: {
            LOG_INFO("EVENT_RESUME received. Resume...");
            g_system.is_suspend = false;
            g_system.is_running = true;
            return true;
        } break;
    }
    return false;
}

bool game_on_resize(uint32_t type, event_t *ev, void *sender, void *recipient) {
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
                g_system.is_suspend = true;
                return true;
            } else {
                if (g_system.is_suspend) {
                    g_system.is_suspend = false;
                }
                render_system_resize(g_system.window->native_win.width,
                                     g_system.window->native_win.height);
            }
        }
    }
    return false;
}
