#include "camera.h"
#include "math/maths.h"

#include <string.h>

static camera_system_t *g_cam = NULL;

camera_system_t *camera_system_init(arena_alloc_t *arena, window_t *window) {
    if (g_cam != NULL) return g_cam;
    camera_system_t *cam = arena_alloc(arena, sizeof(camera_system_t));
    if (!cam) return NULL;
    memset(cam, 0, sizeof(camera_system_t));

    cam->arena = arena;
    cam->window = window;
    cam->main_cam.near = 0.1f;
    cam->main_cam.far = 1000.0f;
    cam->main_cam.fov = 45.0f;
    cam->main_cam.dirty = true;

    cam->main_cam.world_proj =
        mat4_column_perspective(deg_to_rad(cam->main_cam.fov),
                                (float)window->width / (float)window->height,
                                cam->main_cam.near, cam->main_cam.far);

    cam->main_cam.world_view = mat4_identity();
    cam->main_cam.world_view = mat4_inverse_rigid(cam->main_cam.world_view);

    LOG_INFO("camera system initialize");
    g_cam = cam;
    return cam;
}

void camera_system_kill(camera_system_t *cam) {
    if (cam) memset(cam, 0, sizeof(camera_system_t));
    LOG_INFO("camera system kill");
}

void camera_update(camera_system_t *cam) {
    if (cam->main_cam.dirty) {
        mat4 rotation = mat4_euler_xyz(cam->main_cam.rotation.comp1.x,
                                       cam->main_cam.rotation.comp1.y,
                                       cam->main_cam.rotation.comp1.z);
        mat4 translate = mat4_translate(cam->main_cam.position);

        mat4 world_view = mat4_column_multi(rotation, translate);
        cam->main_cam.world_view = mat4_inverse_rigid(world_view);

        if (mat4_has_nan(&cam->main_cam.world_proj)) {
            LOG_ERROR("NaN in projection matrix!");
        }
        if (mat4_has_nan(&cam->main_cam.world_view)) {
            LOG_ERROR("NaN in view matrix!");
        }
        cam->main_cam.dirty = false;
    }
}

void camera_view(mat4 view) { g_cam->main_cam.world_view = view; }

void cam_yaw(camera_system_t *cam, float amount) {
    cam->main_cam.rotation.comp1.y += amount;
    cam->main_cam.dirty = true;
}

void cam_pitch(camera_system_t *cam, float amount) {
    cam->main_cam.rotation.comp1.x += amount;
    float limit = deg_to_rad(89.0f);
    cam->main_cam.rotation.comp1.x =
        CLAMP(cam->main_cam.rotation.comp1.x, -limit, limit);
    cam->main_cam.dirty = true;
}

camera_system_t *get_camera_system(void) { return g_cam; }

camera_t *get_main_camera(void) {
    if (!g_cam) return NULL;
    return &g_cam->main_cam;
}
