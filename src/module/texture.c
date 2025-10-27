#include "texture.h"
#include "renderer/frontend.h"
#include "core/binary_loader.h"

#include <string.h>

static bool default_tex_init(texture_data_t *tex) {
    const uint32_t dimension = 256;
    const uint32_t channel = 4;
    const uint32_t pixel_count = dimension * dimension;

    uint8_t pixels[pixel_count * channel];
    memset(pixels, 255, sizeof(uint8_t) * pixel_count * channel);

    for (uint32_t y = 0; y < dimension; ++y) {
        for (uint32_t x = 0; x < dimension; ++x) {
            uint32_t index = (y * dimension + x) * channel;
            bool is_white = ((x / 32) + (y / 32)) % 2 == 0;

            if (is_white) {
                pixels[index] = 255;
                pixels[index + 1] = 255;
                pixels[index + 2] = 255;
                pixels[index + 3] = 255;
            } else {
                pixels[index] = 255;
                pixels[index + 1] = 0;
                pixels[index + 2] = 255;
                pixels[index + 3] = 255;
            }
        }
    }

    strcpy(tex->name, "default_checker");
    tex->width = dimension;
    tex->height = dimension;
    tex->channels = channel;

    render_tex_init(pixels, tex);
    return true;
}

static void default_tex_kill(texture_data_t *tex) {
    render_tex_kill(tex);

    memset(tex->name, 0, sizeof(char) * 64);
    memset(tex, 0, sizeof(texture_data_t));
}

static bool load_from_file(texture_system_t *tex, const char *name,
                           texture_data_t *out_tex) {
    (void)tex;
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s.png", name);

    int32_t width, height, channels;

    void *pixel = read_image_file(path, &width, &height, &channels);
    strcpy(out_tex->name, name);
    out_tex->width = (uint32_t)width;
    out_tex->height = (uint32_t)height;
    out_tex->channels = (uint32_t)channels;

    /*
    printf("[Texture] create: %s (%d/%d/%d) - expected size: %lu bytes\n", path,
           width, height, channels, (uint64_t)width * (uint64_t)height * 4);
           */

    if (!pixel) {
        return false;
    }

    bool success = render_tex_init(pixel, out_tex);
    stbi_image_free(pixel);

    return success;
}

static bool load_womm_tex(texture_system_t *tex) {
    bool loaded = true;
    if (!load_from_file(tex, "textures/rocks", &tex->gear_base)) {
        tex->gear_base = tex->default_texture;
        LOG_WARN("texture not found. fallback!");
        loaded = false;
    }

    if (!load_from_file(tex, "textures/test", &tex->vulkan_logo)) {
        tex->vulkan_logo = tex->default_texture;
        LOG_WARN("texture not found. fallback!");
        loaded = false;
    }

    if (!load_from_file(tex, "textures/memes", &tex->memes)) {
        tex->memes = tex->default_texture;
        LOG_WARN("texture not found. fallback!");
        loaded = false;
    }
    return loaded;
}

static void unload_womm_tex(texture_system_t *tex) {
    render_tex_kill(&tex->gear_base);
    render_tex_kill(&tex->vulkan_logo);
    render_tex_kill(&tex->memes);
}

texture_system_t *texture_system_init(arena_alloc_t *arena) {
    texture_system_t *tex = arena_alloc(arena, sizeof(texture_system_t));
    if (!tex) return NULL;
    memset(tex, 0, sizeof(texture_system_t));

    tex->arena = arena;

    if (!default_tex_init(&tex->default_texture)) {
        LOG_ERROR("failed to create default texture");
        return NULL;
    }

    load_womm_tex(tex);

    LOG_INFO("texture system initialize");
    return tex;
}

void texture_system_kill(texture_system_t *tex) {
    if (tex) {
        unload_womm_tex(tex);
        default_tex_kill(&tex->default_texture);
        memset(tex, 0, sizeof(texture_system_t));
    }
    LOG_INFO("texture system kill");
}
