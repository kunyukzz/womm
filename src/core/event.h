#ifndef EVENT_H
#define EVENT_H

#include "define.h"
#include "arena.h"

typedef enum {
    EVENT_NONE = 0x00,
    EVENT_QUIT = 0x01,
    EVENT_SUSPEND = 0x02,
    EVENT_RESUME = 0x03,
    EVENT_RESIZE = 0x04,

    EVENT_KEY_PRESS = 0x05,
    EVENT_KEY_RELEASE = 0x06,
    EVENT_MOUSE_PRESS = 0x07,
    EVENT_MOUSE_RELEASE = 0x08,
    EVENT_MOUSE_MOVE = 0x09,
    EVENT_MOUSE_WHEEL = 0x0A,

    EVENT_MAX = 0xFF
} event_type_t;

typedef struct {
    uint32_t type;
    uint32_t ticks;

    union {
        struct {
            uint16_t keycode;
            uint8_t is_repeat;
            uint8_t _pad0;
            uint32_t modifiers; // padding 4 byte
        } keys;

        struct {
            int16_t dx, dy;
            int32_t x, y;
        } mouse_move;

        struct {
            uint8_t button;
            uint8_t press;
            int16_t wheel_delta;
            int32_t x, y;
        } mouse_button;

        struct {
            uint8_t id;
            uint8_t button;
            uint16_t dpad;
            int16_t lx, ly;
            int16_t rx, ry;
            uint8_t lt, rt;
            uint8_t _pad[6];
        } gamepad;

        struct {
            uint32_t width;
            uint32_t height;
            bool is_resizeable;
        } resize;

        uint8_t raw[24];
    } data;
} event_t;

typedef struct event_system_t event_system_t;
typedef bool (*on_event)(event_system_t *event, uint32_t type, event_t *ev,
                         void *sender, void *recipient);

// initializer state
event_system_t *event_system_init(arena_alloc_t *arena);
void event_system_kill(event_system_t *event);

// runtime
bool event_reg(event_system_t *event, uint32_t type, on_event handler,
               void *recipient);
bool event_unreg(event_system_t *event, uint32_t type, on_event handler,
                 void *recipient);
bool event_push(event_system_t *event, uint32_t type, const event_t *ev,
                void *sender);

#endif // EVENT_H
