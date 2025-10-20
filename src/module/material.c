#include "material.h"

#include <string.h>

bool default_material_init(material_system_t *mat);

material_system_t *material_system_init(arena_alloc_t *arena) {
    material_system_t *mat = arena_alloc(arena, sizeof(material_system_t));
    if (!mat) return NULL;
    memset(mat, 0, sizeof(material_system_t));

    mat->arena = arena;
    default_material_init(mat);

    LOG_INFO("material system init");
    return mat;
}

void material_system_kill(material_system_t *mat) {
    if (mat) memset(mat, 0, sizeof(material_system_t));
    LOG_INFO("material system kill");
}

bool default_material_init(material_system_t *mat) {
    // strcpy(mat->default_mat.name, "default");
    mat->default_mat.diffuse_color = (vec4){{0.5f, 0.5f, 0.5f, 1.0f}};
    mat->default_mat.has_texture = false;
    return true;
}
