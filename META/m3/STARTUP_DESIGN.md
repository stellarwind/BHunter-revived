# Isolated original startup diagnostic

This next M3 experiment uses the normal Windows loader for an unchanged local
EXE/BIN copy. It is not linked into source integration and does not extend the
math mapper. It uses only native debugger traces, with no computer-use/visual
tools or UI input injection.

The source-registry call receives the same private HKCU SourceDir redirect
validated by the earlier probe. A process-local GetWindowsDirectoryA adapter
returns an actual existing private profile directory, using its buffer/length
contract. The original config/path/stdio functions still execute and create
real files. This is an explicit filesystem-path adaptation, not evidence that
the original Windows-directory installation convention is portable unchanged.
The original hKey/path and every platform adaptation are logged. Original
HKLM values and Windows-directory game configuration are not intentionally
modified.

Temporary debugger breakpoint bytes are checked/restored when executed or at
controlled stop. The MessageBoxA entry is intercepted to record actual text,
caption and flags from process memory, then stop before opening the dialog.
There is no guessed click, screenshot, visual identification, modal response
or fabricated renderer result. Initial bring-up is intentionally bounded at
that first interaction boundary; it is not a menu/gameplay acceptance trace.

The expanded probe alternatively stops at `00479200` before executing the game
bootstrap. It watches all thirty original WinMain rendering-mode callsites and
their returns, verifies their call targets before instrumentation, and records
the first actual DirectDraw SetDisplayMode HRESULT. No renderer success is
injected. Hidden-window trials and normal-show trials are labeled separately;
the native/compatibility comparison uses normal show mode in both cases.

The renderer preset is a private copy of the actual 644-byte original-written
CFG, with only the measured configured flag (16-bit at offset 0), hardware
selection word (32-bit at 4) and software-resolution word (32-bit at 8) set.
All other bytes, including opaque fields, are retained; the original loader
rebuilds its own serialized profile pointers. This is explicit configuration
input, not a game-code branch or result patch. Source snapshots and exact seed
hashes are retained per probe variant/run.

Record source acceptance, path resolution, DirectDraw enumeration, config
bootstrap and first rendering-mode entry/return when reached, along with x87
controls and native exception/child-exit events. All imported graphics/sound
interfaces remain the real OS dependencies. Native API/driver failures must
remain failures. Timeout terminates only the diagnostic child.

## Bootstrap extension

`--through-bootstrap` is a separately recorded probe variant. It resumes the
original `00479200` and stops at its actual WinMain return point `004733f2`.
Seven additional one-shot checkpoints observe sound/input returns, world entry,
registry/level/navigation returns and world completion. Seven associated direct
call targets are checked before instrumentation. Global reads at these stages
must succeed; unreadable memory is a probe failure, not a fabricated zero count.

This variant intercepts process calls to SetCursorPos and returns FALSE,
logging coordinates and caller. It does not move the user's cursor or simulate
successful input. In the recorded run this also denies two compatibility-layer
cursor requests. Graphics/input observations are therefore qualified by an
explicit cursor-failure adapter; earlier native/compatibility comparisons
belong to their archived variants. auxSetVolume and waveOutSetVolume stop the
child before any such call executes. Neither volume guard was reached in the
bootstrap capture. DirectSound initialization and asset/resource creation
execute normally; no audio result or buffer is supplied by the probe.

Debug events use the actual event thread's context. Thread-creation events
record initial x87 state and entry addresses, without assigning system-thread
purposes from addresses alone. These initial controls do not establish later
worker precision. The original game rendering worker (`00471e00`) is created
after the selected stop boundary and was not started in these runs.

The final bootstrap source variant and its actual linked/run evidence are in
`startup/f7a780ff1c4d.json`; `startup/89994fa7c8f0.json` retains the preceding
variant. Both are original-backed diagnostics, not source-game implementations.
Termination at bootstrap return is not orderly shutdown or menu/playable
scenario acceptance. Audit: `python tools/check_m3_bootstrap_evidence.py`.

## First simulation/render extension

`--through-first-frame` implies bootstrap tracing and additionally observes
`00479140` and `00479090` on their actual threads, installing one-shot return
breakpoints from their real stack return addresses. It stops after the original
surface Flip returns at `00471fc7`; the COM call bytes are checked before arming.
No callback, Flip, input or audio success is substituted.

The original binding-list high-nibble assertion faults on a valid heap pointer
in the default environment. `--crt-initial-heap-mib 128` is an explicit diagnostic
HeapCreate initial-size input adaptation. Exact original arguments and the
owning import callsite are checked; allocator/free APIs and the assertion still
execute normally. `--disable-bottom-up-aslr` is a separate per-child creation
policy experiment, using an optional extended STARTUPINFO and actual policy
query, with no persistent setting. It did not fix this failure in the recorded
trial and is unnecessary in the successful heap-size profile.

Failed first/second-chance exceptions retain context, fault address and the
actual binding pointer's VirtualQueryEx result. These failures remain nonzero
probe exits. A later exception path restores/verifies breakpoint bytes before
terminating; the earliest failed variant did not record restoration and is
labeled accordingly. `FIRST_FRAME.md`, per-run source snapshots and
`runtime-validation.json` distinguish the variants and their limits. The
first-frame profile still denies cursor warps and stops before global volume
writes, performs no UI input/visual inspection and never enters source-game
admission. Audit: `python tools/check_m3_runtime_evidence.py`.

## Bounded menu/input trace

`--observe-callbacks 300` or `600` observes actual simulation returns and reads
18 constructed menu objects. A restored three-byte ADD ESP,8 at the real
simulation return address is single-stepped, then its breakpoint is rearmed on
that same thread; only one pending instruction is allowed. Byte/length checks
fail closed. The actual callback count, original tick word and elapsed host time
are separate fields. This does not impose deterministic timing.

`--menu-trace prologue` additionally posts key messages only to the newly created
child's original HWND, after verifying its process owner for every message.
It uses no computer-use/visual tools or desktop-wide input. Native key-state
transitions, active menu/selection preconditions and the real episode-action
entry/return are recorded; the probe never writes selection/episode globals.
Input replay is explicitly labeled, unlike the earlier observation-only runs.

`--relative-cursor` translates actual successful GetCursorPos samples only at
the original poller's return. EAX remains the native result. The first sample
sets the virtual origin; subsequent samples carry actual physical deltas into
the requested recentered frame. Native failure stops the probe. The following
two-byte PUSH instruction is single-stepped/rearmed on its owning thread. The
existing cursor-warp denial remains, and no game instruction is emulated. This
is an explicit platform-input adaptation, not unchanged native input evidence.

`MENU_TRACE.md` separates successful inventory/navigation from earlier drift,
wrong-selection and precondition-stop trials. Current successful input evidence
is `startup/73beffa6a63f.json`; source snapshots and compile/link/run belong to
that variant. Audit: `python tools/check_m3_menu_evidence.py`. No mission,
movement/objective, playback/network or normal-shutdown acceptance is inferred.
