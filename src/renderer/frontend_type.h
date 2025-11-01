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
    uint32_t width;
    uint32_t height;
    uint32_t channels;
    char name[64];

    void *data_internal; // this for pointing to internal vulkan
} texture_data_t;

typedef struct {
    vec4 diffuse_color;
    texture_data_t *tex;
    bool has_texture;
    uint32_t texture_index;
} material_data_t;

#define MAX_GEO 10

typedef struct {
    geo_gpu_t *geo;
    mat4 model;
    material_data_t material;
} object_bundle_t;

typedef struct {
    object_bundle_t obj[MAX_GEO];
    object_bundle_t ui_obj;
    uint32_t world_obj_count;
    uint32_t debug_ui_count;
    float delta;
} render_bundle_t;

#endif // FRONTEND_TYPE_H
