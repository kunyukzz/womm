#include "texture.h"
#include "renderer/frontend.h"

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

texture_system_t *texture_system_init(arena_alloc_t *arena) {
    texture_system_t *tex = arena_alloc(arena, sizeof(texture_system_t));
    if (!tex) return NULL;
    memset(tex, 0, sizeof(texture_system_t));

    tex->arena = arena;

    if (!default_tex_init(&tex->default_texture)) {
        LOG_ERROR("failed to create default texture");
        return NULL;
    }

    LOG_INFO("texture system initialize");
    return tex;
}

void texture_system_kill(texture_system_t *tex) {
    if (tex) {
        default_tex_kill(&tex->default_texture);
        memset(tex, 0, sizeof(texture_system_t));
    }
    LOG_INFO("texture system kill");
}
