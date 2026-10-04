#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <CMenuManager.h>

// Including the implementation exposes its internal hook seams without adding
// test-only branches to the DLL or copying the production mapping/size logic.
#include "../source/ButtonIcons.cpp"

namespace {
int failures = 0;
int checks = 0;
uint8_t fallbackSymbol = 0;
void Check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        ++failures;
        if (failures <= 20) std::cerr << "FAIL: " << message << '\n';
        else if (failures == 21) std::cerr << "Further failure details suppressed; final count includes every check.\n";
    }
}
bool Near(float a, float b) { return std::fabs(a - b) < 0.001f; }

// Read only RW chunk headers and native texture names/dimensions. The actual
// bundled TXD supplies every raster used by LoadTextures and the drawing tests.
uint32_t Read32(const std::vector<unsigned char>& bytes, size_t offset) {
    uint32_t value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}
uint16_t Read16(const std::vector<unsigned char>& bytes, size_t offset) {
    uint16_t value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}
void ReadChunks(const std::vector<unsigned char>& bytes, size_t begin, size_t end) {
    for (size_t pos = begin; pos + 12 <= end;) {
        const auto type = Read32(bytes, pos);
        const size_t length = Read32(bytes, pos + 4);
        const size_t body = pos + 12;
        if (length > end - body) break;
        if (type == 0x16) ReadChunks(bytes, body, body + length);
        if (type == 0x15 && length >= 100 && Read32(bytes, body) == 1) {
            const size_t native = body + 12;
            const char* name = reinterpret_cast<const char*>(bytes.data() + native + 8);
            const size_t nameLength = strnlen_s(name, 32);
            Harness::textures.emplace(std::string(name, nameLength),
                RwTexture{ { Read16(bytes, native + 80), Read16(bytes, native + 82) } });
        }
        pos = body + length;
    }
}
char* __cdecl OriginalParse(char* text, CRGBA&, bool, char*) {
    *ButtonIcons::g_PS2Symbol = fallbackSymbol;
    return text ? text + 3 : nullptr;
}
void __fastcall OriginalDraw(CSprite2d*, void*, const CRect& rect, const CRGBA& color) {
    Harness::CaptureDraw(rect, color);
}

void CheckKeyMapping(unsigned int keyCode, int expectedIndex, const char* textureName,
                     unsigned char* controller) {
    using namespace ButtonIcons;
    const char* mapped = GetSpriteTokenForKeyCode(keyCode);
    char expected[8];
    sprintf_s(expected, "~K%02d~", expectedIndex);
    Check(mapped && std::strcmp(mapped, expected) == 0,
          "key " + std::to_string(keyCode) + " must map to " + expected);
    // Until the mapping is fixed, avoid the text fallback's absolute game call.
    if (!mapped) return;
    std::memcpy(controller + 0xB70, &keyCode, sizeof(keyCode));
    char* token = GetControllerSettingTextKeyBoard_Impl(0, 0);
    Check(token && std::strcmp(token, expected) == 0, "controller setting returns icon token");
    CRGBA color;
    Check(ParseToken_Hooked(token, color, false, nullptr) == token + 5,
          "controller token consumes five bytes");
    const int index = KEYBOARD_SPRITE_BASE + expectedIndex;
    Check(*g_PS2Symbol == index, "controller token selects matching sprite");
    Check(g_SpriteArray[index].m_pTexture == &Harness::textures.at(textureName),
          "controller token selects expected bundled texture");
}

void CheckExistingTokens() {
    using namespace ButtonIcons;
    // Public token ordering is a compatibility contract, independent of names
    // in the implementation under test.
    const char* expectedNames[] = {
        "W", "A", "S", "D", "38", "40", "37", "39", "E", "Q", "F", "G", "H",
        "N", "Y", "X", "Z", "V", "C", "B", "I", "J", "K", "L", "M", "O", "P", "R", "T", "U",
        "96", "97", "98", "99", "100", "101", "102", "103", "104", "105", "110",
        "162", "163", "160", "164", "32", "9", "20", "46", "36", "35", "33", "34", "13", "padenter",
        "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "107", "106", "109", "45",
        "112", "113", "114", "115", "116", "117", "118", "119", "120", "121", "122", "123"
    };
    CRGBA color;
    for (int i = 0; i < 81; ++i) {
        char token[8];
        sprintf_s(token, "~K%02d~", i);
        Check(ParseToken_Hooked(token, color, true, nullptr) == token + 5,
              std::string(token) + " consumes five bytes");
        Check(*g_PS2Symbol == KEYBOARD_SPRITE_BASE + i, std::string(token) + " preserves index");
        auto found = Harness::textures.find(expectedNames[i]);
        Check(found != Harness::textures.end() &&
              g_SpriteArray[*g_PS2Symbol].m_pTexture == &found->second,
              std::string(token) + " preserves texture " + expectedNames[i]);
    }
    const char* mouseNames[] = { "1", "2", "4", "MWHU", "MWHD", "5", "6" };
    for (int i = 0; i < 7; ++i) {
        char token[8];
        sprintf_s(token, "~M%02d~", i);
        Check(ParseToken_Hooked(token, color, true, nullptr) == token + 5,
              std::string(token) + " consumes five bytes");
        Check(*g_PS2Symbol == MOUSE_SPRITE_BASE + i, std::string(token) + " preserves mouse index");
        Check(g_SpriteArray[*g_PS2Symbol].m_pTexture == &Harness::textures.at(mouseNames[i]),
              std::string(token) + " preserves mouse texture");
    }
}

void CheckGeometry(float& fontScale, bool menuActive) {
    using namespace ButtonIcons;
    const float expectedSize = menuActive ? 17.0f : 13.0f;
    struct Sample { char kind; int key; };
    for (const Sample sample : { Sample{'K', KEY_UP}, {'K', KEY_SPACE}, {'K', KEY_LSHIFT},
                                {'K', 81}, {'K', 82}, {'M', 0}, {'M', 3}, {'M', 6} }) {
        // Before the fix, the new mapping checks already report missing keys.
        if (sample.kind == 'K' && sample.key >= KEYBOARD_COUNT) continue;
        const int spriteIndex = (sample.kind == 'K' ? KEYBOARD_SPRITE_BASE : MOUSE_SPRITE_BASE) + sample.key;
        auto raster = RwTextureGetRaster(g_SpriteArray[spriteIndex].m_pTexture);
        const float aspect = static_cast<float>(raster->width) / raster->height;
        for (float scale : { 0.5f, 1.0f, 1.5f, 2.5f }) {
            fontScale = scale;
            char token[8];
            sprintf_s(token, "~%c%02d~", sample.kind, sample.key);
            float measured = 0.0f;
            Check(AddTokenToWidth(token, measured) == token + 4, "width hook token cursor");
            Check(*g_PS2Symbol == 0, "width hook clears symbol state");
            CRGBA color(255, 255, 255, 255);
            ParseToken_Hooked(token, color, false, nullptr);
            const float drawAdvance = g_SymbolSpriteWidth * scale;
            Check(Near(measured, drawAdvance), "width and draw advances agree");
            // CFont's button branch spans y + 2*scaleY through y + 19*scaleY.
            CRect rect(100.0f, 200.0f + 2.0f * scale, 100.0f + drawAdvance, 200.0f + 19.0f * scale);
            ButtonSprite_Draw_Hook(&g_SpriteArray[spriteIndex], nullptr, rect, color);
            const float width = Harness::drawnRect.right - Harness::drawnRect.left;
            const float height = Harness::drawnRect.bottom - Harness::drawnRect.top;
            const std::string label = std::string(menuActive ? "menu " : "gameplay ") + token +
                " scale=" + std::to_string(scale);
            Check(height > 0.0f && Near(width / height, aspect), label + " keeps texture aspect");
            Check(width <= measured + 0.001f, label + " fits measured advance");
            Check(Near(measured, expectedSize * aspect * scale), label + " measures the context's icon size");
            Check(Near(height, expectedSize * scale), label + " uses the context's font-relative height");
            Check(Near((Harness::drawnRect.top + Harness::drawnRect.bottom) * 0.5f,
                       (rect.top + rect.bottom) * 0.5f), label + " preserves vertical center");
            Check(Near(Harness::drawnRect.left, rect.left) && Near(Harness::drawnRect.right, rect.right),
                  label + " preserves horizontal bounds");

            // PrintChar receives the cached render scale in its rectangle. The
            // mutable layout scale and menu state may have changed since the
            // text was queued. Draw must retain the queued icon dimensions.
            fontScale = scale * 2.0f;
            FrontEndMenuManager.m_bMenuActive = !menuActive;
            ButtonSprite_Draw_Hook(&g_SpriteArray[spriteIndex], nullptr, rect, color);
            Check(Near(Harness::drawnRect.bottom - Harness::drawnRect.top, expectedSize * scale),
                  label + " retains buffered size after layout/menu state changes");
            FrontEndMenuManager.m_bMenuActive = menuActive;
            if (sample.kind == 'K' && sample.key == KEY_UP)
                std::cout << label << " advance=" << measured << " draw=" << width << 'x' << height << '\n';
        }
    }
}

void CheckPassthrough(float& fontScale) {
    using namespace ButtonIcons;
    fallbackSymbol = 1;
    g_ExtendedSpriteWidths[1] = 17.0f;
    float ginputWidth = 24.0f;
    unsigned int ginputTokenWidth = 3;
    g_GInputSpriteWidth = &ginputWidth;
    g_GInputTokenWidth = &ginputTokenWidth;
    CRGBA color(5, 10, 15, 20);
    char token[] = "~X~";
    Check(ParseToken_Hooked(token, color, false, nullptr) == token + 3, "native parser remains callable");
    Check(*g_PS2Symbol == 1, "native parser retains sprite selection");
    Check(Near(g_SymbolSpriteWidth, g_GInputLoaded ? 24.0f : 17.0f), "native parser retains width");
    Check(g_TokenWidth == 3, "native parser retains token width");
    fontScale = 0.5f;
    float measured = 0.0f;
    AddTokenToWidth(token, measured);
    Check(Near(measured, (g_GInputLoaded ? 24.0f : 17.0f) * fontScale), "native width remains unchanged");
    const CRect rect(1.0f, 2.0f, 9.0f, 12.0f);
    ButtonSprite_Draw_Hook(&g_SpriteArray[1], nullptr, rect, color);
    Check(Near(Harness::drawnRect.left, rect.left) && Near(Harness::drawnRect.top, rect.top) &&
          Near(Harness::drawnRect.right, rect.right) && Near(Harness::drawnRect.bottom, rect.bottom),
          "native/GInput drawing rectangle is unchanged");
    Check(Harness::drawnColor.a == color.a, "draw color is unchanged");
    fallbackSymbol = 0;
    g_GInputSpriteWidth = nullptr;
    g_GInputTokenWidth = nullptr;
}

void CheckDirectDrawing() {
    using namespace ButtonIcons;
    DrawIcon(MOUSE_LMB, 50.0f, 100.0f, 8.0f);
    Check(Near(Harness::drawnRect.left, 46.0f) && Near(Harness::drawnRect.top, 96.0f) &&
          Near(Harness::drawnRect.right, 54.0f) && Near(Harness::drawnRect.bottom, 104.0f),
          "direct drawing retains explicit size and center");
    DrawIconColored(MOUSE_LMB, 50.0f, 100.0f, 8.0f, 20, 40, 60, 80);
    Check(Harness::drawnColor.r == 20 && Harness::drawnColor.g == 40 &&
          Harness::drawnColor.b == 60 && Harness::drawnColor.a == 80, "direct drawing retains color");
}
}

int main(int argc, char** argv) {
    const char* assetPath = argc > 1 ? argv[1] : "game_assets/models/pcbtns.txd";
    std::ifstream asset(assetPath, std::ios::binary);
    if (!asset) { std::cerr << "Cannot open TXD: " << assetPath << '\n'; return 2; }
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(asset)), {});
    ReadChunks(bytes, 0, bytes.size());
    if (Harness::textures.empty()) { std::cerr << "TXD contains no readable textures\n"; return 2; }
    std::cout << "Using " << Harness::textures.size() << " textures from " << assetPath << '\n';
    using namespace ButtonIcons;
    uint8_t symbol = 0;
    float fontScale = 1.0f;
    char keyName[0x30]{};
    alignas(4) unsigned char controller[0xC00]{};
    g_PS2Symbol = &symbol;
    g_FontScaleY = &fontScale;
    g_KeyNameBuffer = keyName;
    g_ControllerThis = controller;
    g_SpriteArray = g_ExtendedSprites;
    g_SpriteWidths = g_ExtendedSpriteWidths;
    ParseToken_Original = OriginalParse;
    CSprite2d_Draw_Original = reinterpret_cast<CSprite2d_Draw_t>(OriginalDraw);
    for (int keyboardBase : { 15, 50 }) {
        std::cout << "Sprite base " << keyboardBase << '\n';
        KEYBOARD_SPRITE_BASE = keyboardBase;
        MOUSE_SPRITE_BASE = keyboardBase + KEYBOARD_COUNT;
        g_GInputLoaded = keyboardBase == 50;
        LoadTextures();
        Check(g_TexturesLoaded, "textures loaded");
        CheckKeyMapping(1042, 81, "8", controller);
        CheckKeyMapping(1047, 82, "161", controller);
        CheckExistingTokens();
        // Enter and leave the controls menu without reloading textures: size
        // changes must follow the context immediately and must not leak back.
        for (bool menuActive : { false, true, false }) {
            FrontEndMenuManager.m_bMenuActive = menuActive;
            CheckGeometry(fontScale, menuActive);
            CheckPassthrough(fontScale);
            CheckDirectDrawing();
        }
        UnloadTextures();
    }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
