#!/usr/bin/env python3
"""Writes what the frontend is told about this core, and the tables the code
shares with it, from one list of the controls:

  waterbox.config          the machine: video, sound, settings, firmware,
                           the controls, their letters and headers
  default_keybinds.json    the keyboard to the keyboard, the mouse to the mouse
  backend/controls.h       what each control sends to ScummVM (the core)
  input-names.h            the controls' tokens (the test harness)
  languages/en.json        the English of every key named here (merged with
                           the keys the code names, which are kept)

usage: gen-config.py
"""
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

# ---- the texts a person reads ----------------------------------------------
TEXTS = {}


def text(key, english):
    if TEXTS.setdefault(key, english) != english:
        sys.exit("%s has two texts: %r and %r" % (key, TEXTS[key], english))
    return key


# ---- the controls -----------------------------------------------------------
# (name in waterbox.config, kind, ScummVM's code, the character the key types
#  and the one it types with Shift (printable ones only: the rest are
#  ScummVM's SDL backend's rule, chimera-machine.cpp's keyAscii), movie letter,
#  header, what Chimera sends for it by default)
MOUSE, WHEEL, KEY = "SCVM_CTRL_MOUSE", "SCVM_CTRL_WHEEL", "SCVM_CTRL_KEY"


def k(name, code, ascii=0, shifted=None, letter=None, header=None, bind=None):
    if shifted is None:
        shifted = ascii
    return (name, KEY, code, ascii, shifted, letter, header, bind)


def ch(c):
    return ord(c) if c else 0


CONTROLS = [
    ("Mouse Left Button", MOUSE, "0", 0, 0, "l", "ML", "WMouse L"),
    ("Mouse Right Button", MOUSE, "1", 0, 0, "r", "MR", "WMouse R"),
    ("Mouse Middle Button", MOUSE, "2", 0, 0, "m", "MM", "WMouse M"),
    ("Mouse Wheel Up", WHEEL, "0", 0, 0, "u", "WU", ""),
    ("Mouse Wheel Down", WHEEL, "1", 0, 0, "d", "WD", ""),
    k("Key Escape", "KEYCODE_ESCAPE", letter="e", header="Esc", bind="Escape"),
]
for n in range(1, 13):
    CONTROLS.append(k("Key F%d" % n, "KEYCODE_F%d" % n, letter="%d" % (n % 10), header="F%d" % n, bind="F%d" % n))
CONTROLS += [
    k("Key PrintScreen", "KEYCODE_PRINT", letter="p", header="PrSc", bind="PrintScreen"),
    k("Key ScrollLock", "KEYCODE_SCROLLOCK", letter="S", header="ScrL", bind="ScrollLock"),
    k("Key Pause", "KEYCODE_PAUSE", letter="P", header="Paus", bind="Pause"),
    k("Key Grave", "KEYCODE_BACKQUOTE", ch("`"), ch("~"), letter="`", header="`", bind="Backtick"),
]
for d, s in zip("1234567890", "!@#$%^&*()"):
    CONTROLS.append(k("Key " + d, "KEYCODE_" + d, ch(d), ch(s), letter=d, header=d, bind="Number" + d))
CONTROLS += [
    k("Key Minus", "KEYCODE_MINUS", ch("-"), ch("_"), letter="-", header="-", bind="Minus"),
    k("Key Equals", "KEYCODE_EQUALS", ch("="), ch("+"), letter="=", header="=", bind="Equals"),
    k("Key Backspace", "KEYCODE_BACKSPACE", letter="b", header="Bksp", bind="Backspace"),
    k("Key Tab", "KEYCODE_TAB", letter="t", header="Tab", bind="Tab"),
]
ROWS = ["QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"]
PUNCT_AFTER = {
    "P": [k("Key LeftBracket", "KEYCODE_LEFTBRACKET", ch("["), ch("{"), letter="[", header="[", bind="LeftBracket"),
          k("Key RightBracket", "KEYCODE_RIGHTBRACKET", ch("]"), ch("}"), letter="]", header="]", bind="RightBracket"),
          k("Key Backslash", "KEYCODE_BACKSLASH", ch("\\"), ch("|"), letter="\\", header="\\", bind="Backslash"),
          k("Key CapsLock", "KEYCODE_CAPSLOCK", letter="C", header="Caps", bind="CapsLock")],
    "L": [k("Key Semicolon", "KEYCODE_SEMICOLON", ch(";"), ch(":"), letter=";", header=";", bind="Semicolon"),
          k("Key Quote", "KEYCODE_QUOTE", ch("'"), ch('"'), letter="'", header="'", bind="Apostrophe"),
          k("Key Enter", "KEYCODE_RETURN", letter="e", header="Ent", bind="Enter"),
          k("Key LeftShift", "KEYCODE_LSHIFT", letter="s", header="LSh", bind="Shift, LeftShift")],
    "M": [k("Key Comma", "KEYCODE_COMMA", ch(","), ch("<"), letter=",", header=",", bind="Comma"),
          k("Key Period", "KEYCODE_PERIOD", ch("."), ch(">"), letter="p", header=".", bind="Period"),
          k("Key Slash", "KEYCODE_SLASH", ch("/"), ch("?"), letter="/", header="/", bind="Slash"),
          k("Key RightShift", "KEYCODE_RSHIFT", letter="s", header="RSh", bind="RightShift")],
}
for row in ROWS:
    for c in row:
        CONTROLS.append(k("Key " + c, "KEYCODE_" + c.lower(), ch(c.lower()), ch(c), letter=c, header=c, bind=c))
    CONTROLS += PUNCT_AFTER[row[-1]]
CONTROLS += [
    k("Key LeftCtrl", "KEYCODE_LCTRL", letter="c", header="LCtl", bind="Ctrl, LeftCtrl"),
    k("Key LeftAlt", "KEYCODE_LALT", letter="a", header="LAlt", bind="Alt, LeftAlt"),
    k("Key Space", "KEYCODE_SPACE", ch(" "), letter="s", header="Spc", bind="Space"),
    k("Key RightAlt", "KEYCODE_RALT", letter="a", header="RAlt", bind="RightAlt"),
    k("Key RightCtrl", "KEYCODE_RCTRL", letter="c", header="RCtl", bind="RightCtrl"),
    k("Key Insert", "KEYCODE_INSERT", letter="i", header="Ins", bind="Insert"),
    k("Key Home", "KEYCODE_HOME", letter="h", header="Home", bind="Home"),
    k("Key PageUp", "KEYCODE_PAGEUP", letter="p", header="PgUp", bind="PageUp"),
    k("Key Delete", "KEYCODE_DELETE", letter="d", header="Del", bind="Delete"),
    k("Key End", "KEYCODE_END", letter="e", header="End", bind="End"),
    k("Key PageDown", "KEYCODE_PAGEDOWN", letter="p", header="PgDn", bind="PageDown"),
    k("Key Up", "KEYCODE_UP", letter="^", header="Up", bind="Up"),
    k("Key Left", "KEYCODE_LEFT", letter="<", header="Left", bind="Left"),
    k("Key Down", "KEYCODE_DOWN", letter="v", header="Down", bind="Down"),
    k("Key Right", "KEYCODE_RIGHT", letter=">", header="Rght", bind="Right"),
    k("Key NumLock", "KEYCODE_NUMLOCK", letter="N", header="NumL", bind="NumLock"),
    k("Key KeyPadDivide", "KEYCODE_KP_DIVIDE", ch("/"), letter="/", header="KP/", bind="KeypadDivide"),
    k("Key KeyPadMultiply", "KEYCODE_KP_MULTIPLY", ch("*"), letter="*", header="KP*", bind="KeypadMultiply"),
    k("Key KeyPadMinus", "KEYCODE_KP_MINUS", ch("-"), letter="-", header="KP-", bind="KeypadSubtract"),
]
for d in "789":
    CONTROLS.append(k("Key KeyPad" + d, "KEYCODE_KP" + d, ch(d), letter=d, header="KP" + d, bind="Keypad" + d))
CONTROLS.append(k("Key KeyPadPlus", "KEYCODE_KP_PLUS", ch("+"), letter="+", header="KP+", bind="KeypadAdd"))
for d in "456123":
    CONTROLS.append(k("Key KeyPad" + d, "KEYCODE_KP" + d, ch(d), letter=d, header="KP" + d, bind="Keypad" + d))
CONTROLS += [
    k("Key KeyPadEnter", "KEYCODE_KP_ENTER", letter="e", header="KPEn", bind="KeypadEnter"),
    k("Key KeyPad0", "KEYCODE_KP0", ch("0"), letter="0", header="KP0", bind="Keypad0"),
    k("Key KeyPadPeriod", "KEYCODE_KP_PERIOD", ch("."), letter="p", header="KP.", bind="KeypadDecimal"),
]

AXES = [
    {"name": "Mouse Position X", "min": 0, "max": 65535, "neutral": 32768, "header": "MPX"},
    {"name": "Mouse Position Y", "min": 0, "max": 65535, "neutral": 32768, "header": "MPY"},
]
AXIS_BINDS = {"Mouse Position X": "WMouse X", "Mouse Position Y": "WMouse Y"}

MODIFIER_INDEX = {"Key LeftShift": "SCVM_KEY_LSHIFT", "Key RightShift": "SCVM_KEY_RSHIFT",
                  "Key LeftCtrl": "SCVM_KEY_LCTRL", "Key RightCtrl": "SCVM_KEY_RCTRL",
                  "Key LeftAlt": "SCVM_KEY_LALT", "Key RightAlt": "SCVM_KEY_RALT"}

# ---- the settings and firmware -------------------------------------------------
MACHINE = "It is part of the machine, so a movie needs the same value."
SETTINGS = [
    {"name": "musicDriver", "display": text("setting.musicdriver", "Music device"),
     "type": "enum",
     "options": ["auto", "adlib", "pcspk", "pcjr", "cms", "mt32", "cm32l", "null"],
     "optionLabels": {
         "auto": text("setting.musicdriver.auto", "As the game prefers"),
         "adlib": text("setting.musicdriver.adlib", "AdLib (FM synthesis)"),
         "pcspk": text("setting.musicdriver.pcspk", "PC speaker"),
         "pcjr": text("setting.musicdriver.pcjr", "IBM PCjr"),
         "cms": text("setting.musicdriver.cms", "Creative Music System"),
         "mt32": text("setting.musicdriver.mt32", "Roland MT-32 (needs its ROMs)"),
         "cm32l": text("setting.musicdriver.cm32l", "Roland CM-32L (needs its ROMs)"),
         "null": text("setting.musicdriver.null", "No music"),
     },
     "default": "auto",
     "description": text("setting.musicdriver.about",
                         "The device ScummVM plays the game's music on, for games that can use more than one. "
                         "The Roland devices are emulated and need their ROM files. " + MACHINE)},
    {"name": "oplEmulator", "display": text("setting.oplemulator", "AdLib emulator"),
     "type": "enum",
     "options": ["auto", "mame", "db", "nuked"],
     "optionLabels": {
         "auto": text("setting.oplemulator.auto", "ScummVM's default"),
         "mame": text("setting.oplemulator.mame", "MAME OPL"),
         "db": text("setting.oplemulator.db", "DOSBox OPL"),
         "nuked": text("setting.oplemulator.nuked", "Nuked OPL"),
     },
     "default": "auto",
     "description": text("setting.oplemulator.about",
                         "Which emulation of the AdLib sound chip plays AdLib music and sound. " + MACHINE)},
    {"name": "subtitles", "display": text("setting.subtitles", "Subtitles"),
     "type": "bool", "default": True,
     "description": text("setting.subtitles.about",
                          "Show the spoken lines as text, in games that have both speech and text. " + MACHINE)},
    {"name": "renderer", "display": text("setting.renderer", "3D renderer"),
     "type": "enum",
     "options": ["software", "opengl", "opengl_shaders"],
     "optionLabels": {
         "software": text("setting.renderer.software", "Software (TinyGL)"),
         "opengl": text("setting.renderer.opengl", "OpenGL"),
         "opengl_shaders": text("setting.renderer.opengl_shaders", "OpenGL with shaders"),
     },
     "default": "software",
     "description": text("setting.renderer.about",
                         "How games drawn in 3D are drawn, the same choice as in ScummVM's own options. "
                         "'Software (TinyGL)' is ScummVM's own software renderer, for every engine that has one "
                         "(Grim Fandango, Myst III, The Longest Journey, Syberia, Penumbra and others). 'OpenGL' and "
                         "'OpenGL with shaders' have the engines draw with their OpenGL renderers instead, on an "
                         "OpenGL built into the core (Mesa's softpipe): the same picture on every computer, and "
                         "slower. An engine that lacks the renderer chosen takes the nearest it has. The Colony and "
                         "Thimbleweed Park, which have no software renderer, draw with the OpenGL in the core "
                         "whichever is chosen. Games in 2D are not affected. " + MACHINE)},
]

FIRMWARE = [
    {"id": "MT32_CONTROL.ROM", "display": text("firmware.mt32_control", "Roland MT-32 control ROM"),
     "description": text("firmware.mt32_control.about",
                         "The MT-32's program ROM, version 1.07. ScummVM's MT-32 emulation needs it."),
     "size": 65536, "name": "MT32_CONTROL.ROM", "label": text("firmware.mt32_control.label", "MT-32 control ROM (1.07)"),
     "requiredWhen": {"setting": "musicDriver", "is": "mt32"}, "sha1": "B083518FFFB7F66B03C23B7EB4F868E62DC5A987"},
    {"id": "MT32_PCM.ROM", "display": text("firmware.mt32_pcm", "Roland MT-32 PCM ROM"),
     "description": text("firmware.mt32_pcm.about", "The MT-32's sound samples. ScummVM's MT-32 emulation needs them."),
     "size": 524288, "name": "MT32_PCM.ROM", "label": text("firmware.mt32_pcm.label", "MT-32 PCM ROM"),
     "requiredWhen": {"setting": "musicDriver", "is": "mt32"}, "sha1": "F6B1EEBC4B2D200EC6D3D21D51325D5B48C60252"},
    {"id": "CM32L_CONTROL.ROM", "display": text("firmware.cm32l_control", "Roland CM-32L control ROM"),
     "description": text("firmware.cm32l_control.about",
                         "The CM-32L's program ROM, version 1.02. ScummVM's CM-32L emulation needs it."),
     "size": 65536, "name": "CM32L_CONTROL.ROM", "label": text("firmware.cm32l_control.label", "CM-32L control ROM (1.02)"),
     "requiredWhen": {"setting": "musicDriver", "is": "cm32l"}, "sha1": "A439FBB390DA38CADA95A7CBB1D6CA199CD66EF8"},
    {"id": "CM32L_PCM.ROM", "display": text("firmware.cm32l_pcm", "Roland CM-32L PCM ROM"),
     "description": text("firmware.cm32l_pcm.about", "The CM-32L's sound samples. ScummVM's CM-32L emulation needs them."),
     "size": 1048576, "name": "CM32L_PCM.ROM", "label": text("firmware.cm32l_pcm.label", "CM-32L PCM ROM"),
     "requiredWhen": {"setting": "musicDriver", "is": "cm32l"}, "sha1": "289CC298AD532B702461BFC738009D9EBE8025EA"},
]


def write_json(path, obj):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(obj, f, indent=2, ensure_ascii=False)
        f.write("\n")


def main():
    names = [c[0] for c in CONTROLS]
    if len(set(names)) != len(names):
        sys.exit("a control is listed twice")
    config = {
        "coreName": "ScummVM",
        "systemId": "ScummVM",
        "systemNames": {"ScummVM": "ScummVM"},
        "author": "The ScummVM Team; chimera core by Sergio Martin",
        "url": "https://github.com/scummvm/scummvm",
        "runsOn": ["windows-x86_64", "linux-x86_64", "macos-x86_64"],
        "romFile": "game",
        "extensions": {".zip": "ScummVM"},
        "_comment_extensions": "A ScummVM game is a folder of files, given as a zip of the folder. ScummVM's own detector finds the game in it.",
        "deterministic": True,
        "memoryLayoutMiB": [512, 16, 16, 64, 3072],
        "_memoryLayoutMiB_note": "sbrk, sealed, invisible, plain, mmap. ScummVM's engines load what they need of the game into memory (musl gives large allocations to mmap); the zip itself is read where Chimera mounted it. ScummVM's own stack (16 MiB) is mmap'd.",
        "video": {
            "_comment": "The game's screen at its own resolution (320x200 to the largest an engine draws), or ScummVM's menus over it. Capacity here is the largest allowed; GetVideoWidth/Height give the live size and GetDisplayAspectX/Y the shape (4:3 for games of 200 or 400 lines).",
            "width": 2560,
            "height": 1600,
            "getBgra": "GetVideoBgra",
            "vsyncNumerator": 60,
            "vsyncDenominator": 1,
        },
        "audio": {
            "_comment": "ScummVM's mixer, run for exactly one frame's share of a second each frame: 735 stereo samples at 44100 Hz.",
            "sampleRate": 44100,
            "channels": 2,
            "samplesPerFrame": 735,
            "get": "GetAudio",
        },
        "lag": {"inputWasRead": "InputWasRead"},
        "settings": SETTINGS,
        "firmware": FIRMWARE,
        "input": {
            "name": "ScummVM",
            "_comment": "A PC's keyboard and mouse, as ScummVM's games expect them. The mouse's place is two absolute axes, 0 to 65535 across and down whatever is on the screen (the game, or ScummVM's menus over it); the pointer moves when they change. The wheel's two controls each turn it one step when pressed.",
            "buttons": names,
            "mnemonics": {c[0]: c[5] for c in CONTROLS},
            "headers": {c[0]: c[6] for c in CONTROLS},
            "axes": AXES,
        },
    }
    write_json(os.path.join(HERE, "waterbox.config"), config)

    write_json(os.path.join(HERE, "default_keybinds.json"), {
        "_comment": ["The keyboard binds to the host's same key, and the mouse to the host's mouse: its place in",
                     "the window to the two position axes, its buttons to the buttons. The wheel ships unbound."],
        "AllTrollers": {"ScummVM": {c[0]: c[7] or "" for c in CONTROLS}},
        "AllTrollersAutoFire": {"ScummVM": {}},
        "AllTrollersAnalog": {"ScummVM": {a["name"]: {"Value": AXIS_BINDS[a["name"]], "Mult": 1.0, "Deadzone": 0.0}
                                          for a in AXES}},
    })

    with open(os.path.join(HERE, "backend", "controls.h"), "w") as f:
        f.write("/* controls.h - written by gen-config.py: what each control of waterbox.config\n"
                " * sends to ScummVM, in waterbox.config's order. */\n"
                "#ifndef CHIMERA_CONTROLS_H\n#define CHIMERA_CONTROLS_H\n\n"
                "enum { SCVM_CTRL_MOUSE, SCVM_CTRL_WHEEL, SCVM_CTRL_KEY };\n"
                "struct scvm_control { int kind; int code; int ascii; int shifted; };\n\n")
        f.write("#define SCVM_BTN_COUNT %d\n#define SCVM_AXIS_COUNT %d\n" % (len(CONTROLS), len(AXES)))
        for i, c in enumerate(CONTROLS):
            if c[0] in MODIFIER_INDEX:
                f.write("#define %s %d\n" % (MODIFIER_INDEX[c[0]], i))
        f.write("\nstatic const struct scvm_control scvm_controls[SCVM_BTN_COUNT] = {\n")
        for c in CONTROLS:
            code = c[2] if c[1] != KEY else "Common::" + c[2]
            f.write("\t{ %s, %s, %d, %d }, /* %s */\n" % (c[1], code, c[3], c[4], c[0]))
        f.write("};\n\n#endif\n")

    with open(os.path.join(HERE, "input-names.h"), "w") as f:
        f.write("/* input-names.h - written by gen-config.py: the controls in waterbox.config's\n"
                " * order, for the test harness. A movie line names what is held by these\n"
                " * tokens (the config's names without spaces), and the mouse's place as\n"
                " * X=<0..65535> and Y=<0..65535>. */\n"
                "#define GATE_BTN_COUNT %d\n#define GATE_AXIS_COUNT %d\n"
                "static const char *const gate_button_tokens[GATE_BTN_COUNT] = {\n" % (len(CONTROLS), len(AXES)))
        for c in CONTROLS:
            f.write('\t"%s",\n' % c[0].replace(" ", ""))
        f.write("};\n")

    en = os.path.join(HERE, "languages", "en.json")
    table = json.load(open(en, encoding="utf-8")) if os.path.exists(en) else {"texts": {}}
    table["texts"].update(TEXTS)
    table["texts"] = dict(sorted(table["texts"].items()))
    os.makedirs(os.path.dirname(en), exist_ok=True)
    with open(en, "w", encoding="utf-8") as f:
        json.dump(table, f, indent="\t", ensure_ascii=False)
        f.write("\n")


if __name__ == "__main__":
    main()
