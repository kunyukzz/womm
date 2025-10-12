#ifndef MEMORY_H
#define MEMORY_H

#include "define.h"

typedef enum {
    MEM_UNKNOWN = 0x00,
    MEM_ENGINE,
    MEM_GAME,
    MEM_ARENA,
    MEM_RENDER,
    MEM_AUDIO,
    MEM_ARRAY,
    MEM_DYNARRAY,
    MEM_STRING,
    MEM_RESOURCE,
    MEM_MAX_TAG
} memtag_t;

#define WALLOC(size, tag) alloc_dbg(size, tag, __FILE__, __LINE__)
#define WFREE(block, size, tag) alloc_free(block, size, tag)

bool memory_system_init(uint64_t total_size);
void memory_system_kill(void);

void *alloc_dbg(uint64_t size, memtag_t tag, const char *file, uint32_t line);
void alloc_free(void *block, uint64_t size, memtag_t tag);

char *mem_debug_stat(void);
// void memory_report_leaks(void);

#endif // MEMORY_H
