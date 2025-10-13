#ifndef PATHS_H
#define PATHS_H

#define MAX_PATH 512

#include "define.h"
#include <string.h>

typedef enum { PATH_PHYSICAL, PATH_VIRTUAL } path_type_t;

typedef struct {
    char buffer[MAX_PATH];
} path_t;

INL path_t path_normalize(const char *raw_path) {
    path_t out;
    char temp[MAX_PATH];
    uint32_t len = 0;

    for (uint32_t i = 0; raw_path[i] && i < MAX_PATH - 1; ++i) {
        temp[i] = (raw_path[i] == '\\') ? '/' : raw_path[i];
        len = i + 1;
    }
    temp[len] = '\0';

    char *tokens[128];
    uint32_t token_count = 0;

    const char *token = temp;
    while (*token && token_count < 128) {
        while (*token == '/') {
            ++token;
        }
        if (*token == '\0') break;

        char *token_start = (char *)token;
        while (*token && *token != '/') {
            ++token;
        }

        if (*token) {
            *((char *)token) = '\0';
            ++token;
        }

        if (strcmp(token_start, ".") == 0) {
            // skip it.
        } else if (strcmp(token_start, "..") == 0) {
            if (token_count > 0) {
                token_count--;
            }
        } else {
            tokens[token_count++] = token_start;
        }
    }

    len = 0;
    for (uint32_t i = 0; i < token_count; ++i) {
        uint32_t token_len = (uint32_t)strlen(tokens[i]);

        if (len + token_len + 1 < MAX_PATH) {
            out.buffer[len++] = '/';
            memcpy(&out.buffer[len], tokens[i], token_len);
            len += token_len;
        }
    }

    if (len == 0) {
        out.buffer[0] = '/';
        len = 1;
    }

    out.buffer[len] = '\0';
    return out;
}

INL path_t path_join(const char *base, const char *extra) {
    if (extra == NULL || extra[0] == '\0') {
        LOG_FATAL("called with empty subpath");
        path_t empty = {0};
        return empty;
    }

    char combined[MAX_PATH];
    uint32_t len = 0;

    for (; *base && len < MAX_PATH - 1; ++base) {
        combined[len++] = *base;
    }

    if (len > 0 && combined[len - 1] != '/') {
        combined[len++] = '/';
    }

    for (; *extra && len < MAX_PATH - 1; ++extra) {
        combined[len++] = *extra;
    }

    combined[len] = '\0';
    return path_normalize(combined);
}

INL bool path_is_absolute(const char *path) {
#ifdef PLATFORM_LINUX
    return path[0] == '/';
#elif PLATFORM_WINDOWS
    return (path[0] && path[1] == ':') || (path[0] == '\\' || path[0] == '/');
#endif
}

INL bool path_is_relative(const char *path) { return !path_is_absolute(path); }

#endif // PATHS_H
