#include "window.h"
#if PLATFORM_LINUX
#    include <time.h>
#    include <string.h>
#    include <unistd.h>

#    include <X11/XKBlib.h>
#    include <X11/Xlib.h>

window_system_t *window_system_init(window_config_t config,
                                    arena_alloc_t *arena) {
    window_system_t *window = arena_alloc(arena, sizeof(window_system_t));
    if (!window) return NULL;
    memset(window, 0, sizeof(window_system_t));

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        LOG_FATAL("failed to create window");
        return false;
    }

    int32_t screen = DefaultScreen(dpy);

    XSetWindowAttributes attrs;
    attrs.background_pixel = BlackPixel(dpy, screen);
    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
                       StructureNotifyMask | ButtonPressMask |
                       ButtonReleaseMask;

    uint32_t value_mask = CWBackPixel | CWEventMask;

    Window win = XCreateWindow(dpy, RootWindow(dpy, screen), 0, 0, config.width,
                               config.height, 0, CopyFromParent, InputOutput,
                               CopyFromParent, value_mask, &attrs);

    if (!win) {
        LOG_FATAL("XCreateWindow failed");
        XCloseDisplay(dpy);
        return false;
    }

    int screen_width = DisplayWidth(dpy, screen);
    int screen_height = DisplayHeight(dpy, screen);
    int x = ((uint32_t)screen_width - config.width) / 2;
    int y = ((uint32_t)screen_height - config.height) / 2;

    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(dpy, win, &wm_delete, 1);
    XStoreName(dpy, win, config.name);
    XMapWindow(dpy, win);
    XMoveWindow(dpy, win, x, y);
    XFlush(dpy);

    window->native_win.display = dpy;
    window->native_win.win = win;
    window->native_win.atom = wm_delete;

    LOG_INFO("window system initialized (X11)");
    return window;
}

void window_system_kill(window_system_t *window) {
    if (window->native_win.display) {
        XDestroyWindow(window->native_win.display, window->native_win.win);
        window->native_win.win = 0;
    }
    if (window->native_win.display) {
        XCloseDisplay(window->native_win.display);
        window->native_win.display = NULL;
    }
    LOG_INFO("window system kill (X11)");
}

bool window_system_pump(window_system_t *window, input_system_t *input,
                        event_system_t *event) {
    XEvent ev;
    bool is_quit = false;

    while (XPending(window->native_win.display)) {
        XNextEvent(window->native_win.display, &ev);

        switch (ev.type) {
            case KeyPress:
            case KeyRelease: {
                bool pressed = ev.type == KeyPress;
                KeySym ks =
                    XkbKeycodeToKeysym(window->native_win.display,
                                       (KeyCode)ev.xkey.keycode, 0,
                                       (ev.xkey.state & ShiftMask) ? 1 : 0);
                input_keys_t key = keycode_translate((uint32_t)ks);
                input_process_key(input, event, key, pressed);
            } break;

            case ButtonPress:
            case ButtonRelease: {
                bool pressed = ev.type == ButtonPress;
                input_button_t mb = INPUT_MB_MAX;
                switch (ev.xbutton.button) {
                    case Button1: mb = INPUT_MB_LEFT; break;
                    case Button2: mb = INPUT_MB_MIDDLE; break;
                    case Button3: mb = INPUT_MB_RIGHT; break;
                }
                if (mb != INPUT_MB_MAX)
                    input_process_button(input, event, mb, pressed);
            } break;

            case MotionNotify: {
                input_process_mouse_move(input, event, (int16_t)ev.xmotion.x,
                                         (int16_t)ev.xmotion.y);
            } break;

            case ConfigureNotify: {
                XConfigureEvent *ce = (XConfigureEvent *)&ev;

                event_t e;
                e.data.resize.width = (uint16_t)ce->width;
                e.data.resize.height = (uint16_t)ce->height;
                event_push(event, EVENT_RESIZE, &e, NULL);
            } break;

            case UnmapNotify: {
                event_t e = {};
                event_push(event, EVENT_SUSPEND, &e, NULL);
            } break;

            case MapNotify: {
                event_t e = {};
                event_push(event, EVENT_RESUME, &e, NULL);
            } break;

            case ClientMessage: {
                XClientMessageEvent *cm = (XClientMessageEvent *)&ev;

                if ((Atom)cm->data.l[0] == window->native_win.atom) {
                    event_t e;
                    e.data.raw[0] = (uint8_t)cm->window;
                    event_push(event, EVENT_QUIT, &e, NULL);
                    is_quit = true;
                }
            } break;

            default: break;
        }
        return !is_quit;
    }
    return true;
}

void *window_system_get_native_display(const window_system_t *window) {
    return window->native_win.display; // Returns Display* for X11
}

void *window_system_get_native_window(const window_system_t *window) {
    return (void *)window->native_win.win; // Returns Window for X11
}

double get_abs_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void get_sleep(double wake) {
    struct timespec ts;
    ts.tv_sec = (time_t)wake;
    ts.tv_nsec = (long)((wake - (double)ts.tv_sec) * 1e9);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL);
}

bool get_current_dir(char *out_path, uint64_t max_len) {
    if (getcwd(out_path, max_len) != NULL) {
        return true;
    }
    return false;
}

#endif
