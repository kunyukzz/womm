#ifndef MATERIAL_H
#define MATERIAL_H

#include "core/define.h" // IWYU pragma: keep
#include "core/arena.h"
#include "renderer/frontend_type.h"

typedef struct {
    arena_alloc_t *arena;
    material_data_t default_mat;
} material_system_t;

material_system_t *material_system_init(arena_alloc_t *arena);
void material_system_kill(material_system_t *mat);

#endif // MATERIAL_H
