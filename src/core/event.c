#include "event.h"

#include <stdlib.h>
#include <string.h>

#define MAX_MSG_CODE (EVENT_MAX + 1)

typedef struct {
    on_event callback;
    void *recipient;
} ev_handler;

typedef struct {
    ev_handler *handlers;
    uint64_t count;
    uint64_t capacity;
} ev_entry;

struct event_system_t {
    ev_entry reg[MAX_MSG_CODE];
    arena_alloc_t *arena;
};

static event_system_t *g_ev = NULL;

event_system_t *event_system_init(arena_alloc_t *arena) {
    event_system_t *event = arena_alloc(arena, sizeof(event_system_t));
    if (!event) return NULL;

    memset(event, 0, sizeof(event_system_t));
    event->arena = arena;

    g_ev = event;
    LOG_INFO("event system initialized");
    return event;
}

void event_system_kill(event_system_t *event) {
    if (event) {
        memset(event, 0, sizeof(event_system_t));
    }
    LOG_INFO("event system kill");
}

bool event_reg(uint32_t type, on_event handler, void *recipient) {
    if (type >= MAX_MSG_CODE || !handler) return false;

    ev_entry *entry = &g_ev->reg[type];

    // Grow array if needed
    if (entry->count >= entry->capacity) {
        size_t new_capacity = entry->capacity ? entry->capacity * 2 : 4;
        ev_handler *new_handlers =
            arena_alloc(g_ev->arena, sizeof(ev_handler) * new_capacity);

        if (!new_handlers) return false;

        // Copy existing handlers
        if (entry->handlers) {
            memcpy(new_handlers, entry->handlers,
                   sizeof(ev_handler) * entry->count);
        }

        entry->handlers = new_handlers;
        entry->capacity = new_capacity;
    }

    // Add new handler
    entry->handlers[entry->count].callback = handler;
    entry->handlers[entry->count].recipient = recipient;
    entry->count++;

    return true;
}

bool event_unreg(uint32_t type, on_event handler, void *recipient) {
    if (type >= MAX_MSG_CODE) return false;

    ev_entry *entry = &g_ev->reg[type];

    for (size_t i = 0; i < entry->count; i++) {
        if (entry->handlers[i].callback == handler &&
            entry->handlers[i].recipient == recipient) {
            // Remove by swapping with last
            entry->handlers[i] = entry->handlers[entry->count - 1];
            entry->count--;
            return true;
        }
    }
    return false;
}

bool event_push(uint32_t type, const event_t *ev, void *sender) {
    if (type >= MAX_MSG_CODE) return false;

    // Process immediately (no queue) or add to your arena-based queue
    ev_entry *entry = &g_ev->reg[type];
    for (size_t i = 0; i < entry->count; i++) {
        if (!entry->handlers[i].callback(type, (event_t *)ev, sender,
                                         entry->handlers[i].recipient)) {
            break; // Stop propagation if handler returns false
        }
    }

    return true;
}
