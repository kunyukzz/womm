#ifndef HASH_H
#define HASH_H

#include "core/define.h"
#include "core/arena.h"

typedef struct hash_table_t hash_table_t;

hash_table_t *hash_create(arena_alloc_t *arena, uint32_t capacity);
void hash_kill(hash_table_t *hash);

bool hash_set(hash_table_t *hash, const char *key, uint64_t value);
bool hash_get(hash_table_t *hash, const char *key, uint64_t *out);
bool hash_remove(hash_table_t *hash, const char *key);

#endif // HASH_H
