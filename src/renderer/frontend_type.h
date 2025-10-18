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
    char name[64];
    vec4 color;
    char texture_name[64];
    bool has_texture;
} material_data_t;

#define MAX_GEO 10

typedef struct {
    geo_gpu_t *geo;
    mat4 model;
    vec4 diffuse_color;
} object_bundle_t;

typedef struct {
    object_bundle_t obj[MAX_GEO];
    float delta;
} render_bundle_t;

#endif // FRONTEND_TYPE_H
