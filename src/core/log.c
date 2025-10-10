#include "define.h"

#include <stdio.h> // IWYU pragma: keep
#include <stdarg.h>

#define MSG_BUFFER 16000
#define FINAL_BUFFER 24000

#if PLATFORM_LINUX
#    define WHITE "38;5;15m"
#    define RED "38;5;196m"
#    define ORANGE "38;5;208m"
#    define YELLOW "38;5;220m"
#    define GREEN "38;5;040m"
#    define CYAN "38;5;050m"
#    define MAGENTA "38;5;219m"
#elif PLATFORM_WINDOWS
#    define WHITE 0x0F
#    define RED 0x0C
#    define ORANGE 0x06
#    define YELLOW 0x0E
#    define GREEN 0x0A
#    define CYAN 0x0B
#    define MAGENTA 0x0D
#endif

static const char *lvl_str[6] = {"[TRACE]", "[DEBUG]", "[INFO]",
                                 "[WARN]",  "[ERROR]", "[FATAL]"};

void log_console(const char *msg, uint8_t color) {
#if PLATFORM_LINUX
    const char *color_string[] = {
        MAGENTA, // TRACE
        CYAN,    // DEBUG
        WHITE,   // INFO
        YELLOW,  // WARN
        ORANGE,  // ERROR
        RED      // FATAL
    };

    printf("\033[%s%s\033[0m", color_string[color], msg);
#elif PLATFORM_WINDOWS
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    static u8 types[6] = {
        MAGENTA, // TRACE
        CYAN,    // DEBUG
        WHITE,   // INFO
        YELLOW,  // WARN
        ORANGE,  // ERROR
        RED,     // FATAL
    };

    SetConsoleTextAttribute(console_handle, types[color]);
    DWORD written = 0;
    WriteConsoleA(console_handle, msg, (DWORD)strlen(msg), &written, 0);
#endif
}

void log_msg(log_level_t level, const char *msg, ...) {
    char msg_buffer[MSG_BUFFER];
    char final_buffer[FINAL_BUFFER];

    va_list p_arg;
    va_start(p_arg, msg);
    vsnprintf(msg_buffer, sizeof(msg_buffer), msg, p_arg);
    va_end(p_arg);

    snprintf(final_buffer, sizeof(final_buffer), "%s %s\n", lvl_str[level],
             msg_buffer);

    log_console(final_buffer, (uint8_t)level);
}
