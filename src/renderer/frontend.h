#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "core/define.h" // IWYU pragma: keep
#include "core/camera.h"
#include "frontend_type.h"
#include "backend_type.h"

typedef struct {
    vk_core_t core;
    vk_swapchain_t swap;
    vk_renderpass_t main_pass;
    vk_cmdbuffer_t cmds[FRAME_FLIGHT];

    geo_gpu_t geo_gpu;
    vk_buffer_t vertex_buffer;
    uint32_t vertex_offset;
    vk_buffer_t index_buffer;
    uint32_t index_offset;

    vk_material_t main_material;

    VkSemaphore avail_sema[FRAME_FLIGHT];
    VkSemaphore *done_sema;
    VkFence frame_fence[FRAME_FLIGHT];
    VkFence *image_fence;
    VkFramebuffer *main_framebuff;

    uint32_t frame_idx;
    uint32_t image_idx;
} render_t;

typedef struct {
    arena_alloc_t *arena;
    window_t *window;
    camera_t *camera;

    render_t vk;
    render_bundle_t bundle;
} render_system_t;

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window);
void render_system_kill(render_system_t *r);

bool render_system_draw(render_system_t *r, render_bundle_t *bundle);

void render_system_resize(uint32_t width, uint32_t height);

char *vram_status(render_system_t *r);

void render_geo_init(geo_gpu_t *geo, uint32_t v_size, uint32_t v_count,
                     const void *vert, uint32_t i_size, uint32_t i_count,
                     const void *indices);

#endif // RENDERER_FRONTEND_H
