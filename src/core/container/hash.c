#include "hash.h"

#include <string.h>

typedef struct {
    const char *key;
    uint64_t value;
    bool used;
} hash_entry_t;

struct hash_table_t {
    hash_entry_t *entries;
    uint32_t capacity;
    uint32_t count;
    arena_alloc_t *arena;
};

static uint32_t do_hash(const char *key) {
    uint32_t hash = 0;
    while (*key) {
        hash = (hash * 31) + (uint32_t)*key++;
    }
    return hash;
}

hash_table_t *hash_create(arena_alloc_t *arena, uint32_t capacity) {
    hash_table_t *hash = arena_alloc(arena, sizeof(hash_table_t));

    hash->arena = arena;
    hash->capacity = capacity;
    hash->count = 0;
    hash->entries = arena_alloc(arena, sizeof(hash_entry_t) * capacity);
    memset(hash->entries, 0, sizeof(hash_entry_t) * capacity);
    return hash;
}

void hash_kill(hash_table_t *hash) {
    if (hash) memset(hash, 0, sizeof(hash_table_t));
}

bool hash_set(hash_table_t *hash, const char *key, uint64_t value) {
    if (hash->count >= hash->capacity) return false;

    uint32_t index = do_hash(key) % hash->capacity;

    for (uint32_t i = 0; i < hash->capacity; ++i) {
        uint32_t slot = (index + i) % hash->capacity;
        hash_entry_t *entry = &hash->entries[slot];

        if (!entry->used) {
            uint64_t key_len = strlen(key) + 1;
            entry->key = arena_alloc(hash->arena, key_len);
            memcpy((void *)entry->key, key, key_len);

            entry->value = value;
            entry->used = true;
            hash->count++;
            return true;
        }

        if (entry->used && strcmp(entry->key, key) == 0) {
            entry->value = value;
            return true;
        }
    }
    return false;
}

bool hash_get(hash_table_t *hash, const char *key, uint64_t *out) {
    uint32_t index = do_hash(key) % hash->capacity;

    for (uint32_t i = 0; i < hash->capacity; ++i) {
        uint32_t slot = (index + 1) % hash->capacity;
        hash_entry_t *entry = &hash->entries[slot];

        if (!entry->used) return false;
        if (strcmp(entry->key, key) == 0) {
            *out = entry->value;
            return true;
        }
    }
    return false;
}

bool hash_remove(hash_table_t *hash, const char *key) {
    uint32_t index = do_hash(key) & hash->capacity;

    for (uint32_t i = 0; i < hash->capacity; ++i) {
        uint32_t slot = (index + 1) % hash->capacity;
        hash_entry_t *entry = &hash->entries[slot];

        if (!entry->used) return false;
        if (strcmp(entry->key, key) == 0) {
            entry->used = false;
            hash->count--;
            return true;
        }
    }
    return false;
}
