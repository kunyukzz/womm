#include "arena.h"

#include <stdlib.h>
#include <string.h>

bool arena_set(uint64_t total_size, void *memory, arena_alloc_t *arena) {
    if (!memory || !arena) return false;

    arena->total_size = total_size;
    arena->prev_offset = 0;
    arena->curr_offset = 0;
    arena->memory = memory;
    arena->own_memory = false;

    return true;
}

bool arena_create(uint64_t total_size, arena_alloc_t *arena) {
    if (!arena) return false;

    void *memory = malloc(total_size);
    if (!memory) return false;

    return arena_set(total_size, memory, arena);
}

void arena_kill(arena_alloc_t *arena) {
    if (!arena) return;

    if (arena->own_memory && arena->memory) {
        free(arena->memory);
    }
    memset(arena, 0, sizeof(*arena));
}

void *arena_alloc_align(arena_alloc_t *arena, uint64_t size,
                        uint8_t alignment) {
    if (!arena || size == 0) return NULL;

    uint64_t aligned_offset =
        (arena->curr_offset + (alignment - 1)) & ~(alignment - 1);

    if (aligned_offset + size > arena->total_size) {
        return NULL;
    }

    arena->prev_offset = arena->curr_offset;
    arena->curr_offset = aligned_offset + size;

    return (uint8_t *)arena->memory + aligned_offset;
}

void *arena_alloc(arena_alloc_t *arena, uint64_t size) {
    return arena_alloc_align(arena, size, 8);
}

void arena_reset(arena_alloc_t *arena) {
    if (arena) {
        arena->prev_offset = 0;
        arena->curr_offset = 0;
    }
}

uint64_t arena_remaining(const arena_alloc_t *arena) {
    return arena ? (arena->total_size - arena->curr_offset) : 0;
}

uint64_t arena_used(const arena_alloc_t *arena) {
    return arena ? arena->curr_offset : 0;
}
