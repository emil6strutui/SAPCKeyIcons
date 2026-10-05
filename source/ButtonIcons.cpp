#include "ButtonIcons.h"

#include <plugin.h>
#include <Events.h>
#include <CFont.h>
#include <CMenuManager.h>
#include <CSprite2d.h>
#include <CTxdStore.h>
#include <RenderWare.h>
#include <extensions/Paths.h>

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <string>

using namespace plugin;

namespace ButtonIcons {

// ============================================================================
// GINPUT COMPATIBILITY
// ============================================================================

static bool g_GInputLoaded = false;
static CSprite2d* g_SpriteArray = nullptr;
static float* g_SpriteWidths = nullptr;

static bool IsGInputLoaded() {
    return GetModuleHandleA("GInputSA.asi") != nullptr;
}

// ============================================================================
// EXTENDED SPRITE ARRAY
// ============================================================================

static const int MAX_EXTENDED_SPRITES = 150;
static CSprite2d g_ExtendedSprites[MAX_EXTENDED_SPRITES];
static float g_ExtendedSpriteWidths[MAX_EXTENDED_SPRITES];
static float g_SymbolSpriteWidth = 17.0f;
static unsigned int g_TokenWidth = 3;

static int KEYBOARD_SPRITE_BASE = 15;
static int MOUSE_SPRITE_BASE = 15 + KEYBOARD_COUNT;

static const int GINPUT_KEYBOARD_SPRITE_BASE = 50;
static const int GINPUT_MOUSE_SPRITE_BASE = 50 + KEYBOARD_COUNT;

static_assert(KEYBOARD_COUNT <= 100, "Keyboard tokens have two index digits");
static_assert(MOUSE_COUNT <= 10, "Mouse tokens have one significant index digit");
static_assert(GINPUT_MOUSE_SPRITE_BASE + MOUSE_COUNT <= MAX_EXTENDED_SPRITES,
              "Button sprites must fit alongside GInput sprites");

// ============================================================================
// SPRITE NAMES
// ============================================================================

static const char* const g_KeyboardSpriteNames[KEYBOARD_COUNT] = {
    "W", "A", "S", "D",
    "38", "40", "37", "39",
    "E", "Q", "F", "G", "H",
    "N", "Y", "X", "Z", "V", "C",
    "B", "I", "J", "K", "L", "M",
    "O", "P", "R", "T", "U",
    "96", "97", "98", "99",
    "100", "101", "102", "103",
    "104", "105", "110",
    "162", "163", "160", "164",
    "32", "9", "20",
    "46", "36", "35",
    "33", "34", "13", "padenter",
    "48", "49", "50", "51", "52",
    "53", "54", "55", "56", "57",
    "107", "106", "109",
    // Insert key (68)
    "45",
    // Function keys F1-F12 (69-80)
    "112", "113", "114", "115", "116", "117",
    "118", "119", "120", "121", "122", "123",
    // Backspace, right Shift, and Esc (81-83)
    "8", "161", "27"
};

static const char* const g_MouseSpriteNames[MOUSE_COUNT] = {
    "1", "2", "4",
    "MWHU", "MWHD",
    "5", "6",
    "MWH"
};

// ============================================================================
// INTERNAL STATE
// ============================================================================

static bool g_Enabled = true;
static bool g_TexturesLoaded = false;
static int g_TxdSlot = -1;

// ============================================================================
// RSKEYCODES
// ============================================================================

enum RsKeyCodes : int {
    rsNULL = 1056, rsESC = 1000,
    rsUP = 1019, rsDOWN = 1020, rsLEFT = 1021, rsRIGHT = 1022,
    rsPADINS = 1038, rsPADEND = 1028, rsPADDOWN = 1029, rsPADPGDN = 1030,
    rsPADLEFT = 1031, rsPAD5 = 1032, rsPADRIGHT = 1034,
    rsPADHOME = 1035, rsPADUP = 1036, rsPADPGUP = 1037, rsPADDEL = 1027,
    rsLCTRL = 1049, rsRCTRL = 1050, rsLSHIFT = 1046, rsRSHIFT = 1047, rsLALT = 1051,
    rsBACKSP = 1042,
    rsTAB = 1043, rsCAPSLK = 1044, rsDEL = 1014,
    rsHOME = 1015, rsEND = 1016, rsPGUP = 1017, rsPGDN = 1018,
    rsENTER = 1045, rsPADENTER = 1039, rsINS = 1013,
    rsMOUSE_LEFT_BUTTON = 1, rsMOUSE_MIDDLE_BUTTON = 2, rsMOUSE_RIGHT_BUTTON = 3,
    rsMOUSE_WHEEL_UP_BUTTON = 4, rsMOUSE_WHEEL_DOWN_BUTTON = 5,
    rsMOUSE_X1_BUTTON = 6, rsMOUSE_X2_BUTTON = 7,
};

// ============================================================================
// ARRAY EXPANSION
// ============================================================================

static void ExpandButtonSpriteArray() {
    g_GInputLoaded = IsGInputLoaded();

    if (g_GInputLoaded) {
        KEYBOARD_SPRITE_BASE = GINPUT_KEYBOARD_SPRITE_BASE;
        MOUSE_SPRITE_BASE = GINPUT_MOUSE_SPRITE_BASE;

        CSprite2d* ginputArray = *reinterpret_cast<CSprite2d**>(0x718AE1);
        if (ginputArray) {
            memcpy(g_ExtendedSprites, ginputArray, 50 * sizeof(CSprite2d));
        }

        for (int i = 0; i < MAX_EXTENDED_SPRITES; i++) {
            g_ExtendedSpriteWidths[i] = 17.0f;
        }

        g_SpriteArray = g_ExtendedSprites;
        g_SpriteWidths = g_ExtendedSpriteWidths;
        patch::SetPointer(0x718AE1, g_ExtendedSprites);

    } else {
        KEYBOARD_SPRITE_BASE = 15;
        MOUSE_SPRITE_BASE = 15 + KEYBOARD_COUNT;

        CSprite2d* originalArray = reinterpret_cast<CSprite2d*>(0xC71AD8);
        memcpy(g_ExtendedSprites, originalArray, 15 * sizeof(CSprite2d));

        float* originalWidths = reinterpret_cast<float*>(0xC71A90);
        memcpy(g_ExtendedSpriteWidths, originalWidths, 15 * sizeof(float));

        for (int i = 15; i < MAX_EXTENDED_SPRITES; i++) {
            g_ExtendedSpriteWidths[i] = 17.0f;
        }

        g_SpriteArray = g_ExtendedSprites;
        g_SpriteWidths = g_ExtendedSpriteWidths;
        patch::SetPointer(0x718AE1, g_ExtendedSprites);
    }
}

// ============================================================================
// TEXTURE LOADING
// ============================================================================

static void DeleteButtonTextures() {
    for (int i = KEYBOARD_SPRITE_BASE; i < MOUSE_SPRITE_BASE + MOUSE_COUNT; i++) {
        if (g_SpriteArray[i].m_pTexture) {
            g_SpriteArray[i].Delete();
        }
    }
}

static bool TryLoadTextures(const char* path) {
    // The filename overload of LoadTxd retries forever when opening fails.
    // Open once ourselves so a missing or unreadable candidate can fall back.
    RwStream* stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
    if (!stream) return false;

    const int slot = CTxdStore::AddTxdSlot("buttonicons");
    if (slot == -1) {
        RwStreamClose(stream, nullptr);
        return false;
    }

    const bool loaded = CTxdStore::LoadTxd(slot, stream);
    RwStreamClose(stream, nullptr);
    if (!loaded) {
        CTxdStore::RemoveTxdSlot(slot);
        return false;
    }

    CTxdStore::PushCurrentTxd();
    CTxdStore::SetCurrentTxd(slot);

    for (int i = 0; i < KEYBOARD_COUNT; i++) {
        int idx = KEYBOARD_SPRITE_BASE + i;
        g_SpriteArray[idx].SetTexture(const_cast<char*>(g_KeyboardSpriteNames[i]));
    }

    for (int i = 0; i < MOUSE_COUNT; i++) {
        int idx = MOUSE_SPRITE_BASE + i;
        g_SpriteArray[idx].SetTexture(const_cast<char*>(g_MouseSpriteNames[i]));
    }

    bool complete = true;
    for (int i = KEYBOARD_SPRITE_BASE; i < MOUSE_SPRITE_BASE + MOUSE_COUNT; i++) {
        RwTexture* tex = g_SpriteArray[i].m_pTexture;
        RwRaster* raster = tex ? RwTextureGetRaster(tex) : nullptr;
        if (!raster || RwRasterGetWidth(raster) <= 0 || RwRasterGetHeight(raster) <= 0) {
            complete = false;
            continue;
        }
        const float aspectRatio = static_cast<float>(RwRasterGetWidth(raster)) /
                                  static_cast<float>(RwRasterGetHeight(raster));
        g_ExtendedSpriteWidths[i] = ICON_SIZE * aspectRatio;
    }

    CTxdStore::PopCurrentTxd();

    // The stock pcbtns.txd lacks our icons. Reject incomplete dictionaries too,
    // releasing bound textures before their owning dictionary is removed.
    if (!complete) {
        DeleteButtonTextures();
        CTxdStore::RemoveTxdSlot(slot);
        return false;
    }

    CTxdStore::AddRef(slot);
    g_TxdSlot = slot;
    g_TexturesLoaded = true;
    return true;
}

static void LoadTextures() {
    if (g_TexturesLoaded || !g_SpriteArray) return;

    // Resolve from the actual ASI location, not the process working directory.
    // Copy SDK helper results because their returned buffers are static.
    const std::string pluginPath = paths::GetPluginDirRelativePathA("models\\pcbtns.txd");
    const std::string gamePath = paths::GetGameDirRelativePathA("models\\pcbtns.txd");
    if (TryLoadTextures(pluginPath.c_str())) return;

    // Preserve legacy game-root installs without retrying the same path twice.
    if (_stricmp(pluginPath.c_str(), gamePath.c_str()) != 0) {
        TryLoadTextures(gamePath.c_str());
    }
}

static void UnloadTextures() {
    if (!g_TexturesLoaded) return;
    if (!g_SpriteArray) return;

    DeleteButtonTextures();

    if (g_TxdSlot != -1) {
        CTxdStore::RemoveTxdSlot(g_TxdSlot);
        g_TxdSlot = -1;
    }
    g_TexturesLoaded = false;
}

// ============================================================================
// KEY CODE TO SPRITE TOKEN MAPPING
// ============================================================================

static const char* GetSpriteTokenForKeyCode(unsigned int keyCode) {
    switch (keyCode) {
        case 'W': case 'w': return "~K00~";
        case 'A': case 'a': return "~K01~";
        case 'S': case 's': return "~K02~";
        case 'D': case 'd': return "~K03~";
        case 'E': case 'e': return "~K08~";
        case 'Q': case 'q': return "~K09~";
        case 'F': case 'f': return "~K10~";
        case 'G': case 'g': return "~K11~";
        case 'H': case 'h': return "~K12~";
        case 'N': case 'n': return "~K13~";
        case 'Y': case 'y': return "~K14~";
        case 'X': case 'x': return "~K15~";
        case 'Z': case 'z': return "~K16~";
        case 'V': case 'v': return "~K17~";
        case 'C': case 'c': return "~K18~";
        case 'B': case 'b': return "~K19~";
        case 'I': case 'i': return "~K20~";
        case 'J': case 'j': return "~K21~";
        case 'K': case 'k': return "~K22~";
        case 'L': case 'l': return "~K23~";
        case 'M': case 'm': return "~K24~";
        case 'O': case 'o': return "~K25~";
        case 'P': case 'p': return "~K26~";
        case 'R': case 'r': return "~K27~";
        case 'T': case 't': return "~K28~";
        case 'U': case 'u': return "~K29~";
        case ' ': return "~K45~";
        case '0': return "~K55~";
        case '1': return "~K56~";
        case '2': return "~K57~";
        case '3': return "~K58~";
        case '4': return "~K59~";
        case '5': return "~K60~";
        case '6': return "~K61~";
        case '7': return "~K62~";
        case '8': return "~K63~";
        case '9': return "~K64~";
    }

    switch (keyCode) {
        case rsUP:    return "~K04~";
        case rsDOWN:  return "~K05~";
        case rsLEFT:  return "~K06~";
        case rsRIGHT: return "~K07~";
        case rsPADINS:   return "~K30~";
        case rsPADEND:   return "~K31~";
        case rsPADDOWN:  return "~K32~";
        case rsPADPGDN:  return "~K33~";
        case rsPADLEFT:  return "~K34~";
        case rsPAD5:     return "~K35~";
        case rsPADRIGHT: return "~K36~";
        case rsPADHOME:  return "~K37~";
        case rsPADUP:    return "~K38~";
        case rsPADPGUP:  return "~K39~";
        case rsPADDEL:   return "~K40~";
        case rsLCTRL:  return "~K41~";
        case rsRCTRL:  return "~K42~";
        case rsLSHIFT: return "~K43~";
        case rsLALT:   return "~K44~";
        case rsTAB:      return "~K46~";
        case rsCAPSLK:   return "~K47~";
        case rsDEL:      return "~K48~";
        case rsHOME:     return "~K49~";
        case rsEND:      return "~K50~";
        case rsPGUP:     return "~K51~";
        case rsPGDN:     return "~K52~";
        case rsENTER:    return "~K53~";
        case rsPADENTER: return "~K54~";
        case rsINS:      return "~K68~";
        case rsBACKSP:   return "~K81~";
        case rsRSHIFT:   return "~K82~";
        case rsESC:      return "~K83~";
        default: return nullptr;
    }
}

static const char* GetSpriteTokenForMouseCode(unsigned int mouseCode) {
    switch (mouseCode) {
        case rsMOUSE_LEFT_BUTTON:       return "~M00~";
        case rsMOUSE_RIGHT_BUTTON:      return "~M01~";
        case rsMOUSE_MIDDLE_BUTTON:     return "~M02~";
        case rsMOUSE_WHEEL_UP_BUTTON:   return "~M03~";
        case rsMOUSE_WHEEL_DOWN_BUTTON: return "~M04~";
        case rsMOUSE_X1_BUTTON:         return "~M05~";
        case rsMOUSE_X2_BUTTON:         return "~M06~";
        default: return nullptr;
    }
}

// ============================================================================
// TOKEN PARSING
// ============================================================================

static int ParseKeyboardToken(const char* text) {
    if (text[0] != '~' || text[1] != 'K' || text[4] != '~') return -1;
    char d1 = text[2], d2 = text[3];
    if (d1 >= '0' && d1 <= '9' && d2 >= '0' && d2 <= '9') {
        return (d1 - '0') * 10 + (d2 - '0');
    }
    return -1;
}

// ============================================================================
// GetControllerSettingTextKeyBoard HOOK (0x52FE10)
// ============================================================================

static float* g_FontScaleY = reinterpret_cast<float*>(0xC71A68);
static char* g_KeyNameBuffer = reinterpret_cast<char*>(0xB714BC);
static char* g_NumberBuffer = reinterpret_cast<char*>(0xB7149C);
static int* g_TextLanguage = reinterpret_cast<int*>(0xBA67C8);

static char* CText_Get(const char* key) {
    using CText_Get_t = char*(__thiscall*)(void*, const char*);
    static CText_Get_t CText_Get_Raw = reinterpret_cast<CText_Get_t>(0x6A0050);
    static void* TheText = reinterpret_cast<void*>(0xC1B340);
    return CText_Get_Raw(TheText, key);
}

using InsertNumberInString_t = void(__cdecl*)(char*, int, int, int, int, int, int, char*);
static InsertNumberInString_t CMessages_InsertNumberInString = reinterpret_cast<InsertNumberInString_t>(0x69DE90);

static char g_SpriteTokenBuffer[8];
static void* g_ControllerThis;

static char* __cdecl GetControllerSettingTextKeyBoard_Impl(int action, int type) {
    void* thisPtr = g_ControllerThis;

    memset(g_KeyNameBuffer, 0, 0x30);

    if (!thisPtr || thisPtr == reinterpret_cast<void*>(0xFFFFFFFF)) return nullptr;
    if (action < 0 || action > 58 || type < 0 || type > 3) return nullptr;

    unsigned char* basePtr = reinterpret_cast<unsigned char*>(thisPtr);
    unsigned int keyCode = *reinterpret_cast<unsigned int*>(basePtr + 0xB70 + action * 32 + type * 8);

    if (keyCode == 0 || keyCode == 1056) return nullptr;

    if (keyCode >= 0x100) {
        const char* token = GetSpriteTokenForKeyCode(keyCode);
        if (token && g_Enabled && g_TexturesLoaded) {
            strcpy_s(g_SpriteTokenBuffer, sizeof(g_SpriteTokenBuffer), token);
            return g_SpriteTokenBuffer;
        }

        // F1-F12 keys (0x3E9 = 1001 = F1, 0x3F4 = 1012 = F12)
        if (keyCode >= 0x3E9 && keyCode <= 0x3F4) {
            if (g_Enabled && g_TexturesLoaded) {
                // F1 = index 69, F12 = index 80
                int fKeyIndex = 69 + (keyCode - 0x3E9);
                sprintf_s(g_SpriteTokenBuffer, sizeof(g_SpriteTokenBuffer), "~K%02d~", fKeyIndex);
                return g_SpriteTokenBuffer;
            }
            // Fallback to text if icons not loaded
            char* fncText = CText_Get("FEC_FNC");
            if (fncText) {
                CMessages_InsertNumberInString(fncText, keyCode - 1000, -1, -1, -1, -1, -1, g_NumberBuffer);
                return g_NumberBuffer;
            }
            sprintf_s(g_KeyNameBuffer, 0x30, "F%d", keyCode - 1000);
            return g_KeyNameBuffer;
        }

        if (keyCode == 0x400 && g_Enabled && g_TexturesLoaded) { strcpy_s(g_SpriteTokenBuffer, sizeof(g_SpriteTokenBuffer), "~K66~"); return g_SpriteTokenBuffer; }
        if (keyCode == 0x401 && g_Enabled && g_TexturesLoaded) { strcpy_s(g_SpriteTokenBuffer, sizeof(g_SpriteTokenBuffer), "~K65~"); return g_SpriteTokenBuffer; }
        if (keyCode == 0x402 && g_Enabled && g_TexturesLoaded) { strcpy_s(g_SpriteTokenBuffer, sizeof(g_SpriteTokenBuffer), "~K67~"); return g_SpriteTokenBuffer; }

        switch (keyCode) {
            case 0x3F5: return CText_Get("FEC_IRT");
            case 0x3F6: return CText_Get("FEC_DLL");
            case 0x3F7: return CText_Get("FEC_HME");
            case 0x3F8: return CText_Get("FEC_END");
            case 0x3F9: return CText_Get("FEC_PGU");
            case 0x3FA: return CText_Get("FEC_PGD");
            case 0x3FB: return CText_Get("FEC_UPA");
            case 0x3FC: return CText_Get("FEC_DWA");
            case 0x3FD: return CText_Get("FEC_LFA");
            case 0x3FE: return CText_Get("FEC_RFA");
            case 0x3FF: return CText_Get("FEC_FWS");
            case 0x400: return CText_Get("FECSTAR");
            case 0x401: return CText_Get("FEC_PLS");
            case 0x402: return CText_Get("FEC_MIN");
            case 0x403: return CText_Get("FEC_DOT");
            case 0x404: case 0x405: case 0x406: case 0x407: case 0x408:
            case 0x40A: case 0x40B: case 0x40C: case 0x40D: case 0x40E: {
                char* nmn = CText_Get("FEC_NMN");
                if (nmn) {
                    int num = (keyCode == 0x40E) ? 0 : (keyCode - 0x403);
                    CMessages_InsertNumberInString(nmn, num, -1, -1, -1, -1, -1, g_NumberBuffer);
                    return g_NumberBuffer;
                }
                return nullptr;
            }
            case 0x409: return CText_Get("FEC_NLK");
            case 0x40F: return CText_Get("FEC_ETR");
            case 0x410: return CText_Get("FEC_SLK");
            case 0x411: return CText_Get("FEC_PSB");
            case 0x412: return CText_Get("FEC_BSP");
            case 0x413: return CText_Get("FEC_TAB");
            case 0x414: return CText_Get("FEC_CLK");
            case 0x415: return CText_Get("FEC_RTN");
            case 0x416: return CText_Get("FEC_LSF");
            case 0x417: return CText_Get("FEC_RSF");
            case 0x418: return CText_Get("FEC_SFT");
            case 0x419: return CText_Get("FEC_LCT");
            case 0x41A: return CText_Get("FEC_RCT");
            case 0x41B: return CText_Get("FEC_LAL");
            case 0x41C: return CText_Get("FEC_RAL");
            case 0x41D: return CText_Get("FEC_LWD");
            case 0x41E: return CText_Get("FEC_RWD");
            case 0x41F: return CText_Get("FEC_WRC");
            default: return nullptr;
        }
    }

    if (keyCode == '*') return CText_Get("FEC_AST");
    if (keyCode == '^' && *g_TextLanguage == 2) {
        g_KeyNameBuffer[0] = '|';
        g_KeyNameBuffer[1] = '\0';
        return g_KeyNameBuffer;
    }
    if (keyCode == 0xB2 && *g_TextLanguage == 1) {
        g_KeyNameBuffer[0] = '2';
        g_KeyNameBuffer[1] = '\0';
        return g_KeyNameBuffer;
    }

    const char* token = GetSpriteTokenForKeyCode(keyCode);
    if (token && g_Enabled && g_TexturesLoaded) {
        strcpy_s(g_SpriteTokenBuffer, sizeof(g_SpriteTokenBuffer), token);
        return g_SpriteTokenBuffer;
    }

    char charCode = static_cast<char>(keyCode);
    if (!charCode) charCode = '#';
    g_KeyNameBuffer[0] = charCode;
    g_KeyNameBuffer[1] = '\0';
    return g_KeyNameBuffer;
}

__declspec(naked) void GetControllerSettingTextKeyBoard_Thunk() {
    __asm {
        mov g_ControllerThis, ecx
        mov eax, [esp+8]
        push eax
        mov eax, [esp+8]
        push eax
        call GetControllerSettingTextKeyBoard_Impl
        add esp, 8
        ret 8
    }
}

// ============================================================================
// GetControllerSettingTextMouse HOOK (0x52F390)
// ============================================================================

using GetMouseButton_t = unsigned int(__thiscall*)(void*, int);
static GetMouseButton_t GetMouseButtonAssociatedWithAction = reinterpret_cast<GetMouseButton_t>(0x52F580);

static char g_MouseTokenBuffer[8];

static char* __cdecl GetControllerSettingTextMouse_Impl(int action) {
    void* thisPtr = g_ControllerThis;

    if (!thisPtr || thisPtr == reinterpret_cast<void*>(0xFFFFFFFF)) return nullptr;
    if (action < 0 || action > 58) return nullptr;

    unsigned int mouseCode = GetMouseButtonAssociatedWithAction(thisPtr, action);
    if (mouseCode == 0) return nullptr;

    const char* token = GetSpriteTokenForMouseCode(mouseCode);
    if (token && g_Enabled && g_TexturesLoaded) {
        strcpy_s(g_MouseTokenBuffer, sizeof(g_MouseTokenBuffer), token);
        return g_MouseTokenBuffer;
    }

    switch (mouseCode) {
        case rsMOUSE_LEFT_BUTTON:       return CText_Get("FEC_MSL");
        case rsMOUSE_MIDDLE_BUTTON:     return CText_Get("FEC_MSM");
        case rsMOUSE_RIGHT_BUTTON:      return CText_Get("FEC_MSR");
        case rsMOUSE_WHEEL_UP_BUTTON:   return CText_Get("FEC_MWF");
        case rsMOUSE_WHEEL_DOWN_BUTTON: return CText_Get("FEC_MWB");
        case rsMOUSE_X1_BUTTON:         return CText_Get("FEC_MXO");
        case rsMOUSE_X2_BUTTON:         return CText_Get("FEC_MXT");
        default: return nullptr;
    }
}

__declspec(naked) void GetControllerSettingTextMouse_Thunk() {
    __asm {
        mov g_ControllerThis, ecx
        mov eax, [esp+4]
        push eax
        call GetControllerSettingTextMouse_Impl
        add esp, 4
        ret 4
    }
}

// ============================================================================
// CMenuManager::DisplayHelperText TEXT (0x57E240)
// ============================================================================

// Frontend navigation keys are fixed, so helper text names them directly, as in
// "CLICK LMB / RETURN - BACK". Single-letter names use the keyboard mapping.
struct HelperKeyName {
    const char* name;
    const char* tokens;
};

static const HelperKeyName g_HelperKeyNames[] = {
    { "CLICK LMB", "~M00~" }, { "LMB", "~M00~" },
    { "CLICK RMB", "~M01~" }, { "RMB", "~M01~" },
    { "MOUSEWHEEL", "~M07~" }, { "MSWHEEL", "~M07~" },
    { "RETURN", "~K53~" }, { "BACKSPACE", "~K81~" }, { "ESC", "~K83~" },
    { "SPACEBAR", "~K45~" }, { "PGUP", "~K51~" }, { "PGDN", "~K52~" },
    { "LEFT", "~K06~" }, { "RIGHT", "~K07~" },
    { "CURSORS", "~K04~~K05~~K06~~K07~" },
};

static const char* GetHelperKeyTokens(const char* name, size_t length) {
    if (length == 1) return GetSpriteTokenForKeyCode(static_cast<unsigned char>(name[0]));
    for (const HelperKeyName& key : g_HelperKeyNames) {
        if (strlen(key.name) == length && _strnicmp(key.name, name, length) == 0) return key.tokens;
    }
    return nullptr;
}

static std::string g_HelperText;

// Replace an item's '/'-separated key list only when every name has an icon,
// so translated or unknown names never leave a half-converted list.
static void AppendHelperItem(const char* item, const char* end) {
    static const char separator[] = " - ";
    const char* action = std::search(item, end, separator, separator + 3);
    if (action != end) {
        std::string keys;
        for (const char* name = item;;) {
            const char* nameEnd = std::find(name, action, '/');
            const char* first = name;
            const char* last = nameEnd;
            while (first != last && *first == ' ') ++first;
            while (last != first && last[-1] == ' ') --last;
            const char* tokens = GetHelperKeyTokens(first, static_cast<size_t>(last - first));
            if (!tokens) break;
            keys.append(name, first).append(tokens).append(last, nameEnd);
            if (nameEnd == action) {
                g_HelperText.append(keys).append(action, end);
                return;
            }
            keys += '/';
            name = nameEnd + 1;
        }
    }
    g_HelperText.append(item, end);
}

// Helper lines are separated by "~n~", and items within a line by " , ".
static const char* FindHelperItemEnd(const char* text, size_t& separatorLength) {
    for (; *text; ++text) {
        if (_strnicmp(text, "~n~", 3) == 0 || strncmp(text, " , ", 3) == 0) {
            separatorLength = 3;
            return text;
        }
    }
    separatorLength = 0;
    return text;
}

static char* ReplaceHelperKeyNames(char* text) {
    if (!text || !g_Enabled || !g_TexturesLoaded) return text;

    g_HelperText.clear();
    for (const char* item = text;;) {
        size_t separatorLength;
        const char* end = FindHelperItemEnd(item, separatorLength);
        AppendHelperItem(item, end);
        if (separatorLength == 0) break;
        g_HelperText.append(end, separatorLength);
        item = end + separatorLength;
    }
    return g_HelperText.data();
}

using TextGet_t = char*(__thiscall*)(void*, const char*);
static TextGet_t HelperText_Get_Original = reinterpret_cast<TextGet_t>(0x6A0050);

static char* __fastcall HelperText_Get_Hook(void* text, void*, const char* key) {
    return ReplaceHelperKeyNames(HelperText_Get_Original(text, key));
}

// ============================================================================
// ParseToken HOOK
// ============================================================================

static uint8_t* g_PS2Symbol = reinterpret_cast<uint8_t*>(0xC71A54);
static float* g_GInputSpriteWidth = nullptr;
static unsigned int* g_GInputTokenWidth = nullptr;

using ParseToken_t = char*(__cdecl*)(char*, CRGBA&, bool, char*);
static ParseToken_t ParseToken_Original = reinterpret_cast<ParseToken_t>(0x718F00);

static float GetInlineSpriteWidth(int spriteIndex) {
    // Stored PC widths use the gameplay size. Select menu size during token
    // parsing so measurement, drawing, and cursor advancement share the width.
    const float width = g_ExtendedSpriteWidths[spriteIndex];
    return FrontEndMenuManager.m_bMenuActive ? width * (MENU_ICON_SIZE / ICON_SIZE) : width;
}

char* __cdecl ParseToken_Hooked(char* text, CRGBA& color, bool isBlip, char* tag) {
    g_TokenWidth = 3;

    if (!text || !g_Enabled || !g_TexturesLoaded) {
        return ParseToken_Original(text, color, isBlip, tag);
    }

    if (text[0] == '~' && text[1] == 'K') {
        int keyIndex = ParseKeyboardToken(text);
        if (keyIndex >= 0 && keyIndex < KEYBOARD_COUNT) {
            int spriteIdx = KEYBOARD_SPRITE_BASE + keyIndex;
            if (spriteIdx >= 0 && spriteIdx < MAX_EXTENDED_SPRITES) {
                *g_PS2Symbol = static_cast<uint8_t>(spriteIdx);
                g_SymbolSpriteWidth = GetInlineSpriteWidth(spriteIdx);
                g_TokenWidth = 5;
                return text + 5;
            }
        }
    }

    if (text[0] == '~' && text[1] == 'M' && text[4] == '~') {
        char d1 = text[2], d2 = text[3];
        if (d1 == '0' && d2 >= '0' && d2 < '0' + MOUSE_COUNT) {
            int spriteIdx = MOUSE_SPRITE_BASE + (d2 - '0');
            if (spriteIdx >= 0 && spriteIdx < MAX_EXTENDED_SPRITES) {
                *g_PS2Symbol = static_cast<uint8_t>(spriteIdx);
                g_SymbolSpriteWidth = GetInlineSpriteWidth(spriteIdx);
                g_TokenWidth = 5;
                return text + 5;
            }
        }
    }

    char* result = ParseToken_Original(text, color, isBlip, tag);
    uint8_t symbolAfter = *g_PS2Symbol;

    if (symbolAfter != 0) {
        if (g_GInputLoaded && symbolAfter < KEYBOARD_SPRITE_BASE) {
            if (g_GInputSpriteWidth) {
                g_SymbolSpriteWidth = *g_GInputSpriteWidth;
            }
            if (g_GInputTokenWidth) {
                g_TokenWidth = *g_GInputTokenWidth;
            }
        } else if (symbolAfter < MAX_EXTENDED_SPRITES) {
            g_SymbolSpriteWidth = g_ExtendedSpriteWidths[symbolAfter];
        }
    }

    return result;
}

// ============================================================================
// INLINE ICON PROPORTIONS
// ============================================================================

using CSprite2d_Draw_t = void(__thiscall*)(CSprite2d*, const CRect&, const CRGBA&);
static CSprite2d_Draw_t CSprite2d_Draw_Original = reinterpret_cast<CSprite2d_Draw_t>(0x728350);

void __fastcall ButtonSprite_Draw_Hook(CSprite2d* sprite, void* edx, const CRect& rect, const CRGBA& color) {
    if (g_SpriteArray && sprite) {
        ptrdiff_t offset = reinterpret_cast<uintptr_t>(sprite) - reinterpret_cast<uintptr_t>(g_SpriteArray);
        int spriteIndex = static_cast<int>(offset / sizeof(CSprite2d));

        if (spriteIndex >= KEYBOARD_SPRITE_BASE &&
            spriteIndex < KEYBOARD_SPRITE_BASE + KEYBOARD_COUNT + MOUSE_COUNT &&
            spriteIndex < MAX_EXTENDED_SPRITES) {

            // PrintChar and its cursor advance both use our width multiplied by
            // the buffered font scale. Preserve that width so drawing and text
            // measurement agree, then derive height from the texture aspect.
            const float width = rect.right - rect.left;
            const float aspectRatio = g_ExtendedSpriteWidths[spriteIndex] / ICON_SIZE;
            if (width > 0.0f && aspectRatio > 0.0f) {
                const float height = width / aspectRatio;
                CRect iconRect = rect;
                // Shrink around the native symbol's center instead of its top
                // edge. The incoming width already carries the selected size.
                iconRect.top += (rect.bottom - rect.top - height) * 0.5f;
                iconRect.bottom = iconRect.top + height;

                CSprite2d_Draw_Original(sprite, iconRect, color);
                return;
            }
        }
    }

    CSprite2d_Draw_Original(sprite, rect, color);
}

// ============================================================================
// PUBLIC API: Direct Drawing
// ============================================================================

void DrawIcon(MouseButton button, float x, float y, float size) {
    if (!g_TexturesLoaded || !g_SpriteArray || button < 0 || button >= MOUSE_COUNT) return;

    CSprite2d* sprite = &g_SpriteArray[MOUSE_SPRITE_BASE + button];
    if (!sprite->m_pTexture) return;

    CRect rect(x - size * 0.5f, y - size * 0.5f, x + size * 0.5f, y + size * 0.5f);
    sprite->Draw(rect, CRGBA(255, 255, 255, 255));
}

void DrawIconColored(MouseButton button, float x, float y, float size,
                     unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    if (!g_TexturesLoaded || !g_SpriteArray || button < 0 || button >= MOUSE_COUNT) return;

    CSprite2d* sprite = &g_SpriteArray[MOUSE_SPRITE_BASE + button];
    if (!sprite->m_pTexture) return;

    CRect rect(x - size * 0.5f, y - size * 0.5f, x + size * 0.5f, y + size * 0.5f);
    sprite->Draw(rect, CRGBA(r, g, b, a));
}

// ============================================================================
// AddTokenToWidth HOOK
// ============================================================================

char* __stdcall AddTokenToWidth(char* pText, float& fPos) {
    if (!pText) return pText;

    CRGBA tempColor;
    char* pNewPtr = ParseToken_Hooked(pText, tempColor, true, nullptr) - 1;

    uint8_t symbolIdx = *g_PS2Symbol;
    if (symbolIdx != 0) {
        float fontScaleY = g_FontScaleY ? *g_FontScaleY : 1.0f;
        float widthToAdd = g_SymbolSpriteWidth * fontScaleY;
        *g_PS2Symbol = 0;
        fPos += widthToAdd;
    }

    return pNewPtr;
}

// ============================================================================
// TokenWidthHook
// ============================================================================

void __declspec(naked) TokenWidthHook() {
    __asm {
        test    dl, dl
        jz      TokenWidthHook_Return
        sub     esi, g_TokenWidth
    TokenWidthHook_Return:
        ret
    }
}

// ============================================================================
// INSTALLATION
// ============================================================================

static bool g_HooksInstalled = false;

static void InstallHelperTextHooks() {
    // DisplayHelperText fetches its key-argument, status, and menu-entry texts
    // through these calls. Chain an earlier redirect, but leave the function
    // untouched if another plugin has already split or replaced its calls.
    const uintptr_t calls[] = { 0x57E2A5, 0x57E37A, 0x57E44E };
    uintptr_t destination = 0;
    for (uintptr_t call : calls) {
        if (*reinterpret_cast<uint8_t*>(call) != 0xE8) return;
        const uintptr_t target = call + 5 + *reinterpret_cast<int32_t*>(call + 1);
        if (destination && target != destination) return;
        destination = target;
    }

    HelperText_Get_Original = reinterpret_cast<TextGet_t>(destination);
    for (uintptr_t call : calls) {
        patch::RedirectCall(call, HelperText_Get_Hook);
    }
}

static void InstallGInputCompatibleHooks() {
    if (g_HooksInstalled) return;
    g_HooksInstalled = true;

    ExpandButtonSpriteArray();

    if (g_GInputLoaded) {
        int32_t ginputOffset = *reinterpret_cast<int32_t*>(0x719965 + 1);
        uintptr_t ginputParseToken = 0x719965 + 5 + ginputOffset;
        ParseToken_Original = reinterpret_cast<ParseToken_t>(ginputParseToken);

        g_GInputSpriteWidth = *reinterpret_cast<float**>(0x718A98);

        int32_t tokenHookOffset = *reinterpret_cast<int32_t*>(0x71A336 + 1);
        uintptr_t ginputTokenWidthHook = 0x71A336 + 5 + tokenHookOffset;

        uint8_t* hookBytes = reinterpret_cast<uint8_t*>(ginputTokenWidthHook);
        if (hookBytes[0] == 0x84 && hookBytes[1] == 0xD2 &&
            hookBytes[2] == 0x74 &&
            hookBytes[4] == 0x2B && hookBytes[5] == 0x35) {
            g_GInputTokenWidth = *reinterpret_cast<unsigned int**>(ginputTokenWidthHook + 6);
        } else {
            g_GInputTokenWidth = nullptr;
        }

        patch::RedirectCall(0x719965, ParseToken_Hooked);
        patch::RedirectCall(0x71A018, ParseToken_Hooked);
        patch::RedirectCall(0x71A2C4, ParseToken_Hooked);

        patch::SetPointer(0x718A98, &g_SymbolSpriteWidth);
        patch::SetPointer(0x719A55, &g_SymbolSpriteWidth);

    } else {
        patch::RedirectCall(0x719965, ParseToken_Hooked);
        patch::RedirectCall(0x71A018, ParseToken_Hooked);
        patch::RedirectCall(0x71A2C4, ParseToken_Hooked);

        patch::SetPointer(0x718A98, &g_SymbolSpriteWidth);
        patch::SetPointer(0x719A55, &g_SymbolSpriteWidth);
    }

    patch::SetUInt(0x71A181, 0x0C24448D);
    patch::SetUShort(0x71A185, 0x5650);

    patch::SetUChar(0x71A187, 0xE8);
    uintptr_t callTarget = reinterpret_cast<uintptr_t>(&AddTokenToWidth);
    uintptr_t callAddr = 0x71A187;
    int32_t relativeOffset = static_cast<int32_t>(callTarget - (callAddr + 5));
    patch::SetInt(0x71A188, relativeOffset);

    patch::SetUInt(0x71A18C, 0x08EBF08B);

    patch::RedirectCall(0x71A336, TokenWidthHook);
    patch::RedirectCall(0x718AE5, ButtonSprite_Draw_Hook);

    InstallHelperTextHooks();
}

void InstallHooks() {
    patch::RedirectJump(0x52FE10, GetControllerSettingTextKeyBoard_Thunk);
    patch::RedirectJump(0x52F390, GetControllerSettingTextMouse_Thunk);

    Events::initRwEvent += []() {
        InstallGInputCompatibleHooks();
        LoadTextures();
    };

    Events::shutdownRwEvent += []() {
        UnloadTextures();
    };
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool IsEnabled() { return g_Enabled; }
void SetEnabled(bool enabled) { g_Enabled = enabled; }

void ReloadTextures() {
    UnloadTextures();
    LoadTextures();
}

} // namespace ButtonIcons
