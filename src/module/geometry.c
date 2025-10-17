#include "geometry.h"
#include "core/memory.h"
#include "renderer/frontend.h"

#include <string.h>

static geometry_system_t *g_geo = NULL;

bool default_geo_init(geometry_system_t *geo);

geometry_system_t *geo_system_init(arena_alloc_t *arena) {
    if (g_geo != NULL) return g_geo;

    geometry_system_t *geo = arena_alloc(arena, sizeof(geometry_system_t));
    if (!geo) return NULL;

    memset(geo, 0, sizeof(geometry_system_t));
    geo->arena = arena;

    default_geo_init(geo);
    LOG_INFO("geometry system initialized");
    return geo;
}

void geo_system_kill(geometry_system_t *geo) {
    if (geo) memset(geo, 0, sizeof(geometry_system_t));
    LOG_INFO("geometry system kill");
}

geo_cpu_t geo_create_plane(float width, float height, uint32_t seg_x,
                           uint32_t seg_y) {
    if (width == 0) {
        LOG_WARN("width must be non-ZERO. set to 1");
        width = 1.0f;
    }
    if (height == 0) {
        LOG_WARN("height must be non-ZERO. set to 1");
        height = 1.0f;
    }
    if (seg_x < 1) {
        LOG_WARN("segment_x must be positive number. set to 1");
        seg_x = 1;
    }
    if (seg_y < 1) {
        LOG_WARN("segment_y must be positive number. set to 1");
        seg_y = 1;
    }

    geo_cpu_t geo;
    geo.vertex_size = sizeof(vertex_3d);
    geo.vertex_count = seg_x * seg_y * 4;
    geo.index_size = sizeof(uint32_t);
    geo.index_count = seg_x * seg_y * 6;
    geo.vertices = WALLOC(sizeof(vertex_3d) * geo.vertex_count, MEM_ARRAY);
    geo.indices = WALLOC(sizeof(uint32_t) * geo.index_count, MEM_ARRAY);

    float sw = width / (float)seg_x;
    float sh = height / (float)seg_y;
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float tile_x = 1.0f;
    float tile_y = 1.0f;

    for (uint32_t y = 0; y < seg_y; ++y) {
        for (uint32_t x = 0; x < seg_x; ++x) {
            float min_x = ((float)x * sw) - hw;
            float min_y = ((float)y * sh) - hh;
            float max_x = min_x + sw;
            float max_y = min_y + sh;
            float min_uv_x = ((float)x / (float)seg_x) * tile_x;
            float min_uv_y = ((float)y / (float)seg_y) * tile_y;
            float max_uv_x = ((float)(x + 1) / (float)seg_x) * tile_x;
            float max_uv_y = ((float)(y + 1) / (float)seg_y) * tile_y;

            uint32_t v_offset = ((y * seg_x) + x) * 4;
            vertex_3d *v0 = &((vertex_3d *)geo.vertices)[v_offset + 0];
            vertex_3d *v1 = &((vertex_3d *)geo.vertices)[v_offset + 1];
            vertex_3d *v2 = &((vertex_3d *)geo.vertices)[v_offset + 2];
            vertex_3d *v3 = &((vertex_3d *)geo.vertices)[v_offset + 3];

            /*** top left ***/
            v0->position.comp1.x = min_x;
            v0->position.comp1.y = max_y;
            v0->texcoord.comp1.x = min_uv_x;
            v0->texcoord.comp1.y = min_uv_y;

            /*** top right ***/
            v1->position.comp1.x = max_x;
            v1->position.comp1.y = max_y;
            v1->texcoord.comp1.x = max_uv_x;
            v1->texcoord.comp1.y = min_uv_y;

            /*** bottom right ***/
            v2->position.comp1.x = max_x;
            v2->position.comp1.y = min_y;
            v2->texcoord.comp1.x = max_uv_x;
            v2->texcoord.comp1.y = max_uv_y;

            /*** bottom left ***/
            v3->position.comp1.x = min_x;
            v3->position.comp1.y = min_y;
            v3->texcoord.comp1.x = min_uv_x;
            v3->texcoord.comp1.y = max_uv_y;

            uint32_t i_offset = ((y * seg_x) + x) * 6;
            ((uint32_t *)geo.indices)[i_offset + 0] = v_offset + 0;
            ((uint32_t *)geo.indices)[i_offset + 1] = v_offset + 1;
            ((uint32_t *)geo.indices)[i_offset + 2] = v_offset + 2;
            ((uint32_t *)geo.indices)[i_offset + 3] = v_offset + 0;
            ((uint32_t *)geo.indices)[i_offset + 4] = v_offset + 2;
            ((uint32_t *)geo.indices)[i_offset + 5] = v_offset + 3;
        }
    }

    return geo;
}

/*
geo_cpu_t geo_create_cube(float width, float height, float depth,
                          uint32_t segment) {
    if (width == 0) {
        LOG_WARN("width must be non-ZERO. set to 1");
        width = 1.0f;
    }
    if (height == 0) {
        LOG_WARN("height must be non-ZERO. set to 1");
        height = 1.0f;
    }
    if (depth == 0) {
        LOG_WARN("height must be non-ZERO. set to 1");
        depth = 1.0f;
    }

    geo_cpu_t geo;
    geo.vertex_size = sizeof(vertex_3d);
    geo.vertex_count = segment * 4 * 6;
    geo.index_size = sizeof(uint32_t);
    geo.index_count = segment * 6 * 6;
    geo.vertices = WALLOC(sizeof(vertex_3d) * geo.vertex_count, MEM_ARRAY);
    geo.indices = WALLOC(sizeof(uint32_t) * geo.index_count, MEM_ARRAY);

    float sw = width / (float)segment;
    float sh = height / (float)segment;
    float sd = depth / (float)segment;
    float hw = width * 0.5f;
    float hh = height * 0.5f;
    float hd = depth * 0.5f;
    float tile_x = 1.0f;
    float tile_y = 1.0f;
    float tile_z = 1.0f;

    float min_x = -hw;
    float min_y = -hh;
    float min_z = -hd;
    float max_x = hw;
    float max_y = hh;
    float max_z = hd;

    float min_uv_x = 0.0f;
    float min_uv_y = 0.0f;
    float max_uv_x = tile_x;
    float max_uv_y = tile_y;

    uint32_t offset = 0;
    vertex_3d verts[24];

    return geo;
}

bool default_geo_init(geometry_system_t *geo) {
    vertex_3d vert_3d[4];
    memset(vert_3d, 0, sizeof(vertex_3d) * 4);
    const float f = 10.0f;

    // top left
    vert_3d[0].position.comp1.x = -0.5f * f;
    vert_3d[0].position.comp1.y = 0.5f * f;
    vert_3d[0].texcoord.comp1.x = 0;
    vert_3d[0].texcoord.comp1.y = 0;

    // top right
    vert_3d[1].position.comp1.x = 0.5f * f;
    vert_3d[1].position.comp1.y = 0.5f * f;
    vert_3d[1].texcoord.comp1.x = 1;
    vert_3d[1].texcoord.comp1.y = 0;

    // bottom right
    vert_3d[2].position.comp1.x = 0.5f * f;
    vert_3d[2].position.comp1.y = -0.5f * f;
    vert_3d[2].texcoord.comp1.x = 1;
    vert_3d[2].texcoord.comp1.y = 1;

    // bottom left
    vert_3d[3].position.comp1.x = -0.5f * f;
    vert_3d[3].position.comp1.y = -0.5f * f;
    vert_3d[3].texcoord.comp1.x = 0;
    vert_3d[3].texcoord.comp1.y = 1;

    uint32_t indices[6] = {0, 1, 2, 0, 2, 3};

    render_geo_init(&geo->default_geo, sizeof(vertex_3d), 4, vert_3d,
                    sizeof(uint32_t), 6, indices);
    return true;
}
*/

bool default_geo_init(geometry_system_t *geo) {
    vertex_3d vert_3d[24];
    memset(vert_3d, 0, sizeof(vertex_3d) * 24);
    const float f = 5.0f;

    { // FRONT FACE
        // top left
        vert_3d[0].position.comp1.x = -0.5f * f;
        vert_3d[0].position.comp1.y = 0.5f * f;
        vert_3d[0].position.comp1.z = 0.5f * f;
        vert_3d[0].texcoord.comp1.x = 0;
        vert_3d[0].texcoord.comp1.y = 0;

        // top right
        vert_3d[1].position.comp1.x = 0.5f * f;
        vert_3d[1].position.comp1.y = 0.5f * f;
        vert_3d[1].position.comp1.z = 0.5f * f;
        vert_3d[1].texcoord.comp1.x = 1;
        vert_3d[1].texcoord.comp1.y = 0;

        // bottom right
        vert_3d[2].position.comp1.x = 0.5f * f;
        vert_3d[2].position.comp1.y = -0.5f * f;
        vert_3d[2].position.comp1.z = 0.5f * f;
        vert_3d[2].texcoord.comp1.x = 1;
        vert_3d[2].texcoord.comp1.y = 1;

        // bottom left
        vert_3d[3].position.comp1.x = -0.5f * f;
        vert_3d[3].position.comp1.y = -0.5f * f;
        vert_3d[3].position.comp1.z = 0.5f * f;
        vert_3d[3].texcoord.comp1.x = 0;
        vert_3d[3].texcoord.comp1.y = 1;
    }

    { // BACK FACE
        // top left
        vert_3d[4].position.comp1.x = -0.5f * f;
        vert_3d[4].position.comp1.y = 0.5f * f;
        vert_3d[4].position.comp1.z = -0.5f * f;
        vert_3d[4].texcoord.comp1.x = 1;
        vert_3d[4].texcoord.comp1.y = 0;

        // top right
        vert_3d[5].position.comp1.x = 0.5f * f;
        vert_3d[5].position.comp1.y = 0.5f * f;
        vert_3d[5].position.comp1.z = -0.5f * f;
        vert_3d[5].texcoord.comp1.x = 0;
        vert_3d[5].texcoord.comp1.y = 0;

        // bottom right
        vert_3d[6].position.comp1.x = 0.5f * f;
        vert_3d[6].position.comp1.y = -0.5f * f;
        vert_3d[6].position.comp1.z = -0.5f * f;
        vert_3d[6].texcoord.comp1.x = 0;
        vert_3d[6].texcoord.comp1.y = 1;

        // bottom left
        vert_3d[7].position.comp1.x = -0.5f * f;
        vert_3d[7].position.comp1.y = -0.5f * f;
        vert_3d[7].position.comp1.z = -0.5f * f;
        vert_3d[7].texcoord.comp1.x = 1;
        vert_3d[7].texcoord.comp1.y = 1;
    }

    { // RIGHT FACE
        // top left
        vert_3d[8].position.comp1.x = 0.5f * f;
        vert_3d[8].position.comp1.y = 0.5f * f;
        vert_3d[8].position.comp1.z = 0.5f * f;
        vert_3d[8].texcoord.comp1.x = 0;
        vert_3d[8].texcoord.comp1.y = 0;

        // top right
        vert_3d[9].position.comp1.x = 0.5f * f;
        vert_3d[9].position.comp1.y = 0.5f * f;
        vert_3d[9].position.comp1.z = -0.5f * f;
        vert_3d[9].texcoord.comp1.x = 1;
        vert_3d[9].texcoord.comp1.y = 0;

        // bottom right
        vert_3d[10].position.comp1.x = 0.5f * f;
        vert_3d[10].position.comp1.y = -0.5f * f;
        vert_3d[10].position.comp1.z = -0.5f * f;
        vert_3d[10].texcoord.comp1.x = 1;
        vert_3d[10].texcoord.comp1.y = 1;

        // bottom left
        vert_3d[11].position.comp1.x = 0.5f * f;
        vert_3d[11].position.comp1.y = -0.5f * f;
        vert_3d[11].position.comp1.z = 0.5f * f;
        vert_3d[11].texcoord.comp1.x = 0;
        vert_3d[11].texcoord.comp1.y = 1;
    }

    { // LEFT FACE
        // top left
        vert_3d[12].position.comp1.x = -0.5f * f;
        vert_3d[12].position.comp1.y = 0.5f * f;
        vert_3d[12].position.comp1.z = -0.5f * f;
        vert_3d[12].texcoord.comp1.x = 0;
        vert_3d[12].texcoord.comp1.y = 0;

        // top right
        vert_3d[13].position.comp1.x = -0.5f * f;
        vert_3d[13].position.comp1.y = 0.5f * f;
        vert_3d[13].position.comp1.z = 0.5f * f;
        vert_3d[13].texcoord.comp1.x = 1;
        vert_3d[13].texcoord.comp1.y = 0;

        // bottom right
        vert_3d[14].position.comp1.x = -0.5f * f;
        vert_3d[14].position.comp1.y = -0.5f * f;
        vert_3d[14].position.comp1.z = 0.5f * f;
        vert_3d[14].texcoord.comp1.x = 1;
        vert_3d[14].texcoord.comp1.y = 1;

        // bottom left
        vert_3d[15].position.comp1.x = -0.5f * f;
        vert_3d[15].position.comp1.y = -0.5f * f;
        vert_3d[15].position.comp1.z = -0.5f * f;
        vert_3d[15].texcoord.comp1.x = 0;
        vert_3d[15].texcoord.comp1.y = 1;
    }

    { // TOP FACE
        // top left
        vert_3d[16].position.comp1.x = -0.5f * f;
        vert_3d[16].position.comp1.y = 0.5f * f;
        vert_3d[16].position.comp1.z = -0.5f * f;
        vert_3d[16].texcoord.comp1.x = 0;
        vert_3d[16].texcoord.comp1.y = 0;

        // top right
        vert_3d[17].position.comp1.x = 0.5f * f;
        vert_3d[17].position.comp1.y = 0.5f * f;
        vert_3d[17].position.comp1.z = -0.5f * f;
        vert_3d[17].texcoord.comp1.x = 1;
        vert_3d[17].texcoord.comp1.y = 0;

        // bottom right
        vert_3d[18].position.comp1.x = 0.5f * f;
        vert_3d[18].position.comp1.y = 0.5f * f;
        vert_3d[18].position.comp1.z = 0.5f * f;
        vert_3d[18].texcoord.comp1.x = 1;
        vert_3d[18].texcoord.comp1.y = 1;

        // bottom left
        vert_3d[19].position.comp1.x = -0.5f * f;
        vert_3d[19].position.comp1.y = 0.5f * f;
        vert_3d[19].position.comp1.z = 0.5f * f;
        vert_3d[19].texcoord.comp1.x = 0;
        vert_3d[19].texcoord.comp1.y = 1;
    }

    { // BOTTOM FACE
        // top left
        vert_3d[20].position.comp1.x = -0.5f * f;
        vert_3d[20].position.comp1.y = -0.5f * f;
        vert_3d[20].position.comp1.z = 0.5f * f;
        vert_3d[20].texcoord.comp1.x = 0;
        vert_3d[20].texcoord.comp1.y = 0;

        // top right
        vert_3d[21].position.comp1.x = 0.5f * f;
        vert_3d[21].position.comp1.y = -0.5f * f;
        vert_3d[21].position.comp1.z = 0.5f * f;
        vert_3d[21].texcoord.comp1.x = 1;
        vert_3d[21].texcoord.comp1.y = 0;

        // bottom right
        vert_3d[22].position.comp1.x = 0.5f * f;
        vert_3d[22].position.comp1.y = -0.5f * f;
        vert_3d[22].position.comp1.z = -0.5f * f;
        vert_3d[22].texcoord.comp1.x = 1;
        vert_3d[22].texcoord.comp1.y = 1;

        // bottom left
        vert_3d[23].position.comp1.x = -0.5f * f;
        vert_3d[23].position.comp1.y = -0.5f * f;
        vert_3d[23].position.comp1.z = -0.5f * f;
        vert_3d[23].texcoord.comp1.x = 0;
        vert_3d[23].texcoord.comp1.y = 1;
    }

    uint32_t indices[36] = {// Front face
                            0, 1, 2, 0, 2, 3,
                            // Back face
                            4, 6, 5, 4, 7, 6,
                            // Right face
                            8, 9, 10, 8, 10, 11,
                            // Left face
                            12, 13, 14, 12, 14, 15,
                            // Top face
                            16, 17, 18, 16, 18, 19,
                            // Bottom face
                            20, 21, 22, 20, 22, 23};

    render_geo_init(&geo->default_geo, sizeof(vertex_3d), 24, vert_3d,
                    sizeof(uint32_t), 36, indices);
    return true;
}
