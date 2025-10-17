#ifndef BINARY_LOADER_H
#define BINARY_LOADER_H

#include "define.h"
#include "memory.h"
#include "platform/filesystem.h"

INL void *read_file_binary(const char *path, uint64_t *out_size) {
    file_t file;
    if (!filesys_open(path, READ_BINARY, &file)) {
        LOG_ERROR("Failed to open binary file: %s", path);
        return NULL;
    }

    uint64_t size = 0;
    if (!filesys_size(&file, &size) || size == 0) {
        LOG_ERROR("Binary file is empty: %s", path);
        filesys_close(&file);
        return NULL;
    }

    uint8_t *data = WALLOC(size, MEM_RESOURCE);
    uint64_t read_size = 0;
    if (!filesys_read_all_binary(&file, data, &read_size) ||
        read_size != size) {
        LOG_ERROR("Failed to read binary file: %s", path);
        filesys_close(&file);
        WFREE(data, size, MEM_RESOURCE);
        return NULL;
    }

    filesys_close(&file);
    *out_size = size;
    return data;
}

INL void *read_file_text(const char *path, uint64_t *out_size) {
    file_t file;
    if (!filesys_open(path, READ_TEXT, &file)) {
        LOG_ERROR("Failed to open text file: %s", path);
        return NULL;
    }

    uint64_t size = 0;
    if (!filesys_size(&file, &size) || size == 0) {
        LOG_ERROR("Text file is empty: %s", path);
        filesys_close(&file);
        return NULL;
    }

    char *data = WALLOC(size, MEM_RESOURCE);
    uint64_t read_size = 0;
    if (!filesys_read_all_text(&file, data, &read_size) || read_size != size) {
        LOG_ERROR("Failed to read text file: %s", path);
        filesys_close(&file);
        WFREE(data, size, MEM_RESOURCE);
        return NULL;
    }

    data[size] = '\0';
    filesys_close(&file);
    *out_size = size;
    return data;
}

#endif // BINARY_LOADER_H
