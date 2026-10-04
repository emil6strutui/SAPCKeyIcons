#pragma once

// The executable compiles the production hooks unchanged, but never installs
// them or accesses a running game. Only engine dependencies are replaced.
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

struct CRect {
    float left{}, bottom{}, right{}, top{};
    CRect() = default;
    // Match the actual SA constructor at 0x4041C0: y1 is top, y2 is bottom.
    // The SDK's member order differs from constructor parameter order.
    CRect(float l, float y1, float r, float y2) : left(l), bottom(y2), right(r), top(y1) {}
};
static_assert(offsetof(CRect, bottom) == 4 && offsetof(CRect, top) == 12);
struct CRGBA {
    unsigned char r{}, g{}, b{}, a{};
    CRGBA() = default;
    CRGBA(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
        : r(red), g(green), b(blue), a(alpha) {}
};
struct RwRaster { int width{}, height{}; };
struct RwTexture { RwRaster raster; };
inline RwRaster* RwTextureGetRaster(RwTexture* texture) { return &texture->raster; }
inline int RwRasterGetWidth(RwRaster* raster) { return raster->width; }
inline int RwRasterGetHeight(RwRaster* raster) { return raster->height; }

namespace Harness {
inline std::map<std::string, RwTexture> textures;
inline CRect drawnRect;
inline CRGBA drawnColor;
inline int drawCount = 0;
inline void CaptureDraw(const CRect& rect, const CRGBA& color) {
    drawnRect = rect;
    drawnColor = color;
    ++drawCount;
}
}
class CSprite2d {
public:
    RwTexture* m_pTexture{};
    void SetTexture(char* name) {
        auto found = Harness::textures.find(name);
        m_pTexture = found == Harness::textures.end() ? nullptr : &found->second;
    }
    void Delete() { m_pTexture = nullptr; }
    void Draw(const CRect& rect, const CRGBA& color) { Harness::CaptureDraw(rect, color); }
};
class CTxdStore {
public:
    static int AddTxdSlot(const char*) { return 1; }
    static bool LoadTxd(int, const char*) { return !Harness::textures.empty(); }
    static void AddRef(int) {}
    static void SetCurrentTxd(int) {}
    static void PopCurrentTxd() {}
    static void RemoveTxdSlot(int) {}
};
inline void* GetModuleHandleA(const char*) { return nullptr; }
namespace plugin {
namespace patch {
template<class T> void SetPointer(uintptr_t, T) {}
template<class T> void RedirectCall(uintptr_t, T) {}
template<class T> void RedirectJump(uintptr_t, T) {}
inline void SetUInt(uintptr_t, unsigned int) {}
inline void SetUShort(uintptr_t, unsigned short) {}
inline void SetUChar(uintptr_t, unsigned char) {}
inline void SetInt(uintptr_t, int) {}
}
namespace Events {
struct Event { template<class T> void operator+=(T) {} };
inline Event initRwEvent, shutdownRwEvent;
}
}
