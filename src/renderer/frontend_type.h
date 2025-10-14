#ifndef FRONTEND_TYPE_H
#define FRONTEND_TYPE_H

#include "core/define.h" // IWYU pragma: keep
#include "core/math/math_type.h"

typedef struct {
    uint32_t vertex_size;
    uint32_t vertex_count;
    uint32_t vertex_offset;
    uint32_t index_size;
    uint32_t index_count;
    uint32_t index_offset;
} geo_gpu_t;

typedef struct {
    uint32_t vertex_size;
    uint32_t vertex_count;
    void *vertices;

    uint32_t index_size;
    uint32_t index_count;
    void *indices;
} geo_cpu_t;

typedef struct {
    geo_gpu_t *geo;
    mat4 model;
    float delta;
} render_bundle_t;

#endif // FRONTEND_TYPE_H
