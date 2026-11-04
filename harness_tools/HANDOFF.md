# Handoff: Windows portability milestone 2

Branch: `codex/portable-text-regression`. Baseline: `b59b3b5` (milestone 1 accepted).
Read `AGENTS.md`, `harness_tools/WINDOWS_PORTABILITY_PLAN.md`, and
`harness_tools/PORTABILITY_REGRESSION.md` before continuing. README is unchanged.

## Current state

Milestone 2 replaces runtime GDI text and DIB/DC operations with portable bitmap
composition. `tools/bake_comic_font.py` bakes installed Comic Sans MS using the
original 24-pixel CreateFontA parameters (quality 3, non-antialiased). Original
bearings, advances, clipping and command colors are preserved. The earlier
retail pixel-font substitution was visually rejected and has been superseded.
The user accepts minor differences if the overall appearance is similar.
No new shader or SDF is required. Menus/HUD/credits retain their existing atlas.

The manual arena has two parked real vehicles with native TARGET/SUSPECT HUD
labels. Fixtures, event injection and environment switches exist only in
harness-generated game copies. Production source has no new temporary diagnostic
hooks. User answered "No further cleanup"; keep the framerate profiler.
User accepted milestone 2 on 2026-11-05 after 4K arena and ordinary-game
playtesting, including comparison with the original GDI renderer. Follow
AGENTS.md: no computer use or automated screenshots.

## On another Windows PC

Requirements: Python 3, 32-bit MinGW-w64 GCC/G++ (`i686-w64-mingw32` target),
clang-format, installed Comic Sans MS (`C:/Windows/Fonts/comic.ttf`), and working
DX12 graphics/audio. Install Pillow with `python -m pip install Pillow` for the
offscreen font comparison. Put GCC/G++ and clang-format on PATH. `BH_GCC` and
`BH_GXX` may specify compiler paths. SDL3 is fetched automatically with a pinned
archive hash. No VC5 toolchain is needed.

Copy your own `Bhunter.bin` into `GAME_DATA/Bhunter.bin` (or `build/GAME_DATA/`).
Retail media, generated font data, toolchains, builds, settings, and old local
research are intentionally excluded from Git. Music for the gate is generated;
normal gameplay music can be copied to `MUSIC_DATA/track02.wav` separately.
Run from the repository root:

```powershell
python tools/build_game.py --output-dir build --debugging --symbols
python harness_tools/portability_regression.py --suite
python harness_tools/portability_regression.py --manual
```

Close a manual arena before rebuilding. Main player output is directly in
`build/BHunter-source.exe`; tests and release checks belong under
`harness_tools/output/`. Do not reintroduce nested player build directories.
All required gate fixtures are included; no ignored `tests/` folder is needed.
The three-room scene, textures and synthetic tones are generated from the
tracked scripts. Game models and other retail assets come from your archive.
The installed font is baked locally; it is not redistributed on this branch.

## Validation and next action

Debug and release builds passed. The complete gate passed 76 normal replay
checks, the expected dropped-key-release negative control, 13 subsystem suites,
12 original-vs-portable offscreen text comparisons (zero differing RGB pixels),
and 10 stationary-vehicle checks. After packaging self-contained fixtures, all
13 subsystem suites passed again. This does not certify visible GPU output,
audible quality, real hardware or every mission behavior.

The follow-up fixed NPC message glyph backgrounds: preserve source pure black
in font atlases instead of raising it to RGB (8,8,8) under the world-texture
palette policy. Shaded edges remain unchanged. A real-atlas probe reproduced
the old failure and passed the fix across three palette policies and 16/32-bit
surfaces. Debug/release builds and the complete gate passed again. Harness
cache lookups now resolve Windows path casing consistently.

Milestone 2 is accepted; milestone 3 (sound effects and music) is next in the
plan, when requested. The local profiler comparison showed similar frame
cadence, but concurrent compilation makes it unsuitable as a controlled
performance benchmark. Preserve the profiler.

Only the selected reusable gate sources/fixtures are force-tracked inside the
otherwise ignored `harness_tools/`. Keep new generated outputs ignored. Do not
push local media, executables, caches or unrelated historical investigations.
