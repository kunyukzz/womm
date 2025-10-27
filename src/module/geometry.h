#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "core/define.h" // IWYU pragma: keep
#include "core/arena.h"
#include "renderer/frontend_type.h"

typedef struct {
    arena_alloc_t *arena;
    geo_gpu_t default_geo;
    geo_gpu_t plane;
} geometry_system_t;

geometry_system_t *geo_system_init(arena_alloc_t *arena);
void geo_system_kill(geometry_system_t *geo);

geo_cpu_t geo_create_plane(float width, float height, uint32_t seg_x,
                           uint32_t seg_y);

#endif // GEOMETRY_H
