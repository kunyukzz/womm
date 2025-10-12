#include "arena.h"
#include "memory.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_ALIGNMENT 0x08

bool arena_create(uint64_t total_size, arena_alloc_t *arena, void *memory) {
    if (!arena) return false;

    arena->total_size = total_size;
    arena->prev_offset = 0;
    arena->curr_offset = 0;
    arena->own_memory = memory == NULL;

    if (memory) {
        arena->memory = memory;
    } else {
        arena->memory = WALLOC(total_size, MEM_ARENA);
    }

    if (!memory) return false;

    return true;
}

void arena_kill(arena_alloc_t *arena) {
    if (!arena) return;

    if (arena->memory) {
        WFREE(arena->memory, arena->total_size, MEM_ARENA);
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
    return arena_alloc_align(arena, size, DEFAULT_ALIGNMENT);
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
