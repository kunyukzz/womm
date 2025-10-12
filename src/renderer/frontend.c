#include "frontend.h"
#include "backend.h"

#include <string.h>
#include <stdio.h>

static const char *tag_str[RE_COUNT] = {
    "RDR_TAG_UNKNOWN",
    "RDR_TAG_TEXTURE",        // Base color, normal, roughness/metallic maps
    "RDR_TAG_TEXTURE_HDR",    // HDR cubemaps, IBL textures
    "RDR_TAG_TEXTURE_UI",     // UI atlas, fonts
    "RDR_TAG_BUFFER_VERTEX",  // Vertex buffers for meshes
    "RDR_TAG_BUFFER_INDEX",   // Index buffers
    "RDR_TAG_BUFFER_UNIFORM", // Uniform buffers (per-frame, per-object)
    "RDR_TAG_BUFFER_STAGING", // Staging buffers for uploads
    "RDR_TAG_BUFFER_COMPUTE", // SSBOs for compute shaders
    "RDR_TAG_RENDER_TARGET",  // Color attachments, G-Buffers
    "RDR_TAG_DEPTH_TARGET",   // Depth/Stencil attachments
};

char *vram_status(render_system_t *re) {
    const uint64_t Gib = 1024 * 1024 * 1024;
    const uint64_t Mib = 1024 * 1024;
    const uint64_t Kib = 1024;

    static char buffer[2048];
    uint64_t offset = 0;
    offset += (uint64_t)snprintf(buffer + offset, sizeof(buffer) - offset,
                                 "Engine VRAM Used: %.2f Mib / %.2f Mib\n",
                                 (float)re->core.memories.total_allocated / Mib,
                                 (float)re->core.memories.budget / Mib);

    for (uint16_t i = 0; i < RE_COUNT; ++i) {
        char *unit = "B";
        uint32_t count = (uint32_t)re->core.memories.tag_alloc_count[i];
        float amount = (float)re->core.memories.tag_alloc[i];

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
        count = (uint32_t)re->core.memories.tag_alloc_count[i];

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

render_system_t *render_system_init(arena_alloc_t *arena, window_t *window) {
    render_system_t *re = arena_alloc(arena, sizeof(render_system_t));
    if (!re) return NULL;

    memset(re, 0, sizeof(render_system_t));
    re->arena = arena;
    re->window = window;

    if (!core_init(&re->core)) {
        LOG_FATAL("core render not initialized");
        return NULL;
    }
    if (!swapchain_init(&re->swap, &re->core, window, VK_NULL_HANDLE)) {
        LOG_FATAL("swapchain render not initialized");
        return NULL;
    }

    LOG_INFO("render system initialized");
    return re;
}

void render_system_kill(render_system_t *re) {
    if (re) {
        swapchain_kill(&re->swap, &re->core);
        core_kill(&re->core);

        memset(re, 0, sizeof(render_system_t));
    }
    LOG_INFO("render system kill");
}
