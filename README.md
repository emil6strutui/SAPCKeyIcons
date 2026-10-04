# PC Key Icons for GTA San Andreas

Replaces keyboard and mouse control names in GTA San Andreas help text with matching button icons.

## Features

- Icons for 83 keyboard keys and 7 mouse inputs
- Vertically centered icons with compact in-game sizing and larger menu sizing, preserving texture proportions and matching drawing/wrapping widths
- Automatic support for the player's configured keyboard and mouse bindings
- Compatibility with `GInputSA.asi`
- Runtime enable, reload, and direct mouse-icon drawing APIs for other code in this plugin

## Requirements

- GTA San Andreas 1.0 US
- An ASI loader

## Installation

1. Copy `PCKeyIcons.SA.asi` to the game's `scripts` directory.
2. Copy `game_assets/models/pcbtns.txd` to the game's `models` directory.

Do not load this standalone plugin together with a build of `1991.SA.asi` that still contains the same `ButtonIcons` module. Both plugins patch the same game functions.

## Building

Open `PCKeyIcons.sln` in Visual Studio 2022 and build `Release GTA-SA|Win32`, or run:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' '.\PCKeyIcons.sln' /p:Configuration=Release /p:Platform=GTA-SA
```

Set `PLUGIN_SDK_DIR` to a plugin-sdk checkout with the GTA San Andreas project generated and built. If `GTA_SA_DIR` is set, the post-build step deploys both the ASI and `pcbtns.txd` to the game automatically.

## Regression checks

The standalone harness compiles the real icon implementation with stubbed game APIs and reads texture names and dimensions from the bundled TXD. It checks key mappings, token compatibility, and drawing/measurement consistency without launching GTA:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' '.\tests\ButtonIconsRegression.vcxproj' /p:Configuration=Release /p:Platform=Win32
& '.\bin\tests\ButtonIconsRegression.exe'
```

Inline icon heights are controlled by `ButtonIcons::ICON_SIZE` (13 font units in-game) and `ButtonIcons::MENU_ICON_SIZE` (17 font units while the frontend menu is active) in `source/ButtonIcons.h`. Both sizes retain the native symbol's vertical center. Existing keyboard tokens retain their indices; Backspace is `~K81~` and right Shift is `~K82~`.

## Credits

Extracted from the GTA 1991 `ButtonIcons` module. Silent is credited for GInput source-code and icon-drawing help in the original implementation.
