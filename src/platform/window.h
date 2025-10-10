#ifndef WINDOW_H
#define WINDOW_H

#include "core/define.h"
#include "core/event.h"
#include "core/input.h"

typedef struct {
    uint32_t width;
    uint32_t height;
    const char *name;

    bool is_resizeable;
} window_config_t;

// TODO: this for Linux for now
// this for vulkan renderer later. that's why using opaque void pointer
typedef struct {
#if PLATFORM_LINUX
    void *display;
    uintptr_t win;
    uintptr_t atom;
#elif PLATFORM_WINDOWS

#endif

    uint32_t width;
    uint32_t height;
    const char *name;
} window_t;

typedef struct {
    arena_alloc_t *arena;
    window_t native_win;
} window_system_t;

// initializer state
window_system_t *window_system_init(window_config_t config,
                                    arena_alloc_t *arena);
void window_system_kill(window_system_t *window);

bool window_system_pump(window_system_t *window, input_system_t *input,
                        event_system_t *event);

void *window_system_get_native_display(const window_system_t *window);
void *window_system_get_native_window(const window_system_t *window);

double get_abs_time(void);
void get_sleep(double wake);

#endif // WINDOW_H
