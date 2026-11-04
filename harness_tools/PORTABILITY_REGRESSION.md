# Portability regression arena

Run commands from `D:\Decomp\Bhunter`. Selected gate sources and fixtures are tracked under
`harness_tools/`; generated copies, logs and output remain ignored. Production `src/`, the normal
game build, player saves, and the retail archive are not modified.

## Commands

```powershell
# Full baseline: replay, deliberately dropped key release, subsystem contracts.
python harness_tools/portability_regression.py --suite

# Visible scripted demonstration; approximately 30 seconds after loading.
python harness_tools/portability_regression.py --visible

# Manual free flight with recurring sound cues; close normally when finished.
python harness_tools/portability_regression.py --manual

# Build the separate test game without launching it.
python harness_tools/portability_regression.py --build-only

# Run subsystem contracts alone against the current isolated build.
python harness_tools/portability_contracts.py --no-build
```

Close an existing regression session before rebuilding. Scripted runs reset only
the dedicated test runtime's configuration/video files. Manual settings are
therefore temporary if followed by an automated replay. No global key injection,
foreground-window manipulation, UI automation, screenshots, or audio recording is
performed. Default scripted runs use a hidden SDL window but require working GPU
and audio devices; this is not a headless graphics/audio replacement.

## Arena and automated coverage

Three adjoining rooms use blue, green, and amber grid textures. The start room
contains the initial player/camera positions away from a portal plane. Doorways
connect the rooms along the Z axis. The normal model library is retained, but
`test.lev`, its navigation-cache header, and the block list are isolated loose
overrides. A generated three-second stereo music track makes actual playback
completion and repeat notifications observable during one short run.

The replay checks:

- Real text-capture/profile commit: tap A, B, Backspace, C, Return and expect AC.
- Menu navigation into gameplay using SDL events scoped to this game.
- Held movement, repeat suppression in key history, release, and changed position.
- Mouse-button down/up and a synthetic focus-loss callback clearing held input.
- Escape pause/resume through the player input path.
- Left/right positioned tones and the backend's applied pan values.
- Low/high pitch tones and the backend's applied frequency values.
- Actual warning and impact resources from the game archive.
- Persistent sound start, update, stop, handle retirement, overlapping requests,
  and stopping all effects.
- Music startup and repeated completion notifications, with errors checked.
- GDI text commands reaching the renderer, plus renderer text failure counts.
- Save/reload of configuration, aspect transitions, and retained math environment.
- Clean scenario completion and process exit.

The negative control drops the Up-key release. It passes only when the replay
completes with exactly the expected `keyboard-release` assertion failure and
process exit 90. The later synthetic focus-loss step clears that held key.

The companion contract runner covers input evaluation/messages/collection, SDL
gamepad bindings, input transitions and packets, network codecs, configuration,
WAV/RIFF parsing against real assets, VFILE lookup, sound pools/listener/tick,
and timing/interpolation across display rates. It uses 13 existing test cases
against the current GCC objects. Self-contained fixtures and supporting headers
are tracked under `harness_tools/fixtures/`; retired expectations were adapted
for SDL bindings, synchronous sound stop and the removed VFILE debug switch.
The suite does not depend on the local ignored `tests/` directory.

## Manual checklist

- [ ] Enter all three rooms and cross each doorway in both directions. Check wall
  collision, orientation, texture visibility, HUD, and the on-screen cue label.
- [ ] Hold and release movement/fire keys, use mouse buttons, and try simultaneous
  inputs. Test typing, Backspace, held-text repeat, and saved rebinding in menus.
- [ ] Switch away while holding a key; return with it released. Minimize/restore
  and check for stuck controls, sound changes, and simulation catch-up bursts.
- [ ] Exercise Alt+Enter, resize, video settings, and the monitor/display scales
  you normally use. Check text placement and aspect behavior visually.
- [ ] Listen for left then right tones, low then high pitch, recognizable game
  warning/impact cues, a sustained loop, and its clean stop. Check for clicks,
  clipping, unexpected silence, and gaps at music repeats.
- [ ] Pause/resume and return to menus, then start another session. Confirm engine
  audio returns and no warning/weapon loops leak into the next session.
- [ ] Test gamepad connect/disconnect and unsupported-device fallback. Verify force
  feedback only with suitable hardware; its presence is not an automated pass.
- [ ] Close normally, relaunch, and check test settings and orderly shutdown.
- [ ] Separately play a normal retail level: this synthetic arena does not
  replace mission, traffic, target, pickup, or full combat coverage.

Manual mode starts in free flight. Every 30 seconds it repeats this cue schedule:

| Seconds in cycle | Cue |
| --- | --- |
| 14 / 15 | Left / right tone |
| 16 / 17 | Low / high pitch tone |
| 18 / 19 | Game warning / impact |
| 20 / 21 / 22 | Loop start / quieter higher pitch / stop |
| 23 / 24 | Test configuration save / reload |
| 25 / 26 | Cycle aspect / restore it |
| 27 / 28 | Overlapping tones / stop all effects |

The manual player's engine request is restored after the all-effects-stop cue.
Cue timing follows simulation ticks, so focus loss suspends the schedule. Manual
mode does not post scripted input or automatically close the game.

## Reports and limitations

Output directory: `harness_tools/output/portability-regression/`.

- `suite-results.json`: overall exit codes and production-source integrity.
- `results.json`, `events.jsonl`: normal replay assertions and state milestones.
- `negative-results.json`, `negative-events.jsonl`: injected-failure control.
- `contracts/results.json`: contract build/runtime output and fixture adaptations.
- `build.json`: production/harness/executable hashes.
- `runtime/arena.json`: level bounds, overrides, and coverage limitations.
- `runtime/frame-times-*.csv`: preserved framerate profiler output.

The runner fails on build failure, crash, timeout, missing summary, unexpected
assertions, or nonzero normal exit. Logs record requests and game/backend state;
they do not prove speakers emitted the intended waveform or pixels looked right.
There is no automated screenshot or audio-quality comparison.

The focus check calls the current application callback synthetically. It does not
prove real OS focus/minimize delivery or reset-clock behavior. Text taps last one
simulation tick; held-text repetition remains a manual check. Video checks exercise
the aspect path, while window modes and displays remain manual checks.

When Windows input is replaced, migrate only `regression_key` and the mouse/focus
injection points to the new SDL event entry point. Preserve the timeline and
behavior assertions. Do not add an alternate production input path merely to
keep this harness passing. Sound assertions currently inspect the legacy voice
cache; move equivalent observations to the new backend's real playback boundary
when it replaces DirectSound.

DirectPlay peer behavior, physical CDs, output-device reconnect, force feedback,
non-Windows GPUs, and 64-bit layout/numerical compatibility need dedicated later
tests. Their startup or codec coverage here does not certify the whole subsystem.

## Diagnostic-code ownership

The builder uses checked text anchors to generate instrumented application/main
copies and fails if an anchor changes. It never edits `src/`. All hooks, synthetic
assets, environment switches, and assertions belong to this separate test game.
The normal framerate profiler is preserved. After future tests, inspect the real
game source and follow `AGENTS.md`'s diagnostic-cleanup question before declaring
the work complete.

The manual arena includes two parked vehicles with native TARGET and SUSPECT HUD
labels. Their fixtures use production constructors and callbacks and live only
in generated harness copies. The suite checks their creation, ownership and
stationary positions. It also compares twelve portable overlay render buffers
against original Comic Sans MS TextOutA buffers, including clipping and colors.
No desktop or game-window screenshots are taken.
