/* controls.h - written by gen-config.py: what each control of waterbox.config
 * sends to ScummVM, in waterbox.config's order. */
#ifndef CHIMERA_CONTROLS_H
#define CHIMERA_CONTROLS_H

enum { SCVM_CTRL_MOUSE, SCVM_CTRL_WHEEL, SCVM_CTRL_KEY };
struct scvm_control { int kind; int code; int ascii; int shifted; };

#define SCVM_BTN_COUNT 106
#define SCVM_AXIS_COUNT 2
#define SCVM_KEY_LSHIFT 62
#define SCVM_KEY_RSHIFT 73
#define SCVM_KEY_LCTRL 74
#define SCVM_KEY_LALT 75
#define SCVM_KEY_RALT 77
#define SCVM_KEY_RCTRL 78

static const struct scvm_control scvm_controls[SCVM_BTN_COUNT] = {
	{ SCVM_CTRL_MOUSE, 0, 0, 0 }, /* Mouse Left Button */
	{ SCVM_CTRL_MOUSE, 1, 0, 0 }, /* Mouse Right Button */
	{ SCVM_CTRL_MOUSE, 2, 0, 0 }, /* Mouse Middle Button */
	{ SCVM_CTRL_WHEEL, 0, 0, 0 }, /* Mouse Wheel Up */
	{ SCVM_CTRL_WHEEL, 1, 0, 0 }, /* Mouse Wheel Down */
	{ SCVM_CTRL_KEY, Common::KEYCODE_ESCAPE, 0, 0 }, /* Key Escape */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F1, 0, 0 }, /* Key F1 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F2, 0, 0 }, /* Key F2 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F3, 0, 0 }, /* Key F3 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F4, 0, 0 }, /* Key F4 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F5, 0, 0 }, /* Key F5 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F6, 0, 0 }, /* Key F6 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F7, 0, 0 }, /* Key F7 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F8, 0, 0 }, /* Key F8 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F9, 0, 0 }, /* Key F9 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F10, 0, 0 }, /* Key F10 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F11, 0, 0 }, /* Key F11 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_F12, 0, 0 }, /* Key F12 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_PRINT, 0, 0 }, /* Key PrintScreen */
	{ SCVM_CTRL_KEY, Common::KEYCODE_SCROLLOCK, 0, 0 }, /* Key ScrollLock */
	{ SCVM_CTRL_KEY, Common::KEYCODE_PAUSE, 0, 0 }, /* Key Pause */
	{ SCVM_CTRL_KEY, Common::KEYCODE_BACKQUOTE, 96, 126 }, /* Key Grave */
	{ SCVM_CTRL_KEY, Common::KEYCODE_1, 49, 33 }, /* Key 1 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_2, 50, 64 }, /* Key 2 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_3, 51, 35 }, /* Key 3 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_4, 52, 36 }, /* Key 4 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_5, 53, 37 }, /* Key 5 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_6, 54, 94 }, /* Key 6 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_7, 55, 38 }, /* Key 7 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_8, 56, 42 }, /* Key 8 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_9, 57, 40 }, /* Key 9 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_0, 48, 41 }, /* Key 0 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_MINUS, 45, 95 }, /* Key Minus */
	{ SCVM_CTRL_KEY, Common::KEYCODE_EQUALS, 61, 43 }, /* Key Equals */
	{ SCVM_CTRL_KEY, Common::KEYCODE_BACKSPACE, 0, 0 }, /* Key Backspace */
	{ SCVM_CTRL_KEY, Common::KEYCODE_TAB, 0, 0 }, /* Key Tab */
	{ SCVM_CTRL_KEY, Common::KEYCODE_q, 113, 81 }, /* Key Q */
	{ SCVM_CTRL_KEY, Common::KEYCODE_w, 119, 87 }, /* Key W */
	{ SCVM_CTRL_KEY, Common::KEYCODE_e, 101, 69 }, /* Key E */
	{ SCVM_CTRL_KEY, Common::KEYCODE_r, 114, 82 }, /* Key R */
	{ SCVM_CTRL_KEY, Common::KEYCODE_t, 116, 84 }, /* Key T */
	{ SCVM_CTRL_KEY, Common::KEYCODE_y, 121, 89 }, /* Key Y */
	{ SCVM_CTRL_KEY, Common::KEYCODE_u, 117, 85 }, /* Key U */
	{ SCVM_CTRL_KEY, Common::KEYCODE_i, 105, 73 }, /* Key I */
	{ SCVM_CTRL_KEY, Common::KEYCODE_o, 111, 79 }, /* Key O */
	{ SCVM_CTRL_KEY, Common::KEYCODE_p, 112, 80 }, /* Key P */
	{ SCVM_CTRL_KEY, Common::KEYCODE_LEFTBRACKET, 91, 123 }, /* Key LeftBracket */
	{ SCVM_CTRL_KEY, Common::KEYCODE_RIGHTBRACKET, 93, 125 }, /* Key RightBracket */
	{ SCVM_CTRL_KEY, Common::KEYCODE_BACKSLASH, 92, 124 }, /* Key Backslash */
	{ SCVM_CTRL_KEY, Common::KEYCODE_CAPSLOCK, 0, 0 }, /* Key CapsLock */
	{ SCVM_CTRL_KEY, Common::KEYCODE_a, 97, 65 }, /* Key A */
	{ SCVM_CTRL_KEY, Common::KEYCODE_s, 115, 83 }, /* Key S */
	{ SCVM_CTRL_KEY, Common::KEYCODE_d, 100, 68 }, /* Key D */
	{ SCVM_CTRL_KEY, Common::KEYCODE_f, 102, 70 }, /* Key F */
	{ SCVM_CTRL_KEY, Common::KEYCODE_g, 103, 71 }, /* Key G */
	{ SCVM_CTRL_KEY, Common::KEYCODE_h, 104, 72 }, /* Key H */
	{ SCVM_CTRL_KEY, Common::KEYCODE_j, 106, 74 }, /* Key J */
	{ SCVM_CTRL_KEY, Common::KEYCODE_k, 107, 75 }, /* Key K */
	{ SCVM_CTRL_KEY, Common::KEYCODE_l, 108, 76 }, /* Key L */
	{ SCVM_CTRL_KEY, Common::KEYCODE_SEMICOLON, 59, 58 }, /* Key Semicolon */
	{ SCVM_CTRL_KEY, Common::KEYCODE_QUOTE, 39, 34 }, /* Key Quote */
	{ SCVM_CTRL_KEY, Common::KEYCODE_RETURN, 0, 0 }, /* Key Enter */
	{ SCVM_CTRL_KEY, Common::KEYCODE_LSHIFT, 0, 0 }, /* Key LeftShift */
	{ SCVM_CTRL_KEY, Common::KEYCODE_z, 122, 90 }, /* Key Z */
	{ SCVM_CTRL_KEY, Common::KEYCODE_x, 120, 88 }, /* Key X */
	{ SCVM_CTRL_KEY, Common::KEYCODE_c, 99, 67 }, /* Key C */
	{ SCVM_CTRL_KEY, Common::KEYCODE_v, 118, 86 }, /* Key V */
	{ SCVM_CTRL_KEY, Common::KEYCODE_b, 98, 66 }, /* Key B */
	{ SCVM_CTRL_KEY, Common::KEYCODE_n, 110, 78 }, /* Key N */
	{ SCVM_CTRL_KEY, Common::KEYCODE_m, 109, 77 }, /* Key M */
	{ SCVM_CTRL_KEY, Common::KEYCODE_COMMA, 44, 60 }, /* Key Comma */
	{ SCVM_CTRL_KEY, Common::KEYCODE_PERIOD, 46, 62 }, /* Key Period */
	{ SCVM_CTRL_KEY, Common::KEYCODE_SLASH, 47, 63 }, /* Key Slash */
	{ SCVM_CTRL_KEY, Common::KEYCODE_RSHIFT, 0, 0 }, /* Key RightShift */
	{ SCVM_CTRL_KEY, Common::KEYCODE_LCTRL, 0, 0 }, /* Key LeftCtrl */
	{ SCVM_CTRL_KEY, Common::KEYCODE_LALT, 0, 0 }, /* Key LeftAlt */
	{ SCVM_CTRL_KEY, Common::KEYCODE_SPACE, 32, 32 }, /* Key Space */
	{ SCVM_CTRL_KEY, Common::KEYCODE_RALT, 0, 0 }, /* Key RightAlt */
	{ SCVM_CTRL_KEY, Common::KEYCODE_RCTRL, 0, 0 }, /* Key RightCtrl */
	{ SCVM_CTRL_KEY, Common::KEYCODE_INSERT, 0, 0 }, /* Key Insert */
	{ SCVM_CTRL_KEY, Common::KEYCODE_HOME, 0, 0 }, /* Key Home */
	{ SCVM_CTRL_KEY, Common::KEYCODE_PAGEUP, 0, 0 }, /* Key PageUp */
	{ SCVM_CTRL_KEY, Common::KEYCODE_DELETE, 0, 0 }, /* Key Delete */
	{ SCVM_CTRL_KEY, Common::KEYCODE_END, 0, 0 }, /* Key End */
	{ SCVM_CTRL_KEY, Common::KEYCODE_PAGEDOWN, 0, 0 }, /* Key PageDown */
	{ SCVM_CTRL_KEY, Common::KEYCODE_UP, 0, 0 }, /* Key Up */
	{ SCVM_CTRL_KEY, Common::KEYCODE_LEFT, 0, 0 }, /* Key Left */
	{ SCVM_CTRL_KEY, Common::KEYCODE_DOWN, 0, 0 }, /* Key Down */
	{ SCVM_CTRL_KEY, Common::KEYCODE_RIGHT, 0, 0 }, /* Key Right */
	{ SCVM_CTRL_KEY, Common::KEYCODE_NUMLOCK, 0, 0 }, /* Key NumLock */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP_DIVIDE, 47, 47 }, /* Key KeyPadDivide */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP_MULTIPLY, 42, 42 }, /* Key KeyPadMultiply */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP_MINUS, 45, 45 }, /* Key KeyPadMinus */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP7, 55, 55 }, /* Key KeyPad7 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP8, 56, 56 }, /* Key KeyPad8 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP9, 57, 57 }, /* Key KeyPad9 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP_PLUS, 43, 43 }, /* Key KeyPadPlus */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP4, 52, 52 }, /* Key KeyPad4 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP5, 53, 53 }, /* Key KeyPad5 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP6, 54, 54 }, /* Key KeyPad6 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP1, 49, 49 }, /* Key KeyPad1 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP2, 50, 50 }, /* Key KeyPad2 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP3, 51, 51 }, /* Key KeyPad3 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP_ENTER, 0, 0 }, /* Key KeyPadEnter */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP0, 48, 48 }, /* Key KeyPad0 */
	{ SCVM_CTRL_KEY, Common::KEYCODE_KP_PERIOD, 46, 46 }, /* Key KeyPadPeriod */
};

#endif
