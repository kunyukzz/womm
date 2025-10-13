#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "core/define.h"
#include "core/arena.h"
#include "core/paths.h"

typedef struct {
    void *handle;
    bool is_valid;
} file_t;

typedef enum {
    READ_TEXT = 0x01,
    READ_BINARY = 0x02,
    WRITE_TEXT = 0x04,
    WRITE_BINARY = 0x08
} filemode_t;

typedef struct {
    arena_alloc_t *arena;
    path_t base_path;
    bool is_available;
} file_system_t;

file_system_t *filesys_init(arena_alloc_t *arena);
void filesys_kill(file_system_t *fs);

bool filesys_exist(const char *path);
bool filesys_open(const char *path, filemode_t mode, file_t *handle);
void filesys_close(file_t *handle);

bool filesys_size(file_t *handle, uint64_t *size);
bool filesys_read_all_text(file_t *handle, char *text, uint64_t *out_read);
bool filesys_read_all_binary(file_t *handle, uint8_t *out_byte,
                             uint64_t *out_read);

#endif // FILESYSTEM_H
