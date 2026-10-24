# Source integration build

On Windows x64 with Python 3.10+ on PATH, run from any working directory:

```cmd
D:\Decomp\Bhunter\build.cmd
```

This compiles the admitted source families with their integration drivers, then links and executes `build/m0/bhunter-integration.exe`. The current 132-TU source-only scenario passes 2,906,329 checks. All 66 missing-module controls require LNK2001, including the new lighting, clipping, native rendering and BSP traversal owners. The pinned original math fixture remains verbatim; only its test-driver `main` is renamed at compile time. No original EXE is read, mapped or executed. Every stage must succeed; dependency mismatches, missing inputs, compile/link failures and failed fixtures return nonzero. Output stays under ignored `build/m0/`: `result.json`, `link.txt` and `bhunter-integration.map` retain commands/results and owning symbols.

The portal matcher adds 57,578 source-only checks against 3,739 previously captured aperture records and synthetic ordering/threshold cases; no game launch is used. See [portal restoration evidence](META/m4/PORTALS.md).

Normal builds generate isolated synthetic archive/loose-file fixtures in the build directory and need no game files. To additionally compare all 1,229 directory records and every asset payload byte through the source reader, supply the preserved archive as data:

```cmd
build.cmd --archive GAME_DATA/Bhunter.bin
```

The runner verifies the supplied BIN's preservation hash before and after this scenario. Six representative asset classes also pass through preloaded memory, and real TXT assets exercise both archive and cached line reading. The scene scenario additionally decodes every TEST.LEV placement/link and TEST.PTH route cell through embedded, preloaded and loose backends, comparing all fields with independent raw reads. The current real-data combined run passes 3,398,563 checks. All 505 BSPs additionally pass bounded geometry decoding and derived-bound controls through three backends. The 454-model data registry retains 202 sidecars and resolves all level placement names. All 459 PCXs decode through three backends with independent raw pixel/palette and four palette/index transform controls. The separate image-owner scenario constructs 454 supported native-backed images and verifies five original size-catalog rejections. See [PCX contract](META/m2/PCX.md). All 60 WAVs additionally decode through three backends with exact PCM metadata/sample comparisons; no playback objects are created. See [WAV contract](META/m2/WAVE.md). The entity-pool scenario also exercises all 5,815 placement records in the original 9,600-slot arena. These tests do not construct the full original world, navigation graph, scripts or scene-wide texture/audio objects. See [entity contracts](META/m2/ENTITIES.md) and the [M2 exit audit](META/m2/EXIT_AUDIT.md). See [BSP/registry contracts](META/m2/BLOCKS.md). See [scene reader contracts](META/m2/SCENE.md). See [VFILE contracts and evidence](META/m2/VFILE.md).

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

Historical menu/gameplay automation commands have been withdrawn by the user.
Do not run menu, tutorial, flight, combat or network replay options. Existing
M3 artifacts and read-only audit tools remain historical evidence; M3 is skipped,
not passed. Human playtests may answer focused questions. Automated game
execution remains limited to a bounded startup test.

Native DirectPlay transport checks additionally use the locally installed official
DirectX SDK declarations, pinned in `META/native-directplay-dependencies.json`.
`BH_DIRECTPLAY_SDK` (or `tools/m0.py --directplay-sdk`) selects its root, default
`D:/Decomp/dx7sdk`. Only the transport and its test TU get that include path;
existing VC5 comparison/header contexts stay pinned. `ole32.lib` comes from the
existing VC5 dependencies. Native checks create an unopened DirectPlay3A COM
interface and exercise Send failure paths without a connection or session.

## Static source-built scene view

```powershell
python tools/scene_snapshot.py --archive GAME_DATA/Bhunter.bin
python tools/check_view_camera.py
python tools/check_clipper.py
python tools/check_primitive_queue.py
python tools/check_model_owner.py
python tools/check_image_owner.py
python tools/check_sound_tick.py
```

The separate `build/scene-snapshot/bhunter-scene-snapshot.exe` target loads the
real TEST.LEV/BSP data and saves `build/scene-snapshot/scene.bmp` using a private
native DirectDraw/Direct3D2 RGB render target. It has no window, input or gameplay
loop and executes no original game code. Run the build tool without `--archive`
to build and execute 28 native depth/pixel/clipping/lifecycle checks and 46 texture
ownership/UV/palette checks without game files. Additional SDK headers/import libraries are pinned in
`META/native-render-dependencies.json`, scoped to the new adapter TUs only.
`--sdk` or `BH_DIRECTPLAY_SDK` selects the existing local SDK root.

The current view renders portal-bearing world shells with 146 real textures and
raw BSP edge UVs, using a fixed diagnostic camera/FOV. It excludes props and
invisible entity proxy meshes; 202 sidecars remain unapplied. Six original
texture format/residency owners link into both targets, alongside four complete
image/cache/software-palette owners. The scene uses the recovered image loader.
Two complete original BSP/model owners now supply actual owned geometry and
material/image bindings. Original primitive consumers, visibility/lighting and
playable M4 completion remain unfinished; shell selection and drawing are adapters.
Original HAL slot capabilities remain in source; the native RGB diagnostic
explicitly selects system-memory slots. Official render GUID declarations are
instantiated once in `src/bh_render_guids.c`, avoiding the aggregate DXGUID.LIB
object conflicting with existing official DirectPlay GUID definitions.
The native renderer requires no compatibility wrapper on the tested host.

Deferred sound update checks use silent PCM in real secondary buffers; their
native Play calls establish API/state behavior, not audible gameplay acceptance.
The default integration target still never loads or executes original game code.

Current image recovery audit: `python tools/check_image_owner.py` verifies four
complete bodies (1,726 original instructions), current linked source hashes,
131 synthetic checks, 454 supported native image loads, five catalog rejections,
40 missing-module controls and unchanged composed scene pixels.

Historical texture recovery audit: `python tools/check_texture_owner.py` verifies
six complete body fingerprints (543 original instructions), exact nine-class
format data, source hashes, native controls, linked targets and original hashes.
`python tools/check_scene_view.py` audits the retained historical static-color
variant; its `--clean` option refuses to overwrite that historical evidence.

Current model recovery audit: `python tools/check_model_owner.py` verifies two
complete bodies, the original 0x80 model layout contract, all 505 BSP models,
8,039 focused real-data checks / 63 synthetic source-only checks, 41 missing
module controls, independent raw arrays/root bounds, exact linked source hashes
and unchanged scene pixels. Model cleanup adapters do not claim recovery of an
original destructor. Image failure retains NULL material bindings as original;
source guards reject malformed data and clean partial ownership safely.

Current primitive-buffer audit: `python tools/check_primitive_queue.py` verifies
complete vertex-growth and native fan-flush owners (132 original instructions),
61 focused source/native checks, current linked hashes and the real scene. The
scene uses recovered image run queues with Direct3D2 triangle fans/flags0x0c.
A retained diagnostic control changing only the native call back to repeated
triangle lists reproduces the preceding bitmap exactly; the recovered fan path
changes 1,421 pixel colors with unchanged coverage. Full original linked-ring
production, clipping and software rasterization remain unfinished; projected
TL-vertex composition remains an explicit adapter.

General affine inverse audit: `python tools/check_matrix_inverse.py` verifies
three complete bodies (389 original instructions),3,380 source checks,43 missing
module controls, exact storage fixtures and independent Gauss-Jordan oracle
cases in both real-data and clean builds. This is the projected-texture
prerequisite for00452140; clipping itself remains unfinished.

Current clip/project audit: `python tools/check_clipper.py` verifies complete
00452140 (1,511 original instructions),1,139 source checks,44 missing-module
controls and the native scene using its real linked-ring implementation. The
independent oracle covers256 triangles; a fixed six-plane fixture uses all12
scratch records. Both colored/textured polygons use it. Viewport/camera and
TL-depth bindings remain diagnostic. The prior adapter bitmap is retained;
2,695 colors/three coverage locations change, with397 triangles and146 textures.
The general inverse paragraph above records the preceding prerequisite milestone;
clipping itself is now recovered.

Current camera/frustum audit: `python tools/check_view_camera.py` verifies three
complete owners (421 original instructions),1,623 source checks and45 missing
module controls. The actual scene uses recovered projection construction with
the pinned original60-degree FOV/near1/far100000 defaults, emitting5,881 triangles
and146 textures. Native analytic controls retain their explicit90-degree profile.
Camera choice, world visibility/lighting and original native/software producer
closure remain unfinished; no playable source game or original-game visual
equivalence is claimed.

Git preserves byte-pinned source snapshots/raw decompiler exports and the native
render SDK manifest without text normalization; historical evidence keeps its
original source hashes across checkouts.

Historical model-space light preparation audit: `python tools/check_light_prepare.py`
verifies complete00450980 (257 original instructions),991 focused checks and46
missing-module controls in the91-TU integration. Current full checks pass664,011
with real assets and171,777 in a clean checkout. Original0x5c light layout,
range/radius/cone behavior, borrowed-list caches and override reference semantics
are tested. Shading evaluation, light construction, visibility and native/software
primitive closure remain unfinished; the static scene output is unchanged.

Parallel restoration batch audit: `python tools/check_parallel_hour.py` verifies
31 complete original owners against immutable EXE bytes, current source hashes,
real and clean integration evidence, linked symbols and the static native scene.
The current target has 132 translation units, passes 3,398,563 real-data checks
or 2,906,329 checks without original assets, and rejects all 66 missing-module
controls. The earlier checkpoint totals above remain historical.

The scene target now uses recovered main-camera setup, lit/unlit BSP traversal,
clipping, material/render state and native queues. Its default static world shell
emits 1,798 polygons and 3,112 triangles with 146 textures. A separate 71-check
native composition test verifies coloured output and blank negative controls.
After building the scene target, its executable accepts an optional static camera
and diagnostic lighting preset:

```cmd
build\scene-snapshot\bhunter-scene-snapshot.exe GAME_DATA\Bhunter.bin build\scene-snapshot\red.bmp -60 -17 60 180 0 0 red
```

Presets are `red`, `green` and `point`. These exercise recovered light evaluation;
they do not reconstruct original level lighting. The native primitive adapter
covers only the hardware branch; complete software dispatch/rasterizers, atlas
glyph copy and playable world/entity/script/control orchestration remain open.
These tests render source-built offscreen images and never launch the game.
