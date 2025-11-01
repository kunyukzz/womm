#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "core/define.h" // IWYU pragma: keep
#include "platform/window.h"
#include "frontend_type.h"

typedef enum {
    WORLD_PASS = 0x01,
    DEBUG_UI_PASS = 0x02,
} render_layer_t;

typedef struct render_system_t render_system_t;

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window);

void render_system_kill(render_system_t *r);

bool render_system_draw(render_system_t *r, render_bundle_t *bundle);

void render_system_resize(uint32_t width, uint32_t height);

char *vram_status(render_system_t *r);

void render_geo_init(geo_gpu_t *geo, uint32_t v_size, uint32_t v_count,
                     const void *vert, uint32_t i_size, uint32_t i_count,
                     const void *indices);

bool render_tex_init(const uint8_t *pixel, texture_data_t *tex_data);
void render_tex_kill(texture_data_t *tex_data);

#endif // RENDERER_FRONTEND_H
