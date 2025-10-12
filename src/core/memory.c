#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALLOC_TRACK 4096
#define BUFFER_SIZE 8192

static const char *tag_str[MEM_MAX_TAG] = {"MEM_UNKNOWN", "MEM_ENGINE",
                                           "MEM_GAME",    "MEM_ARENA",
                                           "MEM_RENDER",  "MEM_AUDIO",
                                           "MEM_ARRAY",   "MEM_DYNARRAY",
                                           "MEM_STRING",  "MEM_RESOURCE"};

struct status {
    uint64_t total_allocated;
    uint64_t tag_alloc_count[MEM_MAX_TAG];
    uint64_t tag_allocation[MEM_MAX_TAG];
};

typedef struct {
    memtag_t tag;
    uint64_t size;
    uint32_t line;
    const char *file;
    void *ptr;
} mem_state;

static struct status g_counter = {0};
static mem_state *g_mem;
static uint64_t g_mem_count = 0;
static uint64_t g_mem_capacity = 0;

static void memory_report_leaks(void) {
    if (g_mem_count == 0) {
        LOG_INFO("No memory leaks detected.");
        return;
    }

    printf("\n");
    LOG_WARN("====== MEMORY LEAKS (%lu) ======", g_mem_count);
    for (uint64_t i = 0; i < g_mem_count; ++i) {
        const mem_state *m = &g_mem[i];
        LOG_WARN("at %s:%u → %lu bytes [%s]", m->file, m->line, m->size,
                 tag_str[m->tag]);
    }
}

bool memory_system_init(uint64_t total_size) {
    g_mem_capacity = total_size / sizeof(mem_state);
    // g_mem = platform_allocate(sizeof(mem_state) * g_mem_capacity, false);
    g_mem = malloc(sizeof(mem_state) * g_mem_capacity);
    if (!g_mem) return false;

    g_mem_count = 0;
    g_counter = (struct status){0};

    return true;
}

void memory_system_kill(void) {
    if (g_mem) {
        memory_report_leaks();
        free(g_mem);
        g_mem = 0;
        g_mem_count = 0;
        LOG_INFO("Memory system kill");
    }
}

void *alloc_dbg(uint64_t size, memtag_t tag, const char *file, uint32_t line) {
    void *block = malloc(size);
    if (!block) return 0;

    memset(block, 0, size);
    if (g_mem_count < g_mem_capacity) {
        g_mem[g_mem_count++] = (mem_state){
            .ptr = block,
            .size = size,
            .tag = tag,
            .file = file,
            .line = line,
        };
    }

    g_counter.total_allocated += size;
    g_counter.tag_alloc_count[tag]++;
    g_counter.tag_allocation[tag] += size;

    return block;
}

void alloc_free(void *block, uint64_t size, memtag_t tag) {
    if (!block) return;

    bool found = false;
    for (uint64_t i = 0; i < g_mem_count; ++i) {
        if (g_mem[i].ptr == block) {
            g_mem[i] = g_mem[--g_mem_count];
            found = true;

            break;
        }
    }

    if (!found) {
        LOG_WARN("attempted to free unknown ptr %p", block);
    }

    free(block);

    g_counter.tag_alloc_count[tag]--;
    g_counter.tag_allocation[tag] -= size;
}

char *mem_debug_stat(void) {
    const uint64_t Gib = 1024 * 1024 * 1024;
    const uint64_t Mib = 1024 * 1024;
    const uint64_t Kib = 1024;

    static char buffer[BUFFER_SIZE];
    uint64_t offset = 0;
    offset += (uint64_t)snprintf(buffer + offset, sizeof(buffer) - offset,
                                 "Game Memory Used:\n");

    for (uint32_t i = 0; i < MEM_MAX_TAG; ++i) {
        char *unit = "B";
        uint32_t count = (uint32_t)g_counter.tag_alloc_count[i];
        float amount = (float)g_counter.tag_allocation[i];

        if (count == 0) {
            continue;
        }

        if (amount >= (float)Gib) {
            amount /= (float)Gib;
            unit = "Gib";
        } else if (amount >= (float)Mib) {
            amount /= (float)Mib;
            unit = "Mib";
        } else if (amount >= (float)Kib) {
            amount /= (float)Kib;
            unit = "Kib";
        }
        count = (uint32_t)g_counter.tag_alloc_count[i];

        int32_t length =
            snprintf(buffer + offset, sizeof(buffer) - offset,
                     "--> %s: [%u] %.2f%s\n", tag_str[i], count, amount, unit);

        if (length > 0 && (offset + (uint32_t)length < BUFFER_SIZE)) {
            offset += (uint32_t)length;
        } else {
            break;
        }
    }
    return buffer;
}
