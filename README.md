# PC Key Icons for GTA San Andreas

Replaces keyboard and mouse control names in GTA San Andreas help text with matching button icons.

## Features

- Icons for 83 keyboard keys, including Backspace and right Shift, and 7 mouse inputs
- Vertically centered icons with compact in-game sizing and larger controls-menu sizing, preserving the buttons' proportions
- Automatic support for the player's configured keyboard and mouse bindings
- Compatibility with `GInputSA.asi`
- Mod Loader installation without replacing the game's original texture file
- C++ helpers for enabling/disabling icons, reloading textures, and drawing mouse icons

## Requirements

- GTA San Andreas 1.0 US
- An ASI loader

## Installation

### With Mod Loader

With Mod Loader installed, extract `PCKeyIcons-ModLoader.zip` into the game directory. The resulting layout is:

```text
modloader/
  PCKeyIcons/
    PCKeyIcons.SA.asi
    models/
      pcbtns.txd
```

Install the ASI only once. When switching to Mod Loader, remove the previous `PCKeyIcons.SA.asi` from `scripts` or any other loader directory. Restart the game after installing or updating the ASI. The standalone plugin must not be loaded alongside a 1991 ASI that already includes these button-icon hooks.

### Without Mod Loader

1. Copy `PCKeyIcons.SA.asi` to the game's `scripts` directory.
2. Copy the bundled `models/pcbtns.txd` (under `game_assets` in this repository) to `scripts/models/pcbtns.txd` beside the ASI, creating the `models` directory if needed.

### Texture lookup and troubleshooting

The plugin first loads `models/pcbtns.txd` beside its own ASI. If that file is unavailable, cannot be loaded, or lacks required icons, it tries the game's `models/pcbtns.txd`. This preserves support for older installations that placed the texture in the game directory.

If neither file is usable, control names remain as text. Check that the bundled TXD is installed in one of these locations; the game's original `pcbtns.txd` does not contain this mod's complete icon set. Restart after correcting the installation.

## Building

Building requires Visual Studio 2022 with the C++ desktop tools, the v143 toolset, and a Windows SDK. Set `PLUGIN_SDK_DIR` to a plugin-sdk checkout with its GTA San Andreas libraries built.

Run commands from the repository root. To build Release without deploying into the game:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' '.\PCKeyIcons.sln' /p:Configuration=Release /p:Platform=GTA-SA /p:PostBuildEventUseInBuild=false
```

Use `/p:Configuration=Debug` for Debug. Outputs are `bin/GTA-SA/Release/PCKeyIcons.SA.asi` and `bin/GTA-SA/Debug/PCKeyIcons.SA.asi`.

Alternatively, open `PCKeyIcons.sln` in Visual Studio and select the solution configuration `Release|GTA-SA` or `Debug|GTA-SA`. These map to Win32 project configurations.

When the post-build event is enabled and `GTA_SA_DIR` is set, a successful build forcibly closes a running `gta_sa.exe`, copies the ASI into the game's `scripts` directory, and overwrites the game's `models/pcbtns.txd`. Leave that variable unset for ordinary IDE builds, or disable the post-build event as in the command above. Automatic deployment uses the legacy installation layout, not the Mod Loader package layout.

### Build troubleshooting

If MSBuild fails with `MSB6001` reporting duplicate `PATH` and `Path` keys, ensure its process environment contains only one spelling of that variable. Normalize the build subprocess environment rather than changing the machine-wide PATH.

## Packaging

After building Release, run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\package.ps1
```

This creates `bin/packages/PCKeyIcons-ModLoader.zip` from the local Release ASI and bundled TXD. It does not build the project or copy files into the game. Running it again replaces the generated archive. Use `-Configuration Debug` to package an already built Debug ASI as `PCKeyIcons-ModLoader-Debug.zip`.

## Regression checks

The standalone harness compiles the real icon implementation with stubbed game APIs and reads texture names and dimensions from the bundled TXD. It checks key mappings, token compatibility, drawing/measurement consistency, ASI-relative texture lookup priority, fallback to the game's texture dictionary, and cleanup after failed loads without launching GTA:

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' '.\tests\ButtonIconsRegression.vcxproj' /p:Configuration=Release /p:Platform=Win32
& '.\bin\tests\ButtonIconsRegression.exe'
```

The harness does not run the game or Mod Loader. In-game checks are still needed for changes affecting hooks, rendering, or installation behavior.

## Customization

There is no external size configuration file. To change inline icon heights, edit `ButtonIcons::ICON_SIZE` (13 font units in-game) and `ButtonIcons::MENU_ICON_SIZE` (17 font units while the frontend menu is active) in [source/ButtonIcons.h](source/ButtonIcons.h), then rebuild. Width follows each texture's aspect ratio, and both sizes retain the native symbol's vertical center.

The same header declares the C++ helpers available to code built into this plugin. Existing `~Knn~` keyboard and `~Mnn~` mouse token indices remain stable when new keys are added.

## Credits

Silent for GInput source-code and icon-drawing help in the original implementation.
