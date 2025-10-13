#include "filesystem.h"
#include "window.h"

#include <stdio.h>
#include <sys/stat.h>

static file_system_t *g_file_system = NULL;
static const char *possible_paths[] =
    {"./assets",      // Same directory as executable (bin/)
     "../assets",     // Project root (if running from bin/)
     "../src/assets", // Inside src/ directory
     NULL};

file_system_t *filesys_init(arena_alloc_t *arena) {
    if (g_file_system != NULL) return g_file_system;

    file_system_t *fs = arena_alloc(arena, sizeof(file_system_t));
    if (!fs) return NULL;

    memset(fs, 0, sizeof(file_system_t));
    fs->arena = arena;

    char cwd[MAX_PATH];
    if (!get_current_dir(cwd, sizeof(cwd))) {
        LOG_FATAL("failed to get current working directory");
        return NULL;
    }

    // Try all possible asset paths
    for (uint16_t i = 0; possible_paths[i] != NULL; i++) {
        path_t test_path = path_join(cwd, possible_paths[i]);
        if (filesys_exist(test_path.buffer)) {
            fs->base_path = test_path;
            fs->is_available = true;
            g_file_system = fs;
            LOG_INFO("filesystem initialized base path = %s",
                     fs->base_path.buffer);
            return fs; // Fixed: should return fs, not true
        }
        LOG_DEBUG("tried: %s - not found", test_path.buffer);
    }

    // No assets folder found
    LOG_ERROR("could not locate assets folder (cwd=%s)", cwd);
    LOG_ERROR("tried the following paths:");
    for (uint16_t i = 0; possible_paths[i] != NULL; i++) {
        path_t test_path = path_join(cwd, possible_paths[i]);
        LOG_ERROR("  %s", test_path.buffer);
    }

    fs->is_available = true;
    g_file_system = fs;
    return NULL;
}

void filesys_kill(file_system_t *fs) {
    if (fs) {
        memset(fs, 0, sizeof(file_system_t));
    }
    LOG_INFO("filesystem release");
}

bool filesys_exist(const char *path) {
    struct stat buffer;
    return stat(path, &buffer) == 0;
}

bool filesys_open(const char *path, filemode_t mode, file_t *handle) {
    handle->is_valid = false;
    handle->handle = NULL;
    const char *mode_str = NULL;

    bool is_read = (mode & (READ_TEXT | READ_BINARY)) != 0;
    bool is_write = (mode & (WRITE_TEXT | WRITE_BINARY)) != 0;
    bool is_binary = (mode & (READ_BINARY | WRITE_BINARY)) != 0;

    if (is_read && is_write) {
        LOG_ERROR("cannot open file '%s' in both read & write mode", path);
        return false;
    }

    if (is_read) {
        mode_str = is_binary ? "rb" : "r";
    } else if (is_write) {
        mode_str = is_binary ? "wb" : "w";
    }

    if (!mode_str) {
        LOG_ERROR("invalid file mode flags for '%s'", path);
        return false;
    }

    char full_path[MAX_PATH];
    path_t joined = path_join(g_file_system->base_path.buffer, path);
    strncpy(full_path, joined.buffer, MAX_PATH);
    FILE *file = fopen(full_path, mode_str);
    if (!file) {
        LOG_ERROR("error open file '%s'", full_path);
        return false;
    }

    handle->handle = file;
    handle->is_valid = true;
    return true;
}

void filesys_close(file_t *handle) {
    if (handle->is_valid && handle->handle) {
        fclose((FILE *)handle->handle);
        handle->handle = NULL;
        handle->is_valid = false;
    }
}

bool filesys_size(file_t *handle, uint64_t *size) {
    if (handle->handle) {
        fseek((FILE *)handle->handle, 0, SEEK_END);
        *size = (uint64_t)ftell((FILE *)handle->handle);
        rewind((FILE *)handle->handle);
        return true;
    }
    return false;
}

bool filesys_read_all_text(file_t *handle, char *text, uint64_t *out_read) {
    if (handle->handle && text && out_read) {
        uint64_t size = 0;

        if (!filesys_size(handle, &size)) {
            return false;
        }

        *out_read = fread(text, 1, size, (FILE *)handle->handle);
        return *out_read == size;
    }
    return false;
}

bool filesys_read_all_binary(file_t *handle, uint8_t *out_byte,
                             uint64_t *out_read) {
    if (handle->handle && out_byte && out_read) {
        uint64_t size = 0;

        if (!filesys_size(handle, &size)) {
            return false;
        }

        *out_read = fread(out_byte, 1, size, (FILE *)handle->handle);
        return *out_read == size;
    }
    return false;
}
