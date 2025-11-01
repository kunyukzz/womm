#ifndef CAMERA_H
#define CAMERA_H

#include "define.h" // IWYU pragma: keep
#include "arena.h"
#include "math/math_type.h"
#include "platform/window.h"

typedef struct {
    mat4 world_proj;
    mat4 world_view;
    mat4 ui_proj;
    mat4 ui_view;

    float near, far, fov;
    vec3 position;
    vec3 rotation;

    bool dirty;
} camera_t;

typedef struct {
    arena_alloc_t *arena;
    camera_t main_cam;
    window_t *window;
} camera_system_t;

camera_system_t *camera_system_init(arena_alloc_t *arena, window_t *window);
void camera_system_kill(camera_system_t *cam);

/*** camera control ***/
void camera_update(camera_system_t *cam);
void cam_yaw(camera_system_t *cam, float amount);
void cam_pitch(camera_system_t *cam, float amount);

camera_system_t *get_camera_system(void);
camera_t *get_main_camera(void);

#endif // CAMERA_H
