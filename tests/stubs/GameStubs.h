#pragma once

// The executable compiles the production hooks unchanged, but never installs
// them or accesses a running game. Only engine dependencies are replaced.
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

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
struct RwTexture { RwRaster raster; bool hasRaster = true; };
inline RwRaster* RwTextureGetRaster(RwTexture* texture) { return texture->hasRaster ? &texture->raster : nullptr; }
inline int RwRasterGetWidth(RwRaster* raster) { return raster->width; }
inline int RwRasterGetHeight(RwRaster* raster) { return raster->height; }

namespace Harness {
inline std::map<std::string, RwTexture> textures;
enum class FileKind { Missing, Complete, Vanilla, Incomplete, Corrupt, Unreadable, ZeroRaster, ZeroHeight, NullRaster };
inline std::string pluginDirectory = "C:\\Games\\GTA SA\\modloader\\PCKeyIcons";
inline std::string gameDirectory = "C:\\Games\\GTA SA";
inline std::map<std::string, FileKind> files;
inline std::map<int, std::string> txdSlots;
inline std::vector<int> txdStack;
inline std::vector<std::string> streamOpenAttempts, filenameLoadAttempts, streamLoadPaths;
inline int nextSlot = 1, currentTxd = 77, liveStreams = 0, stackUnderflows = 0;
inline int boundTextures = 0, prematureSlotRemovals = 0, addRefCalls = 0;
inline bool failSlotAllocation = false;
inline RwTexture zeroRasterTexture{{0, 128}}, zeroHeightTexture{{128, 0}}, nullRasterTexture{{128, 128}, false};
inline std::string PluginTexturePath() { return pluginDirectory + "\\models\\pcbtns.txd"; }
inline std::string GameTexturePath() { return gameDirectory + "\\models\\pcbtns.txd"; }
inline FileKind GetFileKind(const std::string& path) {
    const auto found = files.find(path);
    return found == files.end() ? FileKind::Missing : found->second;
}
inline void ResetTextureIo() {
    files.clear(); txdSlots.clear(); txdStack.clear();
    streamOpenAttempts.clear(); filenameLoadAttempts.clear(); streamLoadPaths.clear();
    nextSlot = 1; currentTxd = 77; liveStreams = 0; stackUnderflows = 0;
    boundTextures = 0; prematureSlotRemovals = 0; addRefCalls = 0;
    failSlotAllocation = false;
}
inline CRect drawnRect;
inline CRGBA drawnColor;
inline int drawCount = 0;
inline void CaptureDraw(const CRect& rect, const CRGBA& color) {
    drawnRect = rect;
    drawnColor = color;
    ++drawCount;
}
}
enum RwStreamType { rwSTREAMFILENAME };
enum RwStreamAccessType { rwSTREAMREAD };
struct RwStream { std::string path; };
inline RwStream* RwStreamOpen(RwStreamType, RwStreamAccessType, const void* data) {
    const std::string path = static_cast<const char*>(data);
    Harness::streamOpenAttempts.push_back(path);
    const auto kind = Harness::GetFileKind(path);
    if (kind == Harness::FileKind::Missing || kind == Harness::FileKind::Unreadable) return nullptr;
    ++Harness::liveStreams;
    return new RwStream{path};
}
inline bool RwStreamClose(RwStream* stream, void*) {
    if (!stream) return false;
    --Harness::liveStreams;
    delete stream;
    return true;
}
class CSprite2d {
public:
    RwTexture* m_pTexture{};
    void SetTexture(char* name) {
        Delete();
        const auto slot = Harness::txdSlots.find(Harness::currentTxd);
        const auto kind = slot == Harness::txdSlots.end() ? Harness::FileKind::Missing :
            Harness::GetFileKind(slot->second);
        if (kind == Harness::FileKind::Missing || kind == Harness::FileKind::Vanilla ||
            (kind == Harness::FileKind::Incomplete && std::string(name) == "161")) return;
        auto found = Harness::textures.find(name);
        m_pTexture = found == Harness::textures.end() ? nullptr : &found->second;
        if (std::string(name) == "161") {
            if (kind == Harness::FileKind::ZeroRaster) m_pTexture = &Harness::zeroRasterTexture;
            if (kind == Harness::FileKind::ZeroHeight) m_pTexture = &Harness::zeroHeightTexture;
            if (kind == Harness::FileKind::NullRaster) m_pTexture = &Harness::nullRasterTexture;
        }
        if (m_pTexture) ++Harness::boundTextures;
    }
    void Delete() { if (m_pTexture) --Harness::boundTextures; m_pTexture = nullptr; }
    void Draw(const CRect& rect, const CRGBA& color) { Harness::CaptureDraw(rect, color); }
};
class CTxdStore {
public:
    static int AddTxdSlot(const char*) {
        if (Harness::failSlotAllocation) return -1;
        const int slot = Harness::nextSlot++;
        Harness::txdSlots[slot] = {};
        return slot;
    }
    static bool LoadPath(int slot, const std::string& path) {
        const auto kind = Harness::GetFileKind(path);
        if (kind == Harness::FileKind::Missing || kind == Harness::FileKind::Unreadable ||
            kind == Harness::FileKind::Corrupt) return false;
        Harness::txdSlots[slot] = path;
        return true;
    }
    static bool LoadTxd(int slot, const char* name) {
        Harness::filenameLoadAttempts.push_back(name);
        const std::string path = std::string(name).find(':') == std::string::npos ?
            Harness::gameDirectory + "\\" + name : name;
        return LoadPath(slot, path);
    }
    static bool LoadTxd(int slot, RwStream* stream) {
        Harness::streamLoadPaths.push_back(stream->path);
        return LoadPath(slot, stream->path);
    }
    static void AddRef(int) { ++Harness::addRefCalls; }
    static void PushCurrentTxd() { Harness::txdStack.push_back(Harness::currentTxd); }
    static void SetCurrentTxd(int slot) { Harness::currentTxd = slot; }
    static void PopCurrentTxd() {
        if (Harness::txdStack.empty()) { ++Harness::stackUnderflows; Harness::currentTxd = -1; }
        else { Harness::currentTxd = Harness::txdStack.back(); Harness::txdStack.pop_back(); }
    }
    static void RemoveTxdSlot(int slot) {
        if (Harness::boundTextures != 0) ++Harness::prematureSlotRemovals;
        Harness::txdSlots.erase(slot);
    }
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
