# Windows dependency migration plan

## Goal and scope

Move game-owned platform behavior from Windows APIs to SDL or portable C/C++,
while preserving gameplay, saved settings, audio behavior, and rendering.
SDL's Direct3D 12 backend remains in place.

The first target is a Windows build whose game code no longer requires native
Windows services outside an isolated backend. A native build for another OS and
a 64-bit build are later milestones, not implied results of switching to GCC.

This plan is based on source inspection and imports from the existing executable
on 2026-11-04, followed by the regression gate described below. Imports establish
linked dependencies, not proof that every imported function executes in normal
play. Confirm reachability before deciding that legacy code can be removed.

## Current baseline

- `tools/build_game.py` delegates to the 32-bit MinGW-w64 GCC build.
- SDL owns the window, GPU presentation, frame pacing, and gamepad input.
- The adapter still forwards Windows messages and exposes a native window handle.
- Active startup invokes DirectSound, DirectInput force feedback initialization,
  and DirectPlay provider startup.
- DirectDraw and legacy D3D operations route through the SDL adapter, but Windows
  SDK types and constants remain part of the source interfaces.
- GDI text uses native fonts and a DIB/DC bridge inside the adapter.
- Music uses MCI/WinMM, including playback of the staged WAV track.
- The adapter directly calls `D3DCompile`; this is separate from SDL's DX12 calls.
- The engine enforces 32-bit layouts and relies on an x87 precision profile.

Useful starting points:

| Concern | Source |
| --- | --- |
| Application ownership and main loop | `src/bh_game_app.c` |
| Entry point and working directory | `src/bh_game_main.c` |
| SDL implementation and native message hook | `src/bh_sdl_gpu.cpp` |
| Legacy graphics interface | `src/bh_sdl_adapter.h` |
| Input devices and snapshots | `src/bh_input_devices.c`, `src/bh_input_collection.c`, `src/bh_input_messages.c` |
| Sound device and voice management | `src/bh_sound_startup.c`, `src/bh_sound_queue.c` |
| Music and volume | `src/bh_cd_audio.c`, `src/bh_cd_notify.c`, `src/bh_media_control.c` |
| Text | `src/bh_gdi_text.c` |
| Networking | `src/bh_provider_startup.c`, `src/bh_network_sessions.c`, `src/bh_network_transport.c` |
| Paths and legacy discovery | `src/bh_paths.c`, `src/bh_source_files.c` |
| Floating-point environment | `src/bh_fp.c` |
| Build and dependency staging | `tools/build_gcc.py`, `tools/prepare_sdl3.py` |

## Recommended approach

Make one subsystem migration at a time. Keep recovered gameplay behavior intact
and change the platform boundary beneath it. Avoid a repository-wide rename of
Windows types before the active runtime has portable replacements.

Keep the current 32-bit target during this work. Preserve the framerate profiler
and its measurements, migrating its clock implementation when necessary.

Distinguish three kinds of dependency in each change:

1. Native calls used by the active game.
2. Legacy implementations still compiled or linked, but unused by the active path.
3. Windows services used internally by SDL, the compiler runtime, or the retained
   DX12 backend.

The final Windows executable will still import Windows libraries through its
runtime and SDL. Acceptance should measure game-owned calls and SDK exposure,
not demand an empty Windows DLL import list.

## Required regression gate: Available now

An isolated playable three-room arena and automated regression suite are now
available. Run the gate before a subsystem migration to establish its baseline,
and after the change before accepting the milestone:

```powershell
python harness_tools/portability_regression.py --suite
```

The initial gate passed on 2026-11-04:

- 76 full-game replay checks passed across 900 simulation ticks.
- The negative control deliberately dropped a key release and detected exactly
  the expected `keyboard-release` failure, with process exit 90.
- All 13 subsystem contract suites passed.
- Source hashes confirmed production `src/` remained unchanged.

Coverage includes scripted menu navigation and text editing/profile commit,
movement and key release, repeat suppression, mouse buttons, synthetic focus
loss, pause/resume, positioned sound pan and pitch, game sound cues, persistent
voice updates/stops, overlapping requests, music completion/repeat, text command
execution, configuration roundtrip, aspect changes, and the math environment.
Contract tests add input/gamepad bindings, packet codecs, WAV/RIFF and archive
handling, sound ownership, and timing/interpolation checks.

**Acceptance:** the normal replay and contracts must pass, and the negative
control must still detect its intended failure. Retain the behavior assertions
when replacing backends; adapt input injection and playback observations to the
new real boundary rather than keeping a test-only production path or weakening
checks to obtain a pass. Add milestone-specific tests for behavior outside this
gate's coverage.

Manual playtesting remains required for appearance, audible quality, actual
focus/minimize delivery, window modes, hardware reconnect, force feedback, and
ordinary mission/combat behavior. Synthetic focus loss and codec checks do not
certify OS event delivery or multiplayer interoperability. Launch the manual
arena with:

```powershell
python harness_tools/portability_regression.py --manual
```

See [the harness guide and manual checklist](PORTABILITY_REGRESSION.md)
for the cue schedule, coverage limits, and documented adaptations to older
fixtures. Close a running regression game before rebuilding or running the gate.
Selected harness sources and fixtures are tracked on this branch. Generated
outputs remain ignored under `harness_tools/output/`. See HANDOFF.md for setup.

Results are stored under `harness_tools/output/portability-regression/`:
`suite-results.json`, normal and negative replay reports/event logs, and
`contracts/results.json`. Keep the existing framerate profiler. All new scenario
hooks live in generated harness copies; no diagnostic hooks were added to `src/`.

## Milestone 0: Establish a reproducible baseline

- [ ] Record the commit, compiler versions, build options, asset identities, and
  executable/adapter import lists in git-ignored investigation output.
- [ ] Trace active startup, shutdown, and subsystem ownership from `bh_game_app.c`.
- [ ] Identify legacy bootstrap and worker functions that remain linked; document
  why each is retained before removing anything.
- [ ] Record baseline behavior for input, music, sound effects, video transitions,
  focus changes, save/load, and shutdown.
- [x] Establish the automated regression arena, negative control, and subsystem
  contract gate. Manual baseline playtesting remains outstanding.

Build release and debugging variants into separate directories:

```powershell
python tools/build_game.py --output-dir build/portability-baseline
python tools/build_game.py --output-dir build/portability-baseline-debug --debugging --symbols
objdump -p build/portability-baseline/BHunter-source.exe
objdump -p build/portability-baseline/BHunterGPU.dll
```

These are baseline commands to run during implementation; their inclusion here
does not mean they have been executed for this plan.

**Exit criteria:** reproducible builds, an ownership/dependency inventory, and a
manual baseline against which behavior changes can be judged.

## Milestone 1: Input, focus, clocks, and application startup

- [x] Replace `SDL_SetWindowsMessageHook` input forwarding with SDL events.
- [x] Translate SDL keys/buttons into the existing input snapshot and binding
  representation before considering a new settings format.
- [x] Replace native cursor, window focus, minimization, and coordinate queries.
- [x] Replace game-owned tick, sleep, and profiler clock calls with SDL-backed
  functions that have explicit units and wrap behavior.
- [x] Introduce a portable entry point and explicit asset/settings paths.
- [x] Stop requiring a native window handle in application and input interfaces.
- [x] Keep the music notification bridge until milestone 3 replaces MCI.

Implementation on 2026-11-04 is ready for user playtesting. The complete gate
passed: 76 normal replay checks, the expected dropped-release failure only,
and all 13 contract suites. Additional checks passed 136 SDL event/clock
assertions and six actual-entry-point argument/path cases. Test injection uses
SDL events; native input forwarding and the inactive native frame callback were
removed from the active application. The Windows message hook forwards only MCI
notifications. DirectSound/DirectInput still borrow native handles through the
explicit legacy adapter boundary pending their scheduled migrations.

`main` accepts `--asset-dir` and `--settings-dir`; both default to the executable
directory. Relative explicit roots resolve against the launch cwd before asset
cwd selection. Settings, video options, and profiler CSVs use the settings root.
The 32-bit target, saved binding IDs, 30 Hz simulation, SDL DX12 backend, and
framerate profiler remain. Manual focus/window/input and ordinary mission checks
are outstanding; this milestone is not yet accepted as playtested.

Local evidence: `harness_tools/output/portability-step1/` and the current
regression output. Release/debug playtest builds are under
`build/portability-step1/` and `build/portability-step1-debug/`.

**Recommendations:** retain current mouse behavior first; introduce relative
mouse input separately if desired. Keep keyboard layout translation, text entry,
and gameplay bindings distinct. Preserve the 30 Hz simulation/interpolation
contract and reset timing state after focus loss.

**Concerns:** virtual-key versus scancode differences, saved binding compatibility,
Alt+Enter handling, repeat events, stuck keys after focus changes, high-DPI mouse
coordinates, and timer truncation or wraparound.

**Automated checks:** feed press/release/repeat and focus transitions into the
translation boundary; verify existing binding IDs, snapshot state, and timing
behavior at wrap boundaries and after long pauses. Exercise argument parsing and
asset lookup independently of the process working directory.

**Manual checks:** keyboard and gamepad control, rebinding, text entry, mouse
controls where supported, Alt+Enter, resize, minimize/restore, and switching away
while holding a key. Confirm no stuck input or simulation catch-up burst.

**Exit criteria:** active input, focus, and startup no longer require Win32;
any remaining message bridge exists solely for the scheduled audio migration.

## Milestone 2: Portable text and surface access

- [x] Inventory all GDI text command producers and their visible uses.
- [x] Choose an atlas-based or other portable font path with sufficient glyphs.
- [x] Replace `CreateFontA`/`TextOutA` execution while preserving placement,
  clipping, command ordering, and color interpretation.
- [x] Remove the adapter's DIB/DC bridge once no active caller requires it.

The portable text path now bakes the installed Comic Sans MS font at build time
using the original 24-pixel CreateFontA settings (quality 3, non-antialiased).
Runtime rendering uses portable binary glyph masks, original bearings and
advances; GDI text/font and DIB/DC execution has been removed. Menus/HUD/credits
retain their existing atlas paths. User visual acceptance remains pending.

The mandatory gate now compares the actual command consumer and adapter against
original offscreen TextOutA output, including colors, metrics and clipping.
The previous retail pixel-font substitution was rejected visually and superseded.
The manual arena adds two stationary real vehicles with native TARGET/SUSPECT
HUD ownership; all fixtures stay in ignored harness copies. No new diagnostics
were added to production source; user answered no further cleanup. The profiler
remains. Player output is build/; test and release-check outputs remain under
harness_tools/output/. Current evidence is portability-step2/verification.json.

**Recommendation:** investigate reusing existing atlas text first. Confirm glyph
coverage, metrics, and licensing before adopting a replacement font asset.

**Concerns:** Comic Sans appearance changes, missing characters, text width,
baseline differences, color channel ordering, and surface upload coherence.

**Automated checks:** command ordering, clipping, color conversion, glyph lookup,
and surface updates. Reuse existing text/render tests where applicable.

**Manual checks:** menus, HUD, credits, editor messages, and every identified GDI
text use. The user checks appearance; do not automate screenshots.

**Exit criteria:** game and adapter no longer need GDI font or DIB/DC operations.

## Milestone 3: Sound effects and music

- [ ] Introduce an audio backend boundary below existing resource/voice policies.
- [ ] Replace DirectSound playback with SDL audio output and the required mixing.
- [ ] Preserve pitch/frequency, gain, panning/spatial attenuation, loops, voice
  stealing, resource lifetime, and attached-sound updates.
- [ ] Replace WAV music playback, looping, and completion handling from MCI.
- [ ] Apply music/effects volume inside the game instead of changing OS volume.
- [ ] Define behavior for output-device loss, reopen, and focus transitions.
- [ ] Remove MCI notifications, WinMM volume controls, and the remaining message
  bridge after the new audio path is complete.

**Recommendation:** migrate effects and music in the same milestone so device
ownership and volume policy have one implementation. Preserve sound scheduling
at the existing simulation rate while keeping output mixing independent.

**Concerns:** SDL audio output alone does not reproduce DirectSound voice and
spatial semantics. Watch callback lifetime, synchronization, resampling quality,
clipping, latency, buffer underruns, and music gaps.

**Automated checks:** deterministic offline mixes for silence, gain, panning,
pitch, looping, completion, and saturation; stress repeated voice allocation,
stealing, stop/dispose, and device lifecycle. Compare sample counts and scheduling
against the intended behavior rather than requiring bit-identical resampling.

**Manual checks:** weapons, explosions, attached/moving sounds, overlapping
effects, music loops, pause/resume, volume sliders, device changes, and extended
play. Audio quality and audible discontinuities require user listening.

**Exit criteria:** no game-owned DirectSound, MCI, or WinMM playback/volume calls;
changing game volume does not change system volume.

## Milestone 4: Force feedback and networking

Treat these as separate changes within the milestone.

### Force feedback

- [ ] Inventory effects and the hardware behavior they represent.
- [ ] Map supported behavior to SDL haptics/gamepad rumble.
- [ ] Define a safe fallback for unsupported devices and effects.
- [ ] Remove DirectInput initialization and associated SDK types.

**Concerns:** gamepad rumble is not equivalent to directional joystick force
feedback. Preserve supported behavior explicitly rather than silently claiming
full equivalence.

**Testing:** check capability handling, disconnect/reconnect, effect updates,
focus loss, stop, and shutdown. User verification requires suitable hardware.

### Networking

- [ ] Separate session/lobby discovery from packet transport and gameplay codecs.
- [ ] Decide whether original DirectPlay interoperability is required.
- [ ] Define delivery, ordering, reliability, identity, discovery, and timeout
  requirements before selecting a replacement transport.
- [ ] Replace provider startup and COM ownership in the portable path.
- [ ] Preserve existing packet formats where required and test their bounds.

**Concerns:** replacing DirectPlay is a multiplayer feature migration, not just
an API substitution. Legacy discovery, modem/serial providers, and original-game
interoperability may need separate support decisions.

**Testing:** packet encode/decode and malformed input; two-process sessions;
join/leave, host exit, timeouts, loss, duplication, reordering, and reconnect.
Verify gameplay events, ownership, and scores across peers.

**Exit criteria:** the portable runtime has no unconditional DirectInput or
DirectPlay/COM dependency; supported behavior and any compatibility limits are
explicit and verified.

## Milestone 5: Headers, legacy services, shaders, and build cleanup

- [ ] Remove or isolate unused registry/source discovery, Windows-directory paths,
  native bootstrap, and worker code after confirming their callers.
- [ ] Replace remaining game-owned synchronization with SDL primitives if needed.
- [ ] Contain legacy graphics constants/structures in a project-owned internal
  interface; preserve required layouts while removing Windows SDK header use.
- [ ] Replace compiler-specific string and numeric helpers behind portable code.
- [ ] Precompile shaders to remove the adapter's runtime `D3DCompile` dependency.
- [ ] Remove obsolete linker libraries, SDK definitions, and forced includes.
- [ ] Make dependency staging and output naming target-aware when adding another OS.

**Recommendations:** keep DX12 selection unchanged. Removing the runtime shader
compiler does not require changing the GPU backend. Record how shader binaries
are regenerated and ensure packaging includes the correct artifacts.

**Concerns:** packed structures, callback calling conventions, error-code values,
GUID identities, and legacy surface formats can affect behavior even after native
calls are gone. Do not replace types by name without checking their semantics.

**Testing:** clean builds, packaged launch from another working directory, missing
asset handling, settings persistence, repeated startup/shutdown, and import/source
audits. Confirm shaders load without the game importing `D3DCOMPILER_47.dll`.

**Exit criteria:** Windows SDK use is confined to any explicitly retained backend;
game-owned native service calls and obsolete linkage are removed.

## Later work: Other operating systems and 64-bit support

A native non-Windows build also needs a non-DX12 SDL GPU backend and suitable
shader formats. That is outside the retained-DX12 milestone.

For 64-bit support, audit fixed-width integers, pointer handles, serialized
records, packet formats, structure assertions, and arithmetic assumptions. Keep
disk/wire formats separate from in-memory structures. Address x87 PC53 behavior,
rounding, and numerical regression expectations before changing architecture.

**Recommendation:** first prove portability on a compatible 32-bit target if
practical, then tackle pointer width and numerical behavior as a separate project.

## Verification and completion rules

For each implementation change:

1. Build the affected subsystem, then the complete release and debugging game.
2. Pass the required regression gate above, then run relevant additional tests
   and add focused boundary tests for changed behavior. Do not assume the
   existing gate covers every aspect of a new backend.
3. Recheck imports and source references; distinguish active calls from linked
   legacy code and dependencies internal to SDL/CRT.
4. Have the user perform visual, audio, and hardware checks. Use shell commands,
   source inspection, build tools, and logs only; no computer/UI automation or
   automated screenshots.
5. Compare profiler results under matching scenarios and inspect hitches, audio
   underruns, and changes to simulation cadence. Preserve the profiler.
6. Keep investigation scripts, fixtures, test-only headers, and diagnostic metadata
   in git-ignored `harness_tools/`, not `tools/` or `src/`.
7. After testing, inspect actual game source for temporary checks, fixtures,
   switches, callbacks, capture hooks, traces, and instrumentation. Explicitly ask
   whether the identified diagnostic code needs cleanup before calling the work
   complete, unless cleanup was already requested for that task; in that case,
   perform it. Compile-time guards do not exempt diagnostic code.
8. Format changed C/C++ files with `src/.clang-format`, format scripts consistently,
   and review the final diff. Do not change `README.md` without an explicit request.

## Decisions to resolve when their milestone starts

- Must existing saved bindings and configuration files remain fully compatible?
  Recommendation: yes; migrate formats only when necessary and preserve old input.
- Must physical CD audio remain supported? Recommendation: make staged file music
  the portable baseline and treat physical media as optional scope.
- Must multiplayer interoperate with the original executable? Resolve this before
  choosing the replacement networking design.
- Which force-feedback devices/effects must remain supported? Establish hardware
  coverage before replacing directional effects with rumble.
- Is the next deliverable Windows dependency cleanup or a native non-Windows
  release? Recommendation: complete the former before expanding backend scope.

User-requested diagnostic cleanup (2026-11-04): removed the legacy worker BMP
capture path, its F12 trigger, and capture state. The old writer is retained only
in ignored harness_tools/legacy_capture for historical standalone fixtures.
The framerate profiler is preserved.

User playtest acceptance (2026-11-04): user reported all input good and
authorized commit/push of milestone 1. Automated gate and capture cleanup passed.

Step-2 visual correction (2026-11-04): the initial synthetic-mask gate missed
incorrect retail atlas transparency. A corrected retail mask still looked wrong
because its font differed from the original. The current build bakes Comic Sans
MS instead. Twelve offscreen original-vs-portable comparison cases pass with
zero differing RGB pixels. These are memory buffers, not automated screenshots.
Visual and audio checks remain with the user; minor visual differences are
acceptable if the overall appearance is similar.
