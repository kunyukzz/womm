#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "core/define.h"
#include "platform/window.h"
#include "backend_type.h"

typedef struct {
    arena_alloc_t *arena;
    window_t *window;

    vk_core_t core;
    vk_swapchain_t swap;

} render_system_t;

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window);
void render_system_kill(render_system_t *re);

bool render_system_draw(void);

void render_system_resize(uint32_t width, uint32_t height);

char *vram_status(render_system_t *re);

#endif // RENDERER_FRONTEND_H
