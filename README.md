# BHunter Revived

Restoring BHunter from recovered source.

## Global progress

**Functional source restoration: 100% — 679/679 inventoried game and engine functions restored and linked.**

This preservation checkpoint contains 601 integration translation units. Its recorded
validation passes **5,529,939 real-asset checks** and **5,023,991 asset-free checks**,
with zero failures and 295 missing-module controls.

The percentage describes the inventoried function restoration. Full-game playability
and fidelity remain incomplete. This branch preserves the integration checkpoint
before the playable-preview changes; it builds a test harness, not a playable game.

## Build instructions

Requires Windows, Python 3.10+, the pinned Microsoft Visual C++ 5.0/SP3 toolchain,
and the pinned DirectX SDK dependencies. Proprietary tools and original game assets
are supplied separately. Exact dependencies are recorded in `META/reference-manifest.json`
and `META/native-*-dependencies.json`.

Configure the toolchain locations when they differ from the repository defaults:

```bat
set BH_COMPILER_DIR=C:\tools\vc5-sp3\bin
set BH_VC_ROOT=C:\tools\vc5\VC
set BH_SP3_ROOT=C:\tools\vc5-sp3\vc
set BH_SHARED_DIR=C:\tools\vc5\SharedIDE\bin
set BH_DIRECTPLAY_SDK=C:\tools\dx7sdk
```

Build and run the source integration checks from the repository root:

```bat
build.cmd
```

To include checks against your original asset archive, place it at
`GAME_DATA/Bhunter.bin` and run:

```bat
build.cmd --archive GAME_DATA/Bhunter.bin
```

The executable and reports are written to `build/m0/`, including
`bhunter-integration.exe`, `result.json`, and the link map. A failed compilation,
link or check returns a nonzero exit code. The original game executable is not
loaded or executed by these builds.
