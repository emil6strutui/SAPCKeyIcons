#include "ButtonIcons.h"

class PCKeyIconsPlugin {
public:
    PCKeyIconsPlugin() {
        ButtonIcons::InstallHooks();
    }
} g_PCKeyIconsPlugin;
