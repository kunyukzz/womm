#include "frontend.h"
#include "backend.h"
#include "core/memory.h"
#include "core/math/math_type.h"

#include <string.h>
#include <stdio.h>

static render_system_t *g_re = NULL;

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
 * BUFFER VERTEX & INDEX SET
 ************************************/
static bool set_object_buffer(render_system_t *r) {
    VkMemoryPropertyFlagBits mem_prop = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const uint64_t vb_size_3d = sizeof(vertex_3d) * 1024 * 1024; // 32Mb
    buffer_init(&r->vk.core, &r->vk.vertex_buffer,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                vb_size_3d, mem_prop, RE_BUFFER_VERTEX);

    const uint32_t ib_size = sizeof(uint32_t) * 1024 * 1024; // 4Mb
    buffer_init(&r->vk.core, &r->vk.index_buffer,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                ib_size, mem_prop, RE_BUFFER_INDEX);

    LOG_DEBUG("vulkan buffer initialize");
    return true;
}

static void unset_object_buffer(render_system_t *r) {
    VkMemoryPropertyFlagBits mem_prop = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (r->vk.vertex_buffer.handle != VK_NULL_HANDLE) {
        buffer_kill(&r->vk.core, &r->vk.vertex_buffer, mem_prop,
                    RE_BUFFER_VERTEX);
    }

    if (r->vk.index_buffer.handle != VK_NULL_HANDLE) {
        buffer_kill(&r->vk.core, &r->vk.index_buffer, mem_prop,
                    RE_BUFFER_INDEX);
    }
    LOG_DEBUG("vulkan buffer kill");
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
    // re.vkCmdSetLineWidth(cmd, 1.0f);
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

/************************************
 * DRAW CALL
 ************************************/
static bool draw_world(render_system_t *r, render_bundle_t *bundle) {
    VkDescriptorSet sets = r->vk.main_material.sets[r->vk.frame_idx];
    VkCommandBuffer cmd = r->vk.cmds[r->vk.frame_idx].handle;
    vk_pipeline_t pipeline = r->vk.main_material.pipelines;

    pipeline_bind(&pipeline, cmd, VK_PIPELINE_BIND_POINT_GRAPHICS);

    re.vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               pipeline.layout, 0, 1, &sets, 0, 0);

    /*
    re.vkCmdPushConstants(cmd, pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                          sizeof(mat4), &bundle->model);
                          */

    VkBuffer buff[] = {r->vk.vertex_buffer.handle};
    VkDeviceSize offset[1] = {bundle->geo->vertex_offset};

    /*
    LOG_DEBUG("vertex_offset=%llu index_offset=%llu index_count=%u "
              "stride=%zu",
              bundle->geo->vertex_offset, bundle->geo->index_offset,
              bundle->geo->index_count, sizeof(vertex_3d));
              */

    re.vkCmdBindVertexBuffers(cmd, 0, 1, buff, offset);
    re.vkCmdBindIndexBuffer(cmd, r->vk.index_buffer.handle,
                            bundle->geo->index_offset, VK_INDEX_TYPE_UINT32);

    re.vkCmdDrawIndexed(cmd, bundle->geo->index_count, 1, 0, 0, 0);
    return true;
}

static void update_world(render_system_t *r) {}

/************************************
 * STAGING BUFFER DATA
 ************************************/
static void set_staging_data(render_system_t *r, vk_buffer_t *buffer,
                             VkCommandPool pool, VkQueue queue,
                             VkDeviceSize offset, void *data, VkDeviceSize size,
                             vram_tag_t tag) {
    vk_core_t *core = &r->vk.core;
    VkMemoryPropertyFlags mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    vk_buffer_t staging;
    buffer_init(core, &staging, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, size,
                mem_flags, tag);

    buffer_load(core, &staging, 0, size, data);
    buffer_copy(core, staging.handle, buffer->handle, 0, offset, size, pool,
                queue);

    buffer_kill(core, &staging, mem_flags, tag);
}

/************************************************************************
 ************************************************************************/

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window) {
    if (g_re != NULL) return g_re;

    render_system_t *r = arena_alloc(arena, sizeof(render_system_t));
    if (!r) return NULL;

    memset(r, 0, sizeof(render_system_t));
    r->arena = arena;
    r->window = window;
    // r->camera = get_main_camera();

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
    set_object_buffer(r);

    material_world_init(&r->vk.core, &r->vk.main_material, &r->vk.main_pass,
                        "shaders/base");

    /*
    { // TODO: Temporary code!!
        const uint32_t vertex_count = 4;
        vertex_3d vert_3d[vertex_count];
        memset(vert_3d, 0, sizeof(vertex_3d) * vertex_count);

        vert_3d[0].position.comp1.x = -0.5f;
        vert_3d[0].position.comp1.y = 0.5f;
        vert_3d[0].texcoord.comp1.x = 0;
        vert_3d[0].texcoord.comp1.y = 0;

        vert_3d[1].position.comp1.x = 0.5f;
        vert_3d[1].position.comp1.y = 0.5f;
        vert_3d[1].texcoord.comp1.x = 1;
        vert_3d[1].texcoord.comp1.y = 0;

        vert_3d[2].position.comp1.x = 0.5f;
        vert_3d[2].position.comp1.y = -0.5f;
        vert_3d[2].texcoord.comp1.x = 1;
        vert_3d[2].texcoord.comp1.y = 1;

        vert_3d[3].position.comp1.x = -0.5f;
        vert_3d[3].position.comp1.y = -0.5f;
        vert_3d[3].texcoord.comp1.x = 0;
        vert_3d[3].texcoord.comp1.y = 1;

        const uint32_t idx_count = 6;
        uint32_t indices[6] = {0, 1, 2, 0, 2, 3};

        set_staging_data(r, &r->vk.vertex_buffer, r->vk.core.gfx_pool,
                         r->vk.core.graphic_queue, 0, vert_3d,
                         sizeof(vertex_3d) * vertex_count, RE_BUFFER_STAGING);

        set_staging_data(r, &r->vk.index_buffer, r->vk.core.gfx_pool,
                         r->vk.core.graphic_queue, 0, indices,
                         sizeof(uint32_t) * idx_count, RE_BUFFER_STAGING);
    }
    */

    g_re = r;
    LOG_INFO("render system initialized");
    return r;
}

void render_system_kill(render_system_t *r) {
    if (!r) return;

    re.vkDeviceWaitIdle(r->vk.core.logic_dvc);

    material_kill(&r->vk.core, &r->vk.main_material);

    unset_object_buffer(r);
    unset_framebuffer(r);
    unset_cmdbuffer(r);
    unset_sync(r);

    renderpass_kill(&r->vk.core, &r->vk.main_pass);
    swapchain_kill(&r->vk.swap, &r->vk.core);
    core_kill(&r->vk.core);

    memset(r, 0, sizeof(render_system_t));

    LOG_INFO("render system kill");
}

bool render_system_draw(render_system_t *r, render_bundle_t *bundle) {
    if (begin_frame(r, bundle->delta)) {

        if (!begin_pass(r)) {
            return false;
        }

        draw_world(r, bundle);

        if (!end_pass(r)) {
            return false;
        }

        end_frame(r, bundle->delta);
    }
    return true;
}

void render_geo_init(geo_gpu_t *geo, uint32_t v_size, uint32_t v_count,
                     const void *vert, uint32_t i_size, uint32_t i_count,
                     const void *indices) {
    geo->vertex_offset = g_re->vk.vertex_offset;
    geo->vertex_count = v_count;
    geo->vertex_size = v_size;
    uint32_t total_size = v_size * v_count;

    printf("[Vertex] offset=%u, count=%u, size=%u, total_bytes=%u\n",
           geo->vertex_offset, geo->vertex_count, geo->vertex_size, total_size);

    set_staging_data(g_re, &g_re->vk.vertex_buffer, g_re->vk.core.gfx_pool,
                     g_re->vk.core.graphic_queue, geo->vertex_offset,
                     (void *)vert, total_size, RE_BUFFER_STAGING);

    g_re->vk.vertex_offset += total_size;

    if (i_count && indices) {
        geo->index_offset = g_re->vk.index_offset;
        geo->index_count = i_count;
        geo->index_size = i_size;
        total_size = i_size * i_count;

        printf("[Index]  offset=%u, count=%u, size=%u, total_bytes=%u\n",
               geo->index_offset, geo->index_count, geo->index_size,
               total_size);

        set_staging_data(g_re, &g_re->vk.index_buffer, g_re->vk.core.gfx_pool,
                         g_re->vk.core.graphic_queue, geo->index_offset,
                         (void *)indices, total_size, RE_BUFFER_STAGING);

        g_re->vk.index_offset += total_size;
    }

    // g_re->vk.geo_gpu = *geo;
    // g_re->bundle.geo = &g_re->vk.geo_gpu;
    // g_re->bundle.model = g_re->camera->world_view;
}
