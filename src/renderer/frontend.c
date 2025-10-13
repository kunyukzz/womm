#include "frontend.h"
#include "backend.h"
#include "core/memory.h"

#include <string.h>
#include <stdio.h>

static const char *tag_str[RE_COUNT] = {
    "RE_UNKNOWN",
    "RE_TEXTURE",        // Base color, normal, roughness/metallic maps
    "RE_TEXTURE_HDR",    // HDR cubemaps, IBL textures
    "RE_TEXTURE_UI",     // UI atlas, fonts
    "RE_BUFFER_VERTEX",  // Vertex buffers for meshes
    "RE_BUFFER_INDEX",   // Index buffers
    "RE_BUFFER_UNIFORM", // Uniform buffers (per-frame, per-object)
    "RE_BUFFER_STAGING", // Staging buffers for uploads
    "RE_BUFFER_COMPUTE", // SSBOs for compute shaders
    "RE_RENDER_TARGET",  // Color attachments, G-Buffers
    "RE_DEPTH_TARGET",   // Depth/Stencil attachments
};

char *vram_status(render_system_t *r) {
    const uint64_t Gib = 1024 * 1024 * 1024;
    const uint64_t Mib = 1024 * 1024;
    const uint64_t Kib = 1024;

    static char buffer[2048];
    uint64_t offset = 0;
    offset +=
        (uint64_t)snprintf(buffer + offset, sizeof(buffer) - offset,
                           "Engine VRAM Used: %.2f Mib / %.2f Mib\n",
                           (float)r->vk.core.memories.total_allocated / Mib,
                           (float)r->vk.core.memories.budget / Mib);

    for (uint16_t i = 0; i < RE_COUNT; ++i) {
        char *unit = "B";
        uint32_t count = (uint32_t)r->vk.core.memories.tag_alloc_count[i];
        float amount = (float)r->vk.core.memories.tag_alloc[i];

        if (count == 0) {
            continue;
        }
        if (amount >= (float)Gib) {
            amount /= (float)Gib;
            unit = "Gib";
        } else if (amount >= (float)Mib) {
            amount /= (float)Mib;
            unit = "Mib";
        } else if (amount >= (float)Kib) {
            amount /= (float)Kib;
            unit = "Kib";
        }
        count = (uint32_t)r->vk.core.memories.tag_alloc_count[i];

        int32_t length =
            snprintf(buffer + offset, sizeof(buffer) - offset,
                     "--> %s: [%u] %.2f%s\n", tag_str[i], count, amount, unit);

        if (length > 0 && (offset + (uint32_t)length < 2048)) {
            offset += (uint32_t)length;
        } else {
            break;
        }
    }
    return buffer;
}

/************************************
 * SYNCRONIZATION
 ************************************/
static bool set_sync(render_system_t *r) {
    VkDevice dev = r->vk.core.logic_dvc;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint16_t i = 0; i < FRAME_FLIGHT; ++i) {
        CHECK_VK(re.vkCreateFence(dev, &fence_info, r->vk.core.alloc,
                                  &r->vk.frame_fence[i]));
    }
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (uint16_t i = 0; i < FRAME_FLIGHT; ++i) {
        CHECK_VK(re.vkCreateSemaphore(dev, &sem_info, r->vk.core.alloc,
                                      &r->vk.avail_sema[i]));
    }

    // allocation for image count from swapchain
    uint32_t count = r->vk.swap.image_count;
    r->vk.image_fence = WALLOC(sizeof(VkSemaphore) * count, MEM_RENDER);
    for (uint32_t i = 0; i < count; ++i) {
        r->vk.image_fence[i] = VK_NULL_HANDLE;
    }

    r->vk.done_sema = WALLOC(sizeof(VkSemaphore) * count, MEM_RENDER);
    for (uint32_t i = 0; i < count; ++i) {
        VkSemaphoreCreateInfo sm_info = {};
        sm_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        re.vkCreateSemaphore(r->vk.core.logic_dvc, &sm_info, r->vk.core.alloc,
                             &r->vk.done_sema[i]);
    }

    return true;
}

static void unset_sync(render_system_t *r) {
    uint32_t count = r->vk.swap.image_count;
    for (uint32_t i = 0; i < count; ++i) {
        re.vkDestroySemaphore(r->vk.core.logic_dvc, r->vk.done_sema[i],
                              r->vk.core.alloc);
    }
    WFREE(r->vk.done_sema, sizeof(VkSemaphore) * count, MEM_RENDER);
    WFREE(r->vk.image_fence, sizeof(VkSemaphore) * count, MEM_RENDER);

    for (uint16_t i = 0; i < FRAME_FLIGHT; ++i) {
        if (r->vk.avail_sema[i]) {
            re.vkDestroySemaphore(r->vk.core.logic_dvc, r->vk.avail_sema[i],
                                  r->vk.core.alloc);
            r->vk.avail_sema[i] = VK_NULL_HANDLE;
        }
        if (r->vk.frame_fence[i]) {
            re.vkDestroyFence(r->vk.core.logic_dvc, r->vk.frame_fence[i],
                              r->vk.core.alloc);
            r->vk.frame_fence[i] = VK_NULL_HANDLE;
        }
    }
}

static bool set_cmdbuffer(render_system_t *r) {
    for (uint16_t i = 0; i < FRAME_FLIGHT; ++i) {
        cmdbuff_alloc(&r->vk.core, &r->vk.cmds[i], r->vk.core.gfx_pool);
        LOG_TRACE("allocate command buffer %d: handle: %p", i,
                  (void *)r->vk.cmds[i].handle);
    }
    return true;
}

static void unset_cmdbuffer(render_system_t *r) {
    for (uint16_t i = 0; i < FRAME_FLIGHT; ++i) {
        LOG_TRACE("free command buffer %d: handle: %p", i,
                  (void *)r->vk.cmds[i].handle);
        cmdbuff_free(&r->vk.core, &r->vk.cmds[i], r->vk.core.gfx_pool);
    }
}

/************************************
 * FRAMEBUFFER SET
 ************************************/
static void start_framebuffer(render_system_t *r) {
    for (uint32_t i = 0; i < r->vk.swap.image_count; ++i) {
        VkImageView world_attach[2] = {r->vk.swap.img_views[i],
                                       r->vk.swap.depth_attach.view};

        uint32_t fb_attachment_count = 2;

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = r->vk.main_pass.handle;
        fb_info.attachmentCount = fb_attachment_count;
        fb_info.pAttachments = world_attach;
        fb_info.width = r->vk.swap.extents.width;
        fb_info.height = r->vk.swap.extents.height;
        fb_info.layers = 1;

        CHECK_VK(re.vkCreateFramebuffer(r->vk.core.logic_dvc, &fb_info,
                                        r->vk.core.alloc,
                                        &r->vk.main_framebuff[i]));

        /*
        VkImageView attachments_ui[1] = {be->swap.img_views[i]};
        VkFramebufferCreateInfo ufb_info = {};
        ufb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ufb_info.renderPass = be->overlay_pass.handle;
        ufb_info.attachmentCount = 1;
        ufb_info.pAttachments = attachments_ui;
        ufb_info.width = be->swap.extents.width;
        ufb_info.height = be->swap.extents.height;
        ufb_info.layers = 1;

        VK_CHK(be->core.vk.dvc.vkCreateFramebuffer(be->core.logic_dvc,
                                                   &ufb_info, be->core.alloc,
                                                   &be->swap.framebuffer[i]));
                                                   */
    }
}

static void stop_framebuffer(render_system_t *r) {
    for (uint32_t i = 0; i < r->vk.swap.image_count; ++i) {
        re.vkDestroyFramebuffer(r->vk.core.logic_dvc, r->vk.main_framebuff[i],
                                r->vk.core.alloc);

        re.vkDestroyFramebuffer(r->vk.core.logic_dvc, r->vk.swap.framebuffer[i],
                                r->vk.core.alloc);
    }
}

static bool set_framebuffer(render_system_t *r) {
    uint32_t count = r->vk.swap.image_count;
    r->vk.main_framebuff = WALLOC(sizeof(VkFramebuffer) * count, MEM_RENDER);
    r->vk.swap.framebuffer = WALLOC(sizeof(VkFramebuffer) * count, MEM_RENDER);
    start_framebuffer(r);
    return true;
}

static void unset_framebuffer(render_system_t *r) {
    stop_framebuffer(r);
    WFREE(r->vk.swap.framebuffer,
          sizeof(VkFramebuffer) * r->vk.swap.image_count, MEM_RENDER);
    WFREE(r->vk.main_framebuff, sizeof(VkFramebuffer) * r->vk.swap.image_count,
          MEM_RENDER);
}

/************************************
 * INTERNAL FRAME
 ************************************/
static bool begin_frame(render_system_t *r, float delta) {
    (void)delta;
    vk_core_t *core = &r->vk.core;
    uint32_t frames = r->vk.frame_idx;

    /*
    if (cache_frbuff_width != 0 || cache_frbuff_height != 0) {
        set_reinit_swapchain(be);
    }
    */

    /* wait current frame fence (frame-in-flight sync) */
    re.vkWaitForFences(core->logic_dvc, 1, &r->vk.frame_fence[frames], true,
                       UINT64_MAX);

    /* acquire next image from the swapchain */
    VkResult res =
        re.vkAcquireNextImageKHR(core->logic_dvc, r->vk.swap.handle, UINT64_MAX,
                                 r->vk.avail_sema[frames], VK_NULL_HANDLE,
                                 &r->vk.image_idx);

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        // set_reinit_swapchain(be);
        return false;
    } else if (res == VK_SUBOPTIMAL_KHR) {
        // set_reinit_swapchain(be);
    } else if (res != VK_SUCCESS) {
        LOG_ERROR("vkAcquireNextImageKHR failed: %d", res);
        return false;
    }

    /* wait for image-in-flight fence (per-image sync) */
    uint32_t images = r->vk.image_idx;
    if (r->vk.image_fence[images] != VK_NULL_HANDLE) {
        re.vkWaitForFences(core->logic_dvc, 1, &r->vk.image_fence[images],
                           VK_TRUE, UINT64_MAX);
    }

    /* mark current image as now in use by the current frame fence
     * then reset fences */
    r->vk.image_fence[images] = r->vk.frame_fence[frames];
    re.vkResetFences(core->logic_dvc, 1, &r->vk.frame_fence[frames]);

    cmdbuff_reset(&r->vk.cmds[frames]);
    cmdbuff_begin(&r->vk.cmds[frames], SUBMIT_ONE_TIME);

    VkViewport viewport = {};
    viewport.x = viewport.y = 0;
    viewport.width = (float)r->vk.swap.extents.width;
    viewport.height = (float)r->vk.swap.extents.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent = r->vk.swap.extents;

    VkCommandBuffer cmd = r->vk.cmds[r->vk.frame_idx].handle;
    re.vkCmdSetViewport(cmd, 0, 1, &viewport);
    re.vkCmdSetScissor(cmd, 0, 1, &scissor);
    return true;
}

static bool end_frame(render_system_t *r, float delta) {
    (void)delta;
    vk_core_t *core = &r->vk.core;
    uint32_t frames = r->vk.frame_idx;

    cmdbuff_end(&r->vk.cmds[frames]);

    /* submit to graphics queue */
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    uint32_t images = r->vk.image_idx;
    VkSemaphore wait_semaphores[] = {r->vk.avail_sema[frames]};
    VkSemaphore signal_semaphores[] = {r->vk.done_sema[images]};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &r->vk.cmds[frames].handle;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    CHECK_VK(re.vkQueueSubmit(core->graphic_queue, 1, &submit_info,
                              r->vk.frame_fence[frames]));

    /* present the frame */
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &r->vk.swap.handle;
    present_info.pImageIndices = &images;

    VkResult res = re.vkQueuePresentKHR(core->present_queue, &present_info);

    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        // set_reinit_swapchain(be);
        return false;
    } else if (res != VK_SUCCESS) {
        LOG_ERROR("vkQueuePresentKHR failed: %d", res);
        return false;
    }

    /* advance to next frame-in-flight */
    r->vk.frame_idx = (frames + 1) % FRAME_FLIGHT;
    return true;
}

/************************************
 * RENDERPASS
 ************************************/
static bool begin_pass(render_system_t *r) {
    VkCommandBuffer cmd = r->vk.cmds[r->vk.frame_idx].handle;
    VkFramebuffer frbuffer = r->vk.main_framebuff[r->vk.image_idx];

    renderpass_begin(&r->vk.main_pass, cmd, frbuffer, r->vk.swap.extents);
    return true;
}

static bool end_pass(render_system_t *r) {
    VkCommandBuffer cmd = r->vk.cmds[r->vk.frame_idx].handle;
    renderpass_end(&r->vk.main_pass, cmd);
    return true;
}

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window) {
    render_system_t *r = arena_alloc(arena, sizeof(render_system_t));
    if (!r) return NULL;

    memset(r, 0, sizeof(render_system_t));
    r->arena = arena;
    r->window = window;

    if (!core_init(&r->vk.core)) {
        LOG_FATAL("core render not initialized");
        return NULL;
    }
    if (!swapchain_init(&r->vk.swap, &r->vk.core, window, VK_NULL_HANDLE)) {
        LOG_FATAL("swapchain render not initialized");
        return NULL;
    }

    VkClearColorValue world_clear_color = {{0.3f, 0.2f, 0.5f, 1.0f}};
    if (!renderpass_init(&r->vk.core, &r->vk.main_pass, &r->vk.swap, 1.0f, 0,
                         COLOR_BUFFER | DEPTH_BUFFER, false, true,
                         world_clear_color)) {
        LOG_FATAL("main renderpass not initialized");
        return false;
    }
    LOG_DEBUG("main renderpass initialize");

    set_sync(r);
    set_cmdbuffer(r);
    set_framebuffer(r);

    LOG_INFO("render system initialized");
    return r;
}

void render_system_kill(render_system_t *r) {
    if (r) {
        re.vkDeviceWaitIdle(r->vk.core.logic_dvc);
        unset_framebuffer(r);
        unset_cmdbuffer(r);
        unset_sync(r);
        renderpass_kill(&r->vk.core, &r->vk.main_pass);
        swapchain_kill(&r->vk.swap, &r->vk.core);
        core_kill(&r->vk.core);

        memset(r, 0, sizeof(render_system_t));
    }
    LOG_INFO("render system kill");
}

bool render_system_draw(render_system_t *r, render_bundle_t *bundle) {
    if (begin_frame(r, bundle->delta)) {

        if (!begin_pass(r)) {
            return false;
        }

        // TODO: draw world bundle

        if (!end_pass(r)) {
            return false;
        }

        end_frame(r, bundle->delta);
    }
    return true;
}
