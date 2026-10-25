# Focus reacquisition and CD notification dependencies

Two complete ordinary owners are source implementations: `00471220`
(`00471220..0047125e`,63 bytes/22 instructions) and `00449b90`
(`00449b90..00449c13`,132/39). The pre-implementation role/boundary/active-ledger
audit is `../next-review/review.json`; complete raw bodies were independently
verified against immutable original PE bytes. Both had no active implementation
records. They are engine candidates with unresolved historical provenance,
not CRT, compiler support, import thunks or independently counted shared tails.
The source uses actual DirectInput and WinMM imports/interfaces, not fake
callbacks or vtables.

`bh_force_feedback_start` borrows the admitted stop owner's `BHForceFeedback`
without changing its header/source variant. A separate explicit `started`
field represents original004edef0. When enabled and not already started, it
conditionally calls actual device Acquire, conditionally calls actual Effect
Start(1,0), then latches started1 even after native failures. Pointers are
reloaded from the live borrowed owner between calls, matching original globals.
Native HRESULT diagnostics affect its source return, not later original side
effects. No constructor/device/effect is guessed.

`bh_cd_notify` borrows the admitted `BHCdClose`, plus retained callback and
four original range words. Only nonzero stored device, exactly matching device
argument, successful low16 notification event1 and nonzero playing state cause
MCI_PLAY. It retains the original flags0x0d (NOTIFY|FROM|TO),12-byte x86
MCI_PLAY_PARMS and exact bit packing. `dwFrom=from_track&255`;
`dwTo=(((frame&255)<<8)|(minute_second&65535))<<8|(end_track&255)`.
The OR overlap is intentional original behavior. API failure does not clear
playing/device/range state. No CD open/play/lifetime owner is fabricated.

`build_check.py` compiles the four actual implementation/test TUs and links
real integration definitions into `bhunter-notify-check.exe`, then executes
both checks. Source/header/object hashes and commands are retained in
`standalone-build.json`. The target's name avoids Windows' installer elevation
heuristic triggered by the earlier name containing "dispatch"; no elevated
execution or system changes were attempted.

Eight force-feedback checks exercise invalid bindings, disabled/already-started
gates including noncanonical flag values, absent optional device/effect branches
and the actual latch/idempotence. No physical input device is acquired and no
force-feedback effect starts. Native nonnull Acquire/Start ordering and arguments
are statically complete, but their hardware-positive/error paths remain
unexecuted and unclaimed on this host.

Eighteen CD notification checks exercise every eligibility gate and build the
exact retained packed parameters. A newly created silent PCM WAV is opened
through actual WinMM only to obtain a private device ID, then successfully
closed **before** executing the matching positive notification branch. Real
MCI_PLAY on that retired ID returns274 (invalid device ID), so native error
reporting/state retention executes without audible playback, CD hardware or
MCI_ALL_DEVICE_ID. The fixture is removed. No original code/game execution,
desktop input, screenshots or user file modifications occur.

Root owns the current whole-target integration/admission evidence separately.
These dependencies do not constitute complete startup, force-feedback hardware
validation, CD looping validation or a playable source game.
