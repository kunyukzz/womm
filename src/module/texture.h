#ifndef TEXTURE_H
#define TEXTURE_H

#include "core/define.h" // IWYU pragma: keep
#include "core/arena.h"
#include "renderer/frontend_type.h"

typedef struct {
    arena_alloc_t *arena;
    texture_data_t *textures;
    uint32_t texture_count;
    texture_data_t default_texture;
} texture_system_t;

texture_system_t *texture_system_init(arena_alloc_t *arena);
void texture_system_kill(texture_system_t *tex);

texture_data_t *texture_load(texture_system_t *tex, const char *filename);

#endif // TEXTURE_H
