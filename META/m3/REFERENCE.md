# M3 reference bring-up — started 2026-10-24

M3 is **SKIPPED by user direction on 2026-10-24**, not complete or passed. Proceed to M4 decompilation and source restoration without satisfying the former M3 capture gate. The material below is historical evidence; its proposed further traces and statements that M3 remains active are superseded. No further automated gameplay or pilot development is authorized; any game execution is limited to a bounded startup test. Missing behavior remains explicitly unvalidated. The user also prohibited computer-use skills and visual identification. No computer-use actions or visual inspection were performed after that instruction. A pre-instruction tool initialization/app listing did not launch or interact with the game.

## Immutable disc and installation evidence

`python tools/m3_media.py --extract-source --write-report` verified the pinned MDF/MDS before/after inspection, read their track table and Mode1/Joliet filesystem, fingerprinted both complete stored track spans and all 1,495 files, and recovered the real `Copyright.txt` into ignored `build/m3/reference-source`. Default mode is read-only; extraction/report writing are explicit switches. It is a bounded reader for this pinned single-file v1.5 descriptor, not a general disc backend or installer.

MDS field meanings were cross-checked against the primary [libMirage MDS structures](https://raw.githubusercontent.com/cdemu/cdemu/master/libmirage/images/image-mds/image-mds.h) and [parser](https://raw.githubusercontent.com/cdemu/cdemu/master/libmirage/images/image-mds/parser.c). The tool independently reads fields and validates actual track/file boundaries; it does not copy library implementation code. Pregaps are descriptor intervals absent from stored MDF spans.

| Track | Type | Stored sectors | Stored duration |
|---|---|---:|---:|
| 1 | Raw Mode1 data, 2352 bytes/sector | 32,175 | 429 s |
| 2 | CD audio, 2352 bytes/sector | 98,716 | 1,316.2133 s (21:56.2133) |

Audio availability is now established outside `Bhunter.bin`. This does not verify decoded byte order, audible playback, MCI compatibility or runtime looping. The media inventory records exact offsets, pregaps, hashes, volume extents and file names. No old DirectX driver/installer was run. No BIP_B1.TXT/BIP_B2.TXT was found in this filesystem; the source-check function constructs those names but does not read them on its reviewed path.

The recovered copyright file is 1,413 bytes, SHA256 `2338bda2a87a5a5fb2468f5e56080382e1a88b64bf92c362c6458dcf0a5307ac`. XOR of its complete little-endian 32-bit words is exactly `6b405d08`, the original expected constant. Its final single byte is ignored by the original algorithm. The local staged data is not added to the source distribution.

## Original source-check execution

`python tools/m3_source_probe.py` compiles/links `tools/m3_source_probe.c` into a separate native console diagnostic. Windows normally loads a byte-identical copy of BHUNTER.EXE. This is **original-backed reference execution**, absent from `bhunter-integration.exe`; it neither expands the math mapper nor counts as a recovered game implementation. No window automation, screenshots, keyboard/mouse injection or visual identification are used.

The diagnostic design and instrumentation are explicit in `DEBUGGER_DESIGN.md`. Two temporary debugger breakpoint bytes in the child are checked/restored; no disk instruction or success branch is patched. The original file-reading/checksum body executes normally. The first original return is observed before WinMain creates its window. Only source-registry call arguments are redirected in the two private-source cases; fresh HKCU keys are removed afterward. Both original HKLM registry views and all immutable reference hashes remain unchanged.

| Case | Original return observed | Early x87 control |
|---|---|---|
| Original registry path absent | Rejected, EAX=0 | 027F before and after |
| Private SourceDir, real recovered copyright | Accepted, EAX=1 | 027F before and after |
| Private SourceDir, one-byte-altered copyright copy | Rejected, EAX=0 | 027F before and after |

`source-probe.json` retains complete compiler/link commands, source/copy hashes, native debugger events, real return values, breakpoint restoration, exact child exit and registry cleanup. Hardware-only attempts timed out because debug registers were cleared; their failed evidence is retained separately and is not successful validation. Probe termination is deliberate and does not establish orderly game shutdown.

Measured 027F establishes PC53/nearest/masked at this early source-check boundary. It does not establish later DirectDraw/driver/render-worker/gameplay precision transitions. A successful checksum is an installation prerequisite, not a playable reference session.

## CD-audio behavior identified, not played

`cd-contract.json` records complete reviewed MCI bodies and the actual caller. The game opens `cdaudio`, selects TMSF format, and the reviewed caller requests **track 2 from 00:00 to 21:50**, with notification/from/to flags and looping enabled. Successful matching-device completion notifications reissue the same range; stop/close clear the loop flag. This matches an available track that is slightly longer than the requested range. These are static instruction/caller findings, not captured playback or audio timing. CD music cannot be silently dropped from restoration.

## Remaining M3 work

`scenario-matrix.json` records uncaptured cases explicitly. Startup and original bootstrap/world-loading boundaries are now captured using the separately pinned DDrawCompat environment; the native display-mode failure is preserved as a control. Next: establish actual menu/control/movement/collision/objective/failure/retry/shutdown traces; inventory real features, persistence, rendering, audio/music and network modes. Capture seed/input/timing/state and thread numerical controls where possible. A static menu string or a returning helper is insufficient scenario evidence.

No M3 exit is claimed yet. A hardware, driver or media failure must be measured and recorded with the missing scenarios; it cannot be converted into a fabricated successful runtime. Original code remains immutable on disk, and M2's completed source-only integration gate remains unchanged.

## Native graphics control and compatibility reference — 2026-10-24

`tools/m3_startup_probe.py` and `STARTUP_DESIGN.md` extend the native debugger without changing the earlier source-check probe. The original EXE/BIN run from a separate local directory. A process-local Windows-directory result adapter keeps real original CFG/INI handling inside an existing private profile directory. Both actual Windows-directory files and original HKLM views are fingerprinted/compared before and after; no persistent system setting is changed. Temporary breakpoint bytes are restored and verified at controlled stops.

The first cold run enumerates **Primary Display Driver**, lets the original write a **644-byte CFG** with three profile records, and captures the exact **3D Acceleration** prompt arguments before MessageBox executes. No dialog, screenshot or visual identification is used. This is now an actual original-produced config specimen, not an invented user CFG.

A private copy of that specimen supplies a documented software-high preset. Native control uses normal window show mode and logs all thirty possible WinMain mode callsites. All six attempted modes fail (640x480x16/8, 320x240x16/8, 320x200x16/8), and the original reaches its **Fatal Error! / No suitable screen mode** path. The first real DirectDraw SetDisplayMode result is **80004001**, E_NOTIMPL in the pinned Win32 header. The complete error is preserved in `startup/62d00616a929.json`. Earlier 15-second timeout trials are retained as insufficient observation, not terminal-mode conclusions or successful reference sessions.

The isolated compatibility control uses [DDrawCompat](https://github.com/narzoul/DDrawCompat), whose maintained scope includes DirectDraw/Direct3D 1–7. Its official [v0.7.2 prerelease](https://github.com/narzoul/DDrawCompat/releases/tag/v0.7.2) archive is pinned against the GitHub asset SHA256 before staging a local x86 ddraw.dll. `compat/ddrawcompat-v0.7.2.json` records exact release/asset/DLL hashes and prerelease status. The wrapper lives only in `build/m3/startup-ddrawcompat`; it is neither a normal source-build dependency nor a selected M6 backend. No original DLL, system file or game file is replaced.

With the same measured CFG settings and the same native probe C variant, the wrapper control returns **S_OK** for 640x480x16, the actual renderer helper returns **1**, and original WinMain reaches **00479200 game-bootstrap entry**. The probe stops before executing that bootstrap. The final current driver case is `startup/e1ee374903ef.json`; the earlier compatible case `startup/dae86876751f.json` remains separate evidence. The real DDrawCompat log identifies v0.7.2 and is preserved under ignored per-run logs, with its hash recorded.

All observed startup/config/mode boundaries remain x87 **027F** in these cases. Renderer initialization success does not prove later driver/render-worker/gameplay precision, visual fidelity, sound or usable input. The normal-show case runs the original window but no agent-side visual/UI tools inspect or control it. GPU/OS metadata is read through CIM in `startup/host.json`.

`python tools/check_m3_startup_evidence.py` audits the archived startup variants, real return values, native failure sequence, compatibility success, config files, private-key cleanup, original/System-file preservation and recorded precision. Later edits do not inherit these runs' evidence.

## Original bootstrap and world loading — 2026-10-24

The separately recorded `--through-bootstrap` variant executes the real bootstrap
and stops at `004733f2` after its actual return of **1**. Final evidence is
`startup/f7a780ff1c4d.json`; the preceding variant is retained separately in
`startup/89994fa7c8f0.json`. Both compile, link and execute the diagnostic, with
79 temporary breakpoint bytes restored/verified and private registry cleanup.
All immutable originals, Windows CFG/INI files and original registry views
remain unchanged. No UI/visual tools or input injection are used.

| Checkpoint | Observed original state |
|---|---|
| DirectSound initialization return | 1, real sound-device pointer present |
| DirectInput initialization return | 0, interface present, joystick-device pointer null |
| World entry | 9,600 entity slots, zero used |
| Block registry return | 1, 454 models, 755 used entities |
| Level load return | 7,796 used entities |
| Navigation return | 283 nodes, EAX=80,089, 7,797 used entities |
| World/bootstrap completion | Return 1, 7,831 used entities |

The DirectInput body enumerates joystick devices and returns zero when no
joystick device has been selected. The observed interface/null-device state is
consistent with that branch; it is not evidence of keyboard/mouse failure or
successful control capture. No joystick input is fabricated.

All captured main-thread stage controls remain **027F**. System-thread creation
events record initial controls only. The game rendering worker has not started
at this boundary, so gameplay/worker precision is still open. The level creates
more entities than its 5,815 placement records; counts reflect actual original
constructors/callbacks rather than an assumed one-entity-per-record mapping.

This variant returns FALSE for three captured SetCursorPos requests (two in
platform/compatibility paths, one from original input setup). The cursor does
not warp, and the adaptation is an explicit limit on input/graphics evidence.
System-wide aux/wave volume writes would stop the probe before execution;
neither guard was reached. Actual DirectSound initialization is not audible
playback, CD-loop validation or a source audio implementation.

`python tools/check_m3_bootstrap_evidence.py` audits these stage values, source
snapshots, actual original returns, private state and reference preservation.
M3 remains active: menus, deterministic gameplay/control/seed/time traces,
objectives/failure/retry, sound/music, network modes and orderly shutdown are
still uncaptured. The probe deliberately terminates its child at bootstrap
return; it does not exercise the game's cleanup path.

## First simulation/render/presentation trace — 2026-10-24

The next bounded native trace creates the actual game rendering worker, completes
the first simulation and render callbacks and observes the original surface
Flip returning **S_OK**. Sampled main-thread simulation and worker render/Flip
controls are **027F**. No pixels, UI input or visual identification are used.
`FIRST_FRAME.md` records full evidence, roles and limitations.

Default heap behavior faults at the original input-binding high-nibble assertion
(`004755ce`), with a valid committed pointer above 256 MiB. A per-child ASLR-only
experiment also fails; its actual queried flags are zero, as in the default
control. An explicit diagnostic HeapCreate initial-size input adaptation from
4 KiB to 128 MiB enables this first-presentation trace while leaving the actual
allocator, binding list, assertion and simulation intact. This is a qualified
reference profile, not an unchanged launch or source implementation. Exact
same-source/same-CFG default and adapted controls are
`startup/b8497ba5a485.json` (failed) and `startup/75fef4730703.json` (successful).

`python tools/check_m3_runtime_evidence.py` audits both controls, the ASLR
hypothesis rejection, source snapshots, actual thread identities/returns,
exception memory and private/original-state preservation. Surface Flip success
does not establish visual fidelity, menu/gameplay acceptance, deterministic
timing, audible playback or normal shutdown. Those full M3 requirements remain
open. No assumption is made that later allocations cannot hit the same limit.

## Actual menu inventory and input navigation — 2026-10-24

The original now completes a 300-callback observation and a separate
600-callback menu-input trace. Its constructed objects expose **18 menus / 118
entries**, including GAME/NETWORK/SETUP/QUIT, BARLETTA/STINGER/VIPER, prologue plus
eight episode labels, video/audio/input settings and network connection pages.
`menu-inventory.json` retains exact runtime labels, links and callbacks. These
are constructed feature options, not execution proof for all listed features.

The current trace posts verified-child-only Enter down/up messages and observes
the original GAME → STINGER → prologue path, including actual episode callback
argument 0 and original flag/index writes. It uses no desktop-wide input,
computer-use skill or visual identification. A process-local relative-cursor
adapter preserves real GetCursorPos success and translates physical deltas to
the recentered frame while desktop warp requests remain denied. Earlier drift,
wrong-default and guarded-stop attempts remain distinct evidence.

`MENU_TRACE.md`, `menu-validation.json` and `python tools/check_m3_menu_evidence.py`
document current run `startup/73beffa6a63f.json`, actual key-array transitions,
source variants, callback sampling and limits. The recorded key schedule is not
a deterministic world/seed/time replay. Mission initialization, player movement,
collisions, objectives/failure/retry, sound/CD loops, network execution and normal
shutdown/reload still require evidence; full M3 remains active.

## M3 bounded player/contact/shutdown evidence — 2026-10-24

The repeated prologue route is consolidated in `PLAY_TRACE.md` and `play-validation.json`. Same-source motion/idle controls record actual acceleration and displacement; the final separately instrumented variant captures a real player collision. Five runs complete original abort/menu/quit, six cleanup returns and natural process exit 0. All references remain unchanged. Further launches stopped after the user flagged repetition. This is original-backed evidence only, no source admission or M3 completion. Objectives/failure/retry, saved-config reload, deterministic world replay, audio/CD looping and network modes remain open. `python tools/check_m3_play_evidence.py` audits retained artifacts without launching the game.
