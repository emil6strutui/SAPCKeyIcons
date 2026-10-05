#pragma once

#include <CSprite2d.h>
#include <CRGBA.h>

namespace ButtonIcons {

// Inline icon heights in font-relative units; width follows the texture aspect.
constexpr float ICON_SIZE = 13.0f;
constexpr float MENU_ICON_SIZE = 17.0f;

// ============================================================================
// KEYBOARD KEY INDICES
// Keyboard token indices (84 keys)
// ============================================================================

enum KeyboardKey {
    // Movement keys (0-3)
    KEY_W = 0,
    KEY_A,
    KEY_S,
    KEY_D,

    // Arrow keys (4-7)
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,

    // Letter keys (8-29)
    KEY_E, KEY_Q, KEY_F, KEY_G, KEY_H,
    KEY_N, KEY_Y, KEY_X, KEY_Z, KEY_V, KEY_C,
    KEY_B, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
    KEY_O, KEY_P, KEY_R, KEY_T, KEY_U,

    // Numpad digits (30-40)
    KEY_NUM0, KEY_NUM1, KEY_NUM2, KEY_NUM3, KEY_NUM4,
    KEY_NUM5, KEY_NUM6, KEY_NUM7, KEY_NUM8, KEY_NUM9,
    KEY_NUMDOT,

    // Modifiers (41-44)
    KEY_LCTRL, KEY_RCTRL, KEY_LSHIFT, KEY_LALT,

    // Special keys (45-54)
    KEY_SPACE, KEY_TAB, KEY_CAPSLOCK,
    KEY_DEL, KEY_HOME, KEY_END,
    KEY_PGUP, KEY_PGDN, KEY_RETURN, KEY_ENT,

    // Number row keys (55-64)
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    // Numpad operators (65-67)
    KEY_NUMPLUS, KEY_NUMMUL, KEY_NUMMINUS,

    // Insert key (68)
    KEY_INS,

    // Function keys (69-80)
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
    KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,

    // Append new keys so existing ~K00~ through ~K80~ tokens stay compatible.
    KEY_BACKSPACE, KEY_RSHIFT, KEY_ESC,

    KEYBOARD_COUNT  // = 84
};

// ============================================================================
// MOUSE BUTTON INDICES
// Mouse token indices; sprites are placed after the keyboard sprites.
// ============================================================================

enum MouseButton : int {
    MOUSE_LMB = 0,
    MOUSE_RMB = 1,
    MOUSE_MMB = 2,
    MOUSE_WHEEL_UP = 3,
    MOUSE_WHEEL_DOWN = 4,
    MOUSE_BUTTON_4 = 5,
    MOUSE_BUTTON_5 = 6,
    MOUSE_WHEEL = 7,  // Either wheel direction, as named by menu helper text
    MOUSE_COUNT = 8
};

// ============================================================================
// PUBLIC API
// ============================================================================

void InstallHooks();
bool IsEnabled();
void SetEnabled(bool enabled);
void ReloadTextures();

// ============================================================================
// DIRECT DRAWING API
// ============================================================================

void DrawIcon(MouseButton button, float x, float y, float size);
void DrawIconColored(MouseButton button, float x, float y, float size,
                     unsigned char r, unsigned char g, unsigned char b, unsigned char a);

} // namespace ButtonIcons
