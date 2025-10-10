#include "input.h"

static const char *key_names[INPUT_KEY_MAX] = {
    [INPUT_KEY_UNKNOWN] = "Unknown",
    [INPUT_KEY_A] = "A",
    [INPUT_KEY_B] = "B",
    [INPUT_KEY_C] = "C",
    [INPUT_KEY_D] = "D",
    [INPUT_KEY_E] = "E",
    [INPUT_KEY_F] = "F",
    [INPUT_KEY_G] = "G",
    [INPUT_KEY_H] = "H",
    [INPUT_KEY_I] = "I",
    [INPUT_KEY_J] = "J",
    [INPUT_KEY_K] = "K",
    [INPUT_KEY_L] = "L",
    [INPUT_KEY_M] = "M",
    [INPUT_KEY_N] = "N",
    [INPUT_KEY_O] = "O",
    [INPUT_KEY_P] = "P",
    [INPUT_KEY_Q] = "Q",
    [INPUT_KEY_R] = "R",
    [INPUT_KEY_S] = "S",
    [INPUT_KEY_T] = "T",
    [INPUT_KEY_U] = "U",
    [INPUT_KEY_V] = "V",
    [INPUT_KEY_W] = "W",
    [INPUT_KEY_X] = "X",
    [INPUT_KEY_Y] = "Y",
    [INPUT_KEY_Z] = "Z",
    [INPUT_KEY_1] = "1",
    [INPUT_KEY_2] = "2",
    [INPUT_KEY_3] = "3",
    [INPUT_KEY_4] = "4",
    [INPUT_KEY_5] = "5",
    [INPUT_KEY_6] = "6",
    [INPUT_KEY_7] = "7",
    [INPUT_KEY_8] = "8",
    [INPUT_KEY_9] = "9",
    [INPUT_KEY_0] = "0",
    [INPUT_KEY_F1] = "F1",
    [INPUT_KEY_F2] = "F2",
    [INPUT_KEY_F3] = "F3",
    [INPUT_KEY_F4] = "F4",
    [INPUT_KEY_F5] = "F5",
    [INPUT_KEY_F6] = "F6",
    [INPUT_KEY_F7] = "F7",
    [INPUT_KEY_F8] = "F8",
    [INPUT_KEY_F9] = "F9",
    [INPUT_KEY_F10] = "F10",
    [INPUT_KEY_F11] = "F11",
    [INPUT_KEY_F12] = "F12",
    [INPUT_KEY_ESCAPE] = "Escape",
    [INPUT_KEY_ENTER] = "Enter",
    [INPUT_KEY_TAB] = "Tab",
    [INPUT_KEY_BACKSPACE] = "Backspace",
    [INPUT_KEY_INSERT] = "Insert",
    [INPUT_KEY_DELETE] = "Delete",
    [INPUT_KEY_HOME] = "Home",
    [INPUT_KEY_END] = "End",
    [INPUT_KEY_PAGEUP] = "Pageup",
    [INPUT_KEY_PAGEDOWN] = "Pagedown",
    [INPUT_KEY_LEFT] = "Left",
    [INPUT_KEY_RIGHT] = "Right",
    [INPUT_KEY_UP] = "Up",
    [INPUT_KEY_DOWN] = "Down",
    [INPUT_KEY_LSHIFT] = "Lshift",
    [INPUT_KEY_RSHIFT] = "Rshift",
    [INPUT_KEY_LCONTROL] = "Lcontrol",
    [INPUT_KEY_RCONTROL] = "Rcontrol",
    [INPUT_KEY_LALT] = "Lalt",
    [INPUT_KEY_RALT] = "Ralt",
    [INPUT_KEY_LSUPER] = "Lsuper",
    [INPUT_KEY_RSUPER] = "Rsuper",
    [INPUT_KEY_CAPS] = "Caps",
    [INPUT_KEY_NUM] = "Num",
    [INPUT_KEY_KP_0] = "Kp 0",
    [INPUT_KEY_KP_1] = "Kp 1",
    [INPUT_KEY_KP_2] = "Kp 2",
    [INPUT_KEY_KP_3] = "Kp 3",
    [INPUT_KEY_KP_4] = "Kp 4",
    [INPUT_KEY_KP_5] = "Kp 5",
    [INPUT_KEY_KP_6] = "Kp 6",
    [INPUT_KEY_KP_7] = "Kp 7",
    [INPUT_KEY_KP_8] = "Kp 8",
    [INPUT_KEY_KP_9] = "Kp 9",
    [INPUT_KEY_KP_DECIMAL] = "Kp Decimal",
    [INPUT_KEY_KP_ENTER] = "Kp Enter",
    [INPUT_KEY_KP_ADD] = "Kp Add",
    [INPUT_KEY_KP_SUBSTRACT] = "Kp Substract",
    [INPUT_KEY_KP_MULTIPLY] = "Kp Multiply",
    [INPUT_KEY_KP_DIVIDE] = "Kp Divide",
    [INPUT_KEY_SPACE] = "Space",
    [INPUT_KEY_MINUS] = "Minus",
    [INPUT_KEY_EQUAL] = "Equal",
    [INPUT_KEY_LBRACKET] = "Lbracket",
    [INPUT_KEY_RBRACKET] = "Rbracket",
    [INPUT_KEY_LBRACE] = "Lbrace",
    [INPUT_KEY_RBRACE] = "Rbrace",
    [INPUT_KEY_BACKSLASH] = "Backslash",
    [INPUT_KEY_SEMICOLON] = "Semicolon",
    [INPUT_KEY_APOSTROPHE] = "Apostrophe",
    [INPUT_KEY_GRAVE] = "Grave",
    [INPUT_KEY_COMMA] = "Comma",
    [INPUT_KEY_PERIOD] = "Period",
    [INPUT_KEY_SLASH] = "Slash",
    [INPUT_KEY_PRINT_SCREEN] = "Print Screen",
    [INPUT_KEY_SCROLL_LOCK] = "Scroll Lock",
    [INPUT_KEY_PAUSE] = "Pause",
};

const char *keycode_to_str(input_keys_t key) {
    if (key >= 0 && key < INPUT_KEY_MAX) {
        const char *s = key_names[key];
        return s ? s : "Unknown";
    }
    return "Invalid";
}

#include <X11/keysym.h>

input_keys_t keycode_translate(uint32_t keycode) {
    switch (keycode) {
        // Alphanumeric
        case XK_a:
        case XK_A: return INPUT_KEY_A;
        case XK_b:
        case XK_B: return INPUT_KEY_B;
        case XK_c:
        case XK_C: return INPUT_KEY_C;
        case XK_d:
        case XK_D: return INPUT_KEY_D;
        case XK_e:
        case XK_E: return INPUT_KEY_E;
        case XK_f:
        case XK_F: return INPUT_KEY_F;
        case XK_g:
        case XK_G: return INPUT_KEY_G;
        case XK_h:
        case XK_H: return INPUT_KEY_H;
        case XK_i:
        case XK_I: return INPUT_KEY_I;
        case XK_j:
        case XK_J: return INPUT_KEY_J;
        case XK_k:
        case XK_K: return INPUT_KEY_K;
        case XK_l:
        case XK_L: return INPUT_KEY_L;
        case XK_m:
        case XK_M: return INPUT_KEY_M;
        case XK_n:
        case XK_N: return INPUT_KEY_N;
        case XK_o:
        case XK_O: return INPUT_KEY_O;
        case XK_p:
        case XK_P: return INPUT_KEY_P;
        case XK_q:
        case XK_Q: return INPUT_KEY_Q;
        case XK_r:
        case XK_R: return INPUT_KEY_R;
        case XK_s:
        case XK_S: return INPUT_KEY_S;
        case XK_t:
        case XK_T: return INPUT_KEY_T;
        case XK_u:
        case XK_U: return INPUT_KEY_U;
        case XK_v:
        case XK_V: return INPUT_KEY_V;
        case XK_w:
        case XK_W: return INPUT_KEY_W;
        case XK_x:
        case XK_X: return INPUT_KEY_X;
        case XK_y:
        case XK_Y: return INPUT_KEY_Y;
        case XK_z:
        case XK_Z: return INPUT_KEY_Z;

        case XK_1: return INPUT_KEY_1;
        case XK_2: return INPUT_KEY_2;
        case XK_3: return INPUT_KEY_3;
        case XK_4: return INPUT_KEY_4;
        case XK_5: return INPUT_KEY_5;
        case XK_6: return INPUT_KEY_6;
        case XK_7: return INPUT_KEY_7;
        case XK_8: return INPUT_KEY_8;
        case XK_9: return INPUT_KEY_9;
        case XK_0: return INPUT_KEY_0;

        // Function Keys
        case XK_F1: return INPUT_KEY_F1;
        case XK_F2: return INPUT_KEY_F2;
        case XK_F3: return INPUT_KEY_F3;
        case XK_F4: return INPUT_KEY_F4;
        case XK_F5: return INPUT_KEY_F5;
        case XK_F6: return INPUT_KEY_F6;
        case XK_F7: return INPUT_KEY_F7;
        case XK_F8: return INPUT_KEY_F8;
        case XK_F9: return INPUT_KEY_F9;
        case XK_F10: return INPUT_KEY_F10;
        case XK_F11: return INPUT_KEY_F11;
        case XK_F12: return INPUT_KEY_F12;

        case XK_Escape: return INPUT_KEY_ESCAPE;
        case XK_Return: return INPUT_KEY_ENTER;
        case XK_Tab: return INPUT_KEY_TAB;
        case XK_BackSpace: return INPUT_KEY_BACKSPACE;
        case XK_Insert: return INPUT_KEY_INSERT;
        case XK_Delete: return INPUT_KEY_DELETE;
        case XK_Home: return INPUT_KEY_HOME;
        case XK_End: return INPUT_KEY_END;
        case XK_Page_Up: return INPUT_KEY_PAGEUP;
        case XK_Page_Down: return INPUT_KEY_PAGEDOWN;

        case XK_Left: return INPUT_KEY_LEFT;
        case XK_Right: return INPUT_KEY_RIGHT;
        case XK_Up: return INPUT_KEY_UP;
        case XK_Down: return INPUT_KEY_DOWN;

        // Modifier Keys
        case XK_Shift_L: return INPUT_KEY_LSHIFT;
        case XK_Shift_R: return INPUT_KEY_RSHIFT;
        case XK_Control_L: return INPUT_KEY_LCONTROL;
        case XK_Control_R: return INPUT_KEY_RCONTROL;
        case XK_Alt_L: return INPUT_KEY_LALT;
        case XK_Alt_R: return INPUT_KEY_RALT;
        case XK_Super_L: return INPUT_KEY_LSUPER;
        case XK_Super_R: return INPUT_KEY_RSUPER;
        case XK_Caps_Lock: return INPUT_KEY_CAPS;
        case XK_Num_Lock: return INPUT_KEY_NUM;

        // Numpad
        case XK_KP_0: return INPUT_KEY_KP_0;
        case XK_KP_1: return INPUT_KEY_KP_1;
        case XK_KP_2: return INPUT_KEY_KP_2;
        case XK_KP_3: return INPUT_KEY_KP_3;
        case XK_KP_4: return INPUT_KEY_KP_4;
        case XK_KP_5: return INPUT_KEY_KP_5;
        case XK_KP_6: return INPUT_KEY_KP_6;
        case XK_KP_7: return INPUT_KEY_KP_7;
        case XK_KP_8: return INPUT_KEY_KP_8;
        case XK_KP_9: return INPUT_KEY_KP_9;
        case XK_KP_Decimal: return INPUT_KEY_KP_DECIMAL;
        case XK_KP_Enter: return INPUT_KEY_KP_ENTER;
        case XK_KP_Add: return INPUT_KEY_KP_ADD;
        case XK_KP_Subtract: return INPUT_KEY_KP_SUBSTRACT;
        case XK_KP_Multiply: return INPUT_KEY_KP_MULTIPLY;
        case XK_KP_Divide: return INPUT_KEY_KP_DIVIDE;

        // Symbols
        case XK_space: return INPUT_KEY_SPACE;
        case XK_minus: return INPUT_KEY_MINUS;
        case XK_equal: return INPUT_KEY_EQUAL;
        case XK_bracketleft: return INPUT_KEY_LBRACKET;
        case XK_bracketright: return INPUT_KEY_RBRACKET;
        case XK_braceleft: return INPUT_KEY_LBRACE;
        case XK_braceright: return INPUT_KEY_RBRACE;
        case XK_backslash: return INPUT_KEY_BACKSLASH;
        case XK_semicolon: return INPUT_KEY_SEMICOLON;
        case XK_apostrophe: return INPUT_KEY_APOSTROPHE;
        case XK_grave: return INPUT_KEY_GRAVE;
        case XK_comma: return INPUT_KEY_COMMA;
        case XK_period: return INPUT_KEY_PERIOD;
        case XK_slash: return INPUT_KEY_SLASH;

        // Misc
        case XK_Print: return INPUT_KEY_PRINT_SCREEN;
        case XK_Scroll_Lock: return INPUT_KEY_SCROLL_LOCK;
        case XK_Pause: return INPUT_KEY_PAUSE;

        default: return INPUT_KEY_UNKNOWN;
    }
}
