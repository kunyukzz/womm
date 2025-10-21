#include "game.h"
#include "core/input.h"
#include "core/memory.h"
#include "core/math/maths.h"

game_system_t *game_init(void) {
    game_system_t *game = WALLOC(sizeof(game_system_t), MEM_GAME);

    game->cam = get_camera_system();
    game->cam->main_cam.position = (vec3){.comp1.x = 0.0f, 0.0f, 20.0f, 0.0f};
    game->cam->main_cam.rotation = vec3_zero();

    LOG_INFO("Welcome to WOMM!!");
    return game;
}

bool game_update(game_system_t *game, float delta) {
    float move_speed = 5.0f;
    if (key_press(INPUT_KEY_LEFT)) {
        cam_yaw(game->cam, move_speed * delta);
    }
    if (key_press(INPUT_KEY_RIGHT)) {
        cam_yaw(game->cam, -move_speed * delta);
    }
    if (key_press(INPUT_KEY_UP)) {
        cam_pitch(game->cam, move_speed * delta);
    }
    if (key_press(INPUT_KEY_DOWN)) {
        cam_pitch(game->cam, -move_speed * delta);
    }

    float temp = 5.0f;
    vec3 velo = vec3_zero();
    if (key_press(INPUT_KEY_W)) {
        vec3 forward = mat4_forward(game->cam->main_cam.world_view);
        velo = vec3_add(velo, forward);
    }
    if (key_press(INPUT_KEY_S)) {
        vec3 backward = mat4_backward(game->cam->main_cam.world_view);
        velo = vec3_add(velo, backward);
    }
    if (key_press(INPUT_KEY_A)) {
        vec3 left = mat4_left(game->cam->main_cam.world_view);
        velo = vec3_add(velo, left);
    }
    if (key_press(INPUT_KEY_D)) {
        vec3 right = mat4_right(game->cam->main_cam.world_view);
        velo = vec3_add(velo, right);
    }
    if (key_press(INPUT_KEY_Q)) {
        velo.comp1.y += 1.0f;
    }
    if (key_press(INPUT_KEY_E)) {
        velo.comp1.y -= 1.0f;
    }

    vec3 z = vec3_zero();
    if (!vec3_compared(z, velo, 0.0002f)) {
        vec3_normalized(&velo);
        game->cam->main_cam.position.comp1.x += velo.comp1.x * temp * delta;
        game->cam->main_cam.position.comp1.y += velo.comp1.y * temp * delta;
        game->cam->main_cam.position.comp1.z += velo.comp1.z * temp * delta;

        game->cam->main_cam.dirty = true;
    }
    camera_update(game->cam);
    return true;
}

bool game_render(game_system_t *game, float delta) {
    (void)game;
    (void)delta;
    return true;
}

void game_kill(game_system_t *game) {
    if (game) {
        WFREE(game, 0, sizeof(game_system_t));
    }
    LOG_INFO("Goodbye WOMM!!");
}
