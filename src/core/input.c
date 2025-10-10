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
    uint8_t wheel_delta;
    uint8_t buttons[MAX_BUTTONS];
} mouse_state;

struct input_system_t {
    arena_alloc_t *arena;
    keyboard_state kbd_curr;
    keyboard_state kbd_prev;
    mouse_state mouse_curr;
    mouse_state mouse_prev;
};

input_system_t *input_system_init(arena_alloc_t *arena) {
    input_system_t *input = arena_alloc(arena, sizeof(input_system_t));
    if (!input) return NULL;

    memset(input, 0, sizeof(input_system_t));
    input->arena = arena;

    LOG_INFO("input system initialized");
    return input;
}

void input_system_update(input_system_t *input, float delta,
                         arena_alloc_t *frame_arena) {
    (void)delta;
    (void)frame_arena;

    for (uint32_t i = 0; i < MAX_KEYS; ++i) {
        input->kbd_prev.keys[i] = input->kbd_curr.keys[i];
    }
    for (uint32_t i = 0; i < MAX_BUTTONS; ++i) {
        input->mouse_prev.buttons[i] = input->mouse_curr.buttons[i];
    }

    input->mouse_prev.pos_x = input->mouse_curr.pos_x;
    input->mouse_prev.pos_y = input->mouse_curr.pos_y;
    input->mouse_curr.wheel_delta = 0;
}

void input_system_kill(input_system_t *input) {
    if (input) memset(input, 0, sizeof(input_system_t));
    LOG_INFO("input system kill");
}

void input_process_key(input_system_t *input, event_system_t *event,
                       input_keys_t key, bool is_press) {
    if (input->kbd_curr.keys[key] != is_press) {
        input->kbd_curr.keys[key] = is_press;

        event_t ev;
        ev.data.keys.keycode = (uint16_t)key;
        event_push(event, is_press ? EVENT_KEY_PRESS : EVENT_KEY_RELEASE, &ev,
                   NULL);
    }
}

void input_process_button(input_system_t *input, event_system_t *event,
                          input_button_t button, bool is_press) {
    if (input->mouse_curr.buttons[button] != is_press) {
        input->mouse_curr.buttons[button] = is_press;

        event_t ev;
        ev.data.mouse_button.button = (uint8_t)button;
        event_push(event, is_press ? EVENT_MOUSE_PRESS : EVENT_MOUSE_RELEASE,
                   &ev, NULL);
    }
}

void input_process_mouse_move(input_system_t *input, event_system_t *event,
                              int16_t pos_x, int16_t pos_y) {
    if (input->mouse_curr.pos_x != pos_x || input->mouse_curr.pos_y != pos_y) {
        input->mouse_curr.pos_x = pos_x;
        input->mouse_curr.pos_y = pos_y;

        event_t ev;
        ev.data.mouse_move.x = pos_x;
        ev.data.mouse_move.y = pos_y;
        event_push(event, EVENT_MOUSE_MOVE, &ev, NULL);
    }
}

void input_process_mouse_wheel(input_system_t *input, event_system_t *event,
                               int8_t delta_z) {
    input->mouse_curr.wheel_delta = (uint8_t)delta_z;

    if (delta_z) {
        event_t ev;
        ev.data.mouse_button.wheel_delta = delta_z;
        event_push(event, EVENT_MOUSE_WHEEL, &ev, NULL);
    }
}

bool key_press(const input_system_t *input, input_keys_t key) {
    return input->kbd_curr.keys[key] == true;
}

bool key_release(const input_system_t *input, input_keys_t key) {
    return input->kbd_curr.keys[key] == false;
}

bool key_was_pressed(const input_system_t *input, input_keys_t key) {
    return input->kbd_prev.keys[key] == true;
}

bool key_was_released(const input_system_t *input, input_keys_t key) {
    return input->kbd_prev.keys[key] == false;
}

// mouse
bool button_press(const input_system_t *input, input_button_t button) {
    return input->mouse_curr.buttons[button] == true;
}

bool button_release(const input_system_t *input, input_button_t button) {
    return input->mouse_curr.buttons[button] == false;
}

bool button_was_pressed(const input_system_t *input, input_button_t button) {
    return input->mouse_prev.buttons[button] == true;
}

bool button_was_released(const input_system_t *input, input_button_t button) {
    return input->mouse_prev.buttons[button] == false;
}

void get_mouse_pos(const input_system_t *input, int32_t *pos_x,
                   int32_t *pos_y) {
    *pos_x = input->mouse_curr.pos_x;
    *pos_y = input->mouse_curr.pos_y;
}

void get_mouse_prev_pos(const input_system_t *input, int32_t *pos_x,
                        int32_t *pos_y) {
    *pos_x = input->mouse_prev.pos_x;
    *pos_y = input->mouse_prev.pos_y;
}
