# Agent instructions

These instructions apply to the entire repository.

## Documentation ownership

- Keep [README.md](README.md) focused on players and human contributors. This file is only for agent workflow and implementation guardrails.
- Treat the README as the single source for shared information. Link to its sections instead of copying requirements, installation layouts, commands, configuration values, or troubleshooting advice here.
- When shared behavior changes, update the relevant README section once and keep these references valid:

  - [Requirements](README.md#requirements) and [Installation](README.md#installation)
  - [Building](README.md#building) and [Build troubleshooting](README.md#build-troubleshooting)
  - [Packaging](README.md#packaging)
  - [Regression checks](README.md#regression-checks) and [Customization](README.md#customization)

## Working on the implementation

- Use this checkout as the implementation source of truth. If the user names an external local reference implementation, inspect that checkout; do not substitute a Git remote for it.
- Preserve unrelated working-tree changes. Do not commit, push, deploy, or launch/stop the game unless the user requests those actions.
- Before changing hooks or calling conventions, verify the actual SDK/game behavior. Keep hook installation single-shot through `source/Main.cpp` and texture ownership within the RenderWare initialization/shutdown lifecycle.
- Preserve the compatibility described in [Customization](README.md#customization). When adding keys, update the enum, key-code mapping, texture-name mapping, and asset coverage together; respect sprite-capacity assertions and both native/GInput sprite layouts.
- For layout changes, keep measured widths and drawing dimensions consistent. Buffered drawing must use the size chosen during layout, even if menu or font state changes before drawing.
- Preserve the lookup policy in [Texture lookup and troubleshooting](README.md#texture-lookup-and-troubleshooting). Use the stream overload of `CTxdStore::LoadTxd`; its filename overload can retry a failed file open indefinitely.
- On every texture-load path, close streams, balance current-TXD push/pop, and release bound sprites before removing their dictionary. Publish loaded state only after validating every required texture and raster.
- Extend the existing real-source regression harness rather than duplicating production logic. Keep stubs faithful to the SDK, including `CRect` constructor/member ordering.

## Verification and handoff

- For C++ changes, build Debug and Release using the non-deploying workflow in [Building](README.md#building), then run [Regression checks](README.md#regression-checks). Consult [Build troubleshooting](README.md#build-troubleshooting) for environment issues.
- For documentation-only changes, check links, section anchors, and whitespace; a rebuild is unnecessary.
- When producing a package, rebuild the selected configuration first, follow [Packaging](README.md#packaging), and verify the archive layout and file hashes against its inputs.
- Keep generated binaries, packages, and IDE state out of source commits.
- Report which checks actually ran and what remains unverified, respecting the limitations in [Regression checks](README.md#regression-checks).
