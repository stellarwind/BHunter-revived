# Original menu inventory and child-local input trace

M3 remains active. This block uses CLI/native debugger traces, without
computer-use skills, screenshots or visual identification. Optional input
replay posts key messages only to the newly created diagnostic child's window;
it does not send desktop-wide keyboard/mouse input. This is original-backed
reference execution, absent from source integration and recovery counts.

## Reviewed family and constructed inventory

`menu-review/assignment.json` classifies the menu constructor, item constructor,
input/animation callbacks and action callbacks as game/engine UI candidates
before this assignment. Authorship remains unestablished. Complete exports
support offsets and behavior; no CRT/import/shared tail becomes a gameplay
recovery assignment. `0042b2e0` is the actually observed active menu step.

`0042c300` allocates a real entity and binding node. Its item count is at `1cc`,
selection at `1d0`, and item-pointer array at `200`. `0042c4c0` allocates each
28-byte item: label pointer at `0c`, destination menu at `10`, action callback at
`14`, edit/update callback at `18`. The probe reads those actual objects after
bootstrap and at its bounded stop. It does not reconstruct labels from pixels.

The original constructs **18 menus with 118 entries**:

| Inventory | Actual object labels/structure |
|---|---|
| Main | GAME, NETWORK, SETUP, QUIT |
| Ship choices | BARLETTA, STINGER, VIPER; separate single/network lists |
| Episodes | prologue, episode 1 through episode 8, BACK |
| Setup | VIDEO, AUDIO, INPUT |
| Video | 3D acceleration, graphics device, software resolution |
| Audio | sound FX louder/softer, CD music louder/softer |
| Input profiles | configuration selection, NEW, EDIT, DELETE |
| Motion | keyboard move up/down/left/right, accelerate/brakes; joystick/mouse axes |
| Weapons | primary/secondary fire and toggles; named weapon selections |
| View | cockpit/tail/trace/fly-by/circle/side cameras and keyboard/joystick/mouse look |
| Network pages | connection choices, modem, IP/host, COM/baud/stop/parity/flow, session create/join |
| Quit | YES / NO |

The machine-readable `menu-inventory.json` retains every captured label, target,
action and edit callback from the final run. Constructed options are feature
inventory evidence, not proof that every backend, weapon, episode or network
transport works. Runtime gameplay content/feature coverage is still incomplete.

`startup/3fa87b3191c2.json` observes 300 real simulation returns without injected
keys and reaches the actual main-menu callback. Subsequent navigation trials
preserve their own source snapshots and outcomes.

## Cursor/input contract and unsuccessful attempts

The original input poller reads GetCursorPos then requests a warp to (160,100).
The pre-existing diagnostic guard returns FALSE to avoid requesting desktop
cursor warps. With absolute mouse coordinates still passed through, the same
offset repeats each tick and menu selections cycle. The first key trial
`startup/9e7445abb9a7.json` opens ship/episode pages but does not select the
prologue. Its successful probe exit is not successful requested navigation.

`--relative-cursor` is a separate explicit platform-input adaptation. The real
GetCursorPos API executes. Only after its actual successful return at `0047549c`
does the probe translate its output to:

```
first sample:       (160, 100)
subsequent samples: (160 + physical_x - previous_x,
                     100 + physical_y - previous_y)
```

The native success result is preserved; failure aborts the diagnostic rather
than fabricating success. Physical coordinates, translated coordinates and the
first-sample flag are recorded. Only the original poller's stack POINT changes.
The existing SetCursorPos failure guard remains. This compensates for denied
recentring; it is not evidence that an unchanged original input backend works
without recentering, and actual physical movement can still affect the trace.

The initial translated trial `startup/1ad993c8a666.json` still sends two
unconditional Up presses, entering Setup from the now-stable initial GAME
selection. It is not the requested path. A later guarded trial
`startup/3e8c09b297f2.json` refuses an assumed ship index 0: the original's
selected ship is actually **1 / STINGER**. That failed probe is retained and
does not claim breakpoint restoration. The following variant accepts the
original selected ship while checking it is a valid ship, not BACK; no selection
word or game state is patched. Later probe-error handling restores/verifies
breakpoints before terminating.

## Actual menu path

Final current evidence is `startup/73beffa6a63f.json`; the preceding successful
variant is `startup/1a9aaba8c7d0.json`. Both execute GAME → STINGER → prologue.
The final driver records the requested-path result separately from generic
probe success.

| After completed simulation callbacks | Actual input/observation |
|---|---|
| 300, 306, 312 | Read current root selection; Up is sent only if needed. Final run already selects GAME, so no Up is sent. |
| 320 / 322 | Post Enter down/up after verifying GAME is active and selected |
| 360 / 362 | Post Enter down/up after verifying an active valid ship selection; observed STINGER index 1 |
| 400 / 402 | Post Enter down/up after verifying active prologue index 0 |
| Original `0042ca10` entry/return | Actual episode argument 0; original stores `004dfb08=1` and `004dfb04=0` |
| 440 | All three menu step callbacks inactive; episode-action flag remains 1 |
| 600 | Bounded observation stop; child terminated by diagnostic |

Each PostMessage target is read from the original HWND global and checked with
GetWindowThreadProcessId against the child PID. Actual posting results and the
original key-array down/up states are recorded. No SendInput, desktop click,
physical key event, forced menu callback or patched episode/global is used.

The flag at `004dfb08` is called `episode_action_flag` in current traces. Earlier
variants called it `start_pending`; that label is not evidence of a transient
pending operation. Its broader gameplay semantics remain under review. The
episode list initially contains nine action callbacks, but the original ship
action gates them using the signed settings word at `00496a3a`: the observed
profile leaves only prologue enabled. Listing eight episode labels is not
completion or accessibility proof for those episodes.

One original simulation-return instruction (ADD ESP,8) and the original
post-GetCursorPos instruction (PUSH 100) are stepped/rearmed on their owning
thread. Their bytes and instruction lengths are checked; no gameplay
instruction is emulated or skipped. The final run restores/verifies all 89
instrumented sites, including actual callback returns. Repeated trap writes do
not count as new recovered functions. Main/render sampled controls stay 027F.

## Reproduction and limits

Using the prior staged source marker, original-written seed and pinned local
DDrawCompat prerequisite:

```powershell
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat --menu-trace prologue --relative-cursor --crt-initial-heap-mib 128
python tools/check_m3_menu_evidence.py
```

This retains the explicit initial-heap-size, private profile/source-key and
graphics compatibility profile. Original EXE/BIN/MDF/MDS, Windows CFG/INI files
and original HKLM views remain unchanged; private keys are removed. No global
aux/wave volume guard is reached.

The key schedule is recorded and repeatable; random seed, host timing, mouse
movement and thread scheduling are not pinned. This is not a bit-identical
simulation replay. Mission initialization, actual player movement/collision,
objectives/failure/retry, sound/CD playback and loops, network execution and
normal shutdown/reload still need separate evidence. The original menu callback
and flag write do not satisfy those requirements.
