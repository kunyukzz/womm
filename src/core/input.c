#include "input.h"
#include "event.h"

#include <string.h>

#define MAX_KEYS 256
#define MAX_BUTTONS 8

typedef struct {
    bool keys[MAX_KEYS];
} keyboard_state;

typedef struct {
    int16_t pos_x;
    int16_t pos_y;
    int8_t wheel_delta;
    uint8_t buttons[MAX_BUTTONS];
} mouse_state;

struct input_system_t {
    arena_alloc_t *arena;
    keyboard_state kbd_curr;
    keyboard_state kbd_prev;
    mouse_state mouse_curr;
    mouse_state mouse_prev;
};

static input_system_t *g_ip = NULL;

input_system_t *input_system_init(arena_alloc_t *arena) {
    input_system_t *input = arena_alloc(arena, sizeof(input_system_t));
    if (!input) return NULL;

    memset(input, 0, sizeof(input_system_t));
    input->arena = arena;

    g_ip = input;
    LOG_INFO("input system initialized");
    return input;
}

void input_system_update(float delta) {
    (void)delta;

    for (uint32_t i = 0; i < MAX_KEYS; ++i) {
        g_ip->kbd_prev.keys[i] = g_ip->kbd_curr.keys[i];
    }
    for (uint32_t i = 0; i < MAX_BUTTONS; ++i) {
        g_ip->mouse_prev.buttons[i] = g_ip->mouse_curr.buttons[i];
    }

    g_ip->mouse_prev.pos_x = g_ip->mouse_curr.pos_x;
    g_ip->mouse_prev.pos_y = g_ip->mouse_curr.pos_y;
    g_ip->mouse_curr.wheel_delta = 0;
}

void input_system_kill(input_system_t *input) {
    if (input) memset(input, 0, sizeof(input_system_t));
    LOG_INFO("input system kill");
}

void input_process_key(input_keys_t key, bool is_press) {
    if (g_ip->kbd_curr.keys[key] != is_press) {
        g_ip->kbd_curr.keys[key] = is_press;

        event_t ev;
        ev.data.keys.keycode = (uint16_t)key;
        event_push(is_press ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE, &ev, NULL);
    }
}

void input_process_button(input_button_t button, bool is_press) {
    if (g_ip->mouse_curr.buttons[button] != is_press) {
        g_ip->mouse_curr.buttons[button] = is_press;

        event_t ev;
        ev.data.mouse_button.button = (uint8_t)button;
        event_push(is_press ? EVENT_MOUSE_PRESS : EVENT_MOUSE_RELEASE, &ev,
                   NULL);
    }
}

void input_process_mouse_move(int16_t pos_x, int16_t pos_y) {
    if (g_ip->mouse_curr.pos_x != pos_x || g_ip->mouse_curr.pos_y != pos_y) {
        g_ip->mouse_curr.pos_x = pos_x;
        g_ip->mouse_curr.pos_y = pos_y;

        event_t ev;
        ev.data.mouse_move.x = pos_x;
        ev.data.mouse_move.y = pos_y;
        event_push(EVENT_MOUSE_MOVE, &ev, NULL);
    }
}

void input_process_mouse_wheel(int8_t delta_z) {
    g_ip->mouse_curr.wheel_delta = delta_z;

    if (delta_z) {
        event_t ev;
        ev.data.mouse_button.wheel_delta = delta_z;
        event_push(EVENT_MOUSE_WHEEL, &ev, NULL);
    }
}

bool key_press(input_keys_t key) { return g_ip->kbd_curr.keys[key] == true; }

bool key_release(input_keys_t key) { return g_ip->kbd_curr.keys[key] == false; }

bool key_was_pressed(input_keys_t key) {
    return g_ip->kbd_prev.keys[key] == true;
}

bool key_was_released(input_keys_t key) {
    return g_ip->kbd_prev.keys[key] == false;
}

// mouse
bool button_press(input_button_t button) {
    return g_ip->mouse_curr.buttons[button] == true;
}

bool button_release(input_button_t button) {
    return g_ip->mouse_curr.buttons[button] == false;
}

bool button_was_pressed(input_button_t button) {
    return g_ip->mouse_prev.buttons[button] == true;
}

bool button_was_released(input_button_t button) {
    return g_ip->mouse_prev.buttons[button] == false;
}

void get_mouse_pos(int32_t *pos_x, int32_t *pos_y) {
    *pos_x = g_ip->mouse_curr.pos_x;
    *pos_y = g_ip->mouse_curr.pos_y;
}

void get_mouse_prev_pos(int32_t *pos_x, int32_t *pos_y) {
    *pos_x = g_ip->mouse_prev.pos_x;
    *pos_y = g_ip->mouse_prev.pos_y;
}
