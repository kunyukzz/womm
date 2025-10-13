#ifndef RENDERER_FRONTEND_H
#define RENDERER_FRONTEND_H

#include "core/define.h"
#include "platform/window.h"
#include "backend_type.h"

typedef struct {
    vk_core_t core;
    vk_swapchain_t swap;
    vk_renderpass_t main_pass;
    vk_cmdbuffer_t cmds[FRAME_FLIGHT];

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

    render_t vk;
} render_system_t;

typedef struct {
    float delta;
} render_bundle_t;

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window);
void render_system_kill(render_system_t *r);

bool render_system_draw(render_system_t *r, render_bundle_t *bundle);

void render_system_resize(uint32_t width, uint32_t height);

char *vram_status(render_system_t *r);

#endif // RENDERER_FRONTEND_H
