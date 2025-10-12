#ifndef ARENA_ALLOC_H
#define ARENA_ALLOC_H

#include "define.h"

typedef struct {
    uint64_t total_size;
    uint64_t prev_offset;
    uint64_t curr_offset;
    void *memory;
    bool own_memory;
} arena_alloc_t;

bool arena_create(uint64_t total_size, arena_alloc_t *arena, void *memory);
void arena_kill(arena_alloc_t *arena);

void *arena_alloc_align(arena_alloc_t *arena, uint64_t size, uint8_t alignment);
void *arena_alloc(arena_alloc_t *arena, uint64_t size);
void arena_reset(arena_alloc_t *arena);

uint64_t arena_remaining(const arena_alloc_t *arena);
uint64_t arena_used(const arena_alloc_t *arena);

#endif // ARENA_ALLOC_H
