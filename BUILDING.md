# Source integration build

On Windows x64 with Python 3.10+ on PATH, run from any working directory:

```cmd
D:\Decomp\Bhunter\build.cmd
```

This compiles the real math/FP, archive/VFILE, config/path, scene, BSP/block-data, entity pool, WAV/PCM and PCX/palette data, portal matching, and numerical-storage translation units with their integration drivers, then links and executes `build/m0/bhunter-integration.exe`. The default source-only scenario passes 93,709 checks, including 786 BSP/registry and 819 entity, 226 WAV and 2,098 PCX checks. Omitted math, VFILE, config, paths, scene, BSP, blocks, round-store, entity, WAV, PCX and portal modules must each produce LNK2001. The pinned original math fixture remains verbatim; only its test-driver `main` is renamed at compile time. No original EXE is read, mapped or executed. Every stage must succeed; dependency mismatches, missing inputs, compile/link failures and failed fixtures return nonzero. Output stays under ignored `build/m0/`: `result.json`, `link.txt` and `bhunter-integration.map` retain commands/results and owning symbols.

The portal matcher adds 57,578 source-only checks against 3,739 previously captured aperture records and synthetic ordering/threshold cases; no game launch is used. See [portal restoration evidence](META/m4/PORTALS.md).

Normal builds generate isolated synthetic archive/loose-file fixtures in the build directory and need no game files. To additionally compare all 1,229 directory records and every asset payload byte through the source reader, supply the preserved archive as data:

```cmd
build.cmd --archive GAME_DATA/Bhunter.bin
```

The runner verifies the supplied BIN's preservation hash before and after this scenario. Six representative asset classes also pass through preloaded memory, and real TXT assets exercise both archive and cached line reading. The scene scenario additionally decodes every TEST.LEV placement/link and TEST.PTH route cell through embedded, preloaded and loose backends, comparing all fields with independent raw reads. The real-data combined run passes 434,189 checks. All 505 BSPs additionally pass bounded geometry decoding and derived-bound controls through three backends. The 454-model data registry retains 202 sidecars and resolves all level placement names. All 459 PCXs decode through three backends with independent raw pixel/palette and four palette/index transform controls; no surface objects are created. See [PCX contract](META/m2/PCX.md). All 60 WAVs additionally decode through three backends with exact PCM metadata/sample comparisons; no playback objects are created. See [WAV contract](META/m2/WAVE.md). The entity-pool scenario also exercises all 5,815 placement records in the original 9,600-slot arena. These tests do not construct the full original world, navigation graph, scripts or texture/audio objects. See [entity contracts](META/m2/ENTITIES.md) and the [M2 exit audit](META/m2/EXIT_AUDIT.md). See [BSP/registry contracts](META/m2/BLOCKS.md). See [scene reader contracts](META/m2/SCENE.md). See [VFILE contracts and evidence](META/m2/VFILE.md).

Config tests exercise default/custom binary records, circular profile ownership, failure rollback, device selection and bounded paths, including real read-only Win32 directory discovery. Persistence and composed VFILE fixtures stay under the build directory; machine configuration is not changed. The [config/path contract](META/m2/CONFIG.md) records interface adaptations and the limits of the legacy pointer-containing format. Use explicit config paths; original global zero-argument interfaces are not drop-in wrappers.

This is a source-only integration harness. Its numerical boundary explicitly establishes/verifies/restores x87 PC53, nearest rounding and all masked exceptions through `bh_fp.c`. The 4,032 original-derived golden results are numerical data, not original executable code. Exact finite/zero/infinity bits, NaN classes, supported aliases and numerical-environment controls are checked. NaN payload/sign, PC24 inverse-point behavior and original driver/thread transitions remain limited; see [math contract and evidence](META/m2/MATH.md). No matching C++ source is linked.

For the regression negative control, run `build.cmd --math-regression-control` (also accepts `--archive`). It separately compiles the archived pre-correction math source, links it with the current fixtures and requires numerical rejection while VFILE checks pass. The positive target remains intact.

## Local prerequisites

The compiler and SDK are proprietary local dependencies and are not distributed in Git. Supply the same stock VC5 installation and extracted Visual Studio 97 SP3 files as the preservation manifest. The source build verifies hashes before compilation. Root locations can change; file bytes cannot change without a reviewed manifest update.

| Environment variable | Default local directory | Contents used |
|---|---|---|
| `BH_COMPILER_DIR` | `analysis/toolchain-sp3/bin` under checkout | Prepared compiler and helper binaries below |
| `BH_VC_ROOT` | `D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC` | Stock include/lib trees and `bin/LINK.EXE` |
| `BH_SP3_ROOT` | `D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc` | SP3 include/lib overlay, searched first |
| `BH_SHARED_DIR` | `D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN` | VC5 helper DLLs |

To prepare a new compiler directory, copy stock `VC/bin/CL.EXE`; copy `c1.dll`, `c1xx.dll`, `c2.exe` and `link.exe` from the SP3 extraction's `VS97_SP3/enu/vc/bin`; copy `MSDIS100.DLL`, `MSPDB50.DLL` and `MSVCP50.DLL` from stock `DEVSTUDIO/SHAREDIDE/BIN`. Preserve the SP3 `all/vc/include` and `all/vc/lib` overlay separately from the stock trees. Use the supplied manifest to check exact hashes, including helpers. Set the variables above if these directories differ. Equivalent command-line overrides are listed by `python tools/m0.py --help`.

Declared combination: CL driver 11.00.7022; C frontend 11.00.7113; C++ frontend/backend 11.00.7303; stock LINK 5.00.7022. Integration flags are `/ML /O2 /G5 /Oi /Gy`; these are a scoped historical integration baseline, not a modern game compiler choice. INCLUDE and LIB are constructed explicitly, and inherited CL/_CL_/LINK/_LINK_ options are removed. The historical C++ matching flags and source/header arrangement remain unchanged.

SP3 LINK 5.010.7303 exits `0xC0000139` (entry point not found) on this host before emitting link diagnostics. `python tools/m0.py probe-sp3-link` reproduces and records the failure in `build/m0/sp3-link-probe.json`; a nonzero result is expected on this host. This diagnostic is separate from the working stock-linker target. The exact missing DLL export has not been established.

## Preservation and optional checks

`META/reference-manifest.json` records the four original EXE/BIN/MDF/MDS files, supplied compiler/helpers, complete SDK include/lib trees and the integration/differential/matching source fixtures. This broad SDK snapshot covers the headers and libraries that the current harness needs; it does not establish the original game's full SDK provenance. System Windows DLLs and the Python interpreter are host prerequisites, not bundled historical dependencies. Build results record host/Python versions and source hashes.

```powershell
python tools/m0.py verify-reference
python tools/check_m0_clean.py
python tools/check_m0_clean.py --evidence-dir META/m2/pcx-clean
python tools/check_config_evidence.py
python tools/check_scene_evidence.py
python tools/check_blocks_evidence.py
python tools/check_entities_evidence.py
python tools/check_wave_evidence.py
python tools/check_pcx_evidence.py
python tools/check_m2_exit.py
python analysis/astra-review/verify_fixed_math.py
```

Reference verification requires all four user-supplied original files in their manifest locations. Normal `build.cmd` requires none. The clean check needs Git and the local compiler prerequisites; it uses a temporary index to check out the current source tree into a directory with spaces, with no original files or compiler inside it, and runs the build from a different directory. It leaves the user's index unchanged. Select a new `--evidence-dir` for later milestones so M0's historical results remain intact. The tree ID identifies the tested source snapshot, including uncommitted files.

The existing optional byte-match verifier retains its pinned local paths and requires the original EXE plus its prepared compiler/header context. It builds the combined C++ source twice and preserves the direction-only context difference. The expanded optional oracle is `python tools/math_oracle.py --label verification`. It requires the original EXE and reuses the unchanged pinned `analysis/math/differential.c` mapper for the same ten pure math functions only. The explicit `--golden tests/math_golden.h` option regenerates numerical fixture data and should be used only when deliberately reviewing that capture. The historical `powershell -File analysis/math/build_and_test.ps1` experiment remains separate. None of these original-backed diagnostics is part of the source-only build.

`record-manifest` is a one-time preservation operation and refuses to overwrite an existing manifest. Do not refresh hashes to hide reference or toolchain changes. Historical research records remain unchanged.

## M3 original-reference diagnostics (opt-in)

These commands are separate from the source-only integration build and require the preserved local media/EXE:

```powershell
python tools/m3_media.py --extract-source --write-report
python tools/m3_source_probe.py
```

The media tool reads/fingerprints the pinned MDF/MDS and stages real source markers under ignored `build/m3`. The native probe compiles/links its own console helper and executes a byte-identical original EXE copy through Windows debugging APIs. It uses two temporary, restored process breakpoint bytes, redirects the source-registry call to a fresh private HKCU key for two cases, observes the actual checksum return and terminates before WinMain UI creation. It verifies original file hashes and registry cleanup. It does not enter the integration target, automate Windows UI, capture screenshots, patch disk references or supply a playable source game. Hardware-only failed attempts and the revised [diagnostic design](META/m3/DEBUGGER_DESIGN.md) are explicit. SourceDir baseline currently requires the original installation key to be absent; the driver refuses to modify an existing installed configuration. See [M3 progress/limits](META/m3/REFERENCE.md).

The expanded M3 startup diagnostic captures real device/config handling and stops before its first prompt:

```powershell
python tools/m3_startup_probe.py
```

To reproduce the graphics comparison, use the original-written seed CFG named in `META/m3/startup/c57437263011.json` (the local build files must be present). Native Windows currently rejects its six software modes. The pinned compatibility experiment reaches game-bootstrap entry:

```powershell
python tools/m3_prepare_ddrawcompat.py
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat
python tools/check_m3_startup_evidence.py
```

The preparation command downloads only the pinned official v0.7.2 prerelease asset and checks its published SHA256 before staging the x86 DLL. `--ddrawcompat` uses a separate local reference directory; omitting it requires the native control directory to contain no wrapper. `--show-reference` uses normal original show mode; no UI inspection/input is performed. The path adapter redirects only the diagnostic process's Windows-directory result to a real private profile folder. Neither graphics results nor game logic are substituted. The probe terminates at an actual message boundary or before game bootstrap; it does not establish a playable reference session or orderly shutdown. See the [startup design](META/m3/STARTUP_DESIGN.md).

To extend the original through actual bootstrap/world loading, with cursor-warp
failure and stop-before-system-volume-write guards:

```powershell
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat --through-bootstrap
python tools/check_m3_bootstrap_evidence.py
```

This original-backed variant stops at bootstrap return, before menu/gameplay and
the game rendering worker. Recorded loading counts do not establish playable
reference acceptance, audible playback, input fidelity or normal shutdown.
Every run stores its exact source snapshots and private profile separately.

To reproduce the qualified first simulation/render/surface-Flip trace:

```powershell
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat --through-first-frame --crt-initial-heap-mib 128
python tools/check_m3_runtime_evidence.py
```

The last flag adapts only the original CRT HeapCreate initial-size argument;
actual allocation/free APIs and the original pointer assertion still execute.
Without it the recorded default profile fails at the 28-bit binding-pointer
assertion. This trace ends after the first actual Flip result and does not
establish visual fidelity, menu interaction, gameplay or shutdown. See the
[first-frame controls and limits](META/m3/FIRST_FRAME.md).

Bounded menu-object observation and child-local key replay are separate options:

```powershell
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat --observe-callbacks 300 --crt-initial-heap-mib 128
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat --menu-trace prologue --relative-cursor --crt-initial-heap-mib 128
python tools/check_m3_menu_evidence.py
```

Replay verifies the child HWND owner for each PostMessage key event. It uses no
desktop-wide input or computer-use/visual tooling. Relative-cursor mode preserves
actual GetCursorPos success and translates observed deltas in the game's stack
output. The original menu/episode callbacks execute. This remains a qualified
reference diagnostic, not a source game, full gameplay replay or normal shutdown.
See [menu trace and unsuccessful controls](META/m3/MENU_TRACE.md).
