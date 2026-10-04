#pragma once
#include "../GameStubs.h"

namespace plugin::paths {
inline const char* GetPluginDirRelativePathA(const char* relativePath) {
    static std::string path;
    path = Harness::pluginDirectory + "\\" + relativePath;
    return path.c_str();
}
inline const char* GetGameDirRelativePathA(const char* relativePath) {
    static std::string path;
    path = Harness::gameDirectory + "\\" + relativePath;
    return path.c_str();
}
}
