#ifndef INPUT_H
#define INPUT_H

#include "define.h"
#include "arena.h"
#include "event.h"

typedef enum {
    INPUT_MB_LEFT,
    INPUT_MB_RIGHT,
    INPUT_MB_MIDDLE,
    INPUT_MB_WHEEL_UP,
    INPUT_MB_WHEEL_DOWN,
    INPUT_MB_MAX
} input_button_t;

typedef enum {
    INPUT_KEY_UNKNOWN = 0x00,

    // Alphanumeric
    INPUT_KEY_A,
    INPUT_KEY_B,
    INPUT_KEY_C,
    INPUT_KEY_D,
    INPUT_KEY_E,
    INPUT_KEY_F,
    INPUT_KEY_G,
    INPUT_KEY_H,
    INPUT_KEY_I,
    INPUT_KEY_J,
    INPUT_KEY_K,
    INPUT_KEY_L,
    INPUT_KEY_M,
    INPUT_KEY_N,
    INPUT_KEY_O,
    INPUT_KEY_P,
    INPUT_KEY_Q,
    INPUT_KEY_R,
    INPUT_KEY_S,
    INPUT_KEY_T,
    INPUT_KEY_U,
    INPUT_KEY_V,
    INPUT_KEY_W,
    INPUT_KEY_X,
    INPUT_KEY_Y,
    INPUT_KEY_Z,

    INPUT_KEY_1,
    INPUT_KEY_2,
    INPUT_KEY_3,
    INPUT_KEY_4,
    INPUT_KEY_5,
    INPUT_KEY_6,
    INPUT_KEY_7,
    INPUT_KEY_8,
    INPUT_KEY_9,
    INPUT_KEY_0,

    // Function Keys
    INPUT_KEY_F1,
    INPUT_KEY_F2,
    INPUT_KEY_F3,
    INPUT_KEY_F4,
    INPUT_KEY_F5,
    INPUT_KEY_F6,
    INPUT_KEY_F7,
    INPUT_KEY_F8,
    INPUT_KEY_F9,
    INPUT_KEY_F10,
    INPUT_KEY_F11,
    INPUT_KEY_F12,

    INPUT_KEY_ESCAPE,
    INPUT_KEY_ENTER,
    INPUT_KEY_TAB,
    INPUT_KEY_BACKSPACE,
    INPUT_KEY_INSERT,
    INPUT_KEY_DELETE,
    INPUT_KEY_HOME,
    INPUT_KEY_END,
    INPUT_KEY_PAGEUP,
    INPUT_KEY_PAGEDOWN,

    INPUT_KEY_LEFT,
    INPUT_KEY_RIGHT,
    INPUT_KEY_UP,
    INPUT_KEY_DOWN,

    // Modifier key
    INPUT_KEY_LSHIFT,
    INPUT_KEY_RSHIFT,
    INPUT_KEY_LCONTROL,
    INPUT_KEY_RCONTROL,
    INPUT_KEY_LALT,
    INPUT_KEY_RALT,
    INPUT_KEY_LSUPER,
    INPUT_KEY_RSUPER,
    INPUT_KEY_CAPS,
    INPUT_KEY_NUM,

    // Numpad
    INPUT_KEY_KP_0,
    INPUT_KEY_KP_1,
    INPUT_KEY_KP_2,
    INPUT_KEY_KP_3,
    INPUT_KEY_KP_4,
    INPUT_KEY_KP_5,
    INPUT_KEY_KP_6,
    INPUT_KEY_KP_7,
    INPUT_KEY_KP_8,
    INPUT_KEY_KP_9,
    INPUT_KEY_KP_DECIMAL,
    INPUT_KEY_KP_ENTER,
    INPUT_KEY_KP_ADD,
    INPUT_KEY_KP_SUBSTRACT,
    INPUT_KEY_KP_MULTIPLY,
    INPUT_KEY_KP_DIVIDE,

    // Symbols
    INPUT_KEY_SPACE,
    INPUT_KEY_MINUS,
    INPUT_KEY_EQUAL,
    INPUT_KEY_LBRACKET,
    INPUT_KEY_RBRACKET,
    INPUT_KEY_LBRACE,
    INPUT_KEY_RBRACE,
    INPUT_KEY_BACKSLASH,
    INPUT_KEY_SEMICOLON,
    INPUT_KEY_APOSTROPHE,
    INPUT_KEY_GRAVE,
    INPUT_KEY_COMMA,
    INPUT_KEY_PERIOD,
    INPUT_KEY_SLASH,

    // Misc
    INPUT_KEY_PRINT_SCREEN,
    INPUT_KEY_SCROLL_LOCK,
    INPUT_KEY_PAUSE,

    INPUT_KEY_MAX
} input_keys_t;

typedef struct input_system_t input_system_t;

// initializer state
input_system_t *input_system_init(arena_alloc_t *arena);
void input_system_update(input_system_t *input, float delta,
                         arena_alloc_t *frame_arena);
void input_system_kill(input_system_t *input);

// runtime
void input_process_key(input_system_t *input, event_system_t *event,
                       input_keys_t key, bool is_press);
void input_process_button(input_system_t *input, event_system_t *event,
                          input_button_t button, bool is_press);
void input_process_mouse_move(input_system_t *input, event_system_t *event,
                              int16_t pos_x, int16_t pos_y);
void input_process_mouse_wheel(input_system_t *input, event_system_t *event,
                               int8_t delta_z);

// keys
bool key_press(const input_system_t *input, input_keys_t key);
bool key_release(const input_system_t *input, input_keys_t key);
bool key_was_pressed(const input_system_t *input, input_keys_t key);
bool key_was_released(const input_system_t *input, input_keys_t key);

// mouse
bool button_press(const input_system_t *input, input_button_t button);
bool button_release(const input_system_t *input, input_button_t button);
bool button_was_pressed(const input_system_t *input, input_button_t button);
bool button_was_released(const input_system_t *input, input_button_t button);
void get_mouse_pos(const input_system_t *input, int32_t *pos_x, int32_t *pos_y);
void get_mouse_prev_pos(const input_system_t *input, int32_t *pos_x,
                        int32_t *pos_y);

// debug
const char *keycode_to_str(input_keys_t key);
input_keys_t keycode_translate(uint32_t keycode);

#endif // INPUT_H
