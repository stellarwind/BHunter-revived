# Original first simulation and presentation trace

M3 is still active. This block uses native CLI debugger traces only. No
computer-use skill, screenshot, visual identification or UI input was used.
Original code is required by these diagnostics and remains outside the normal
source integration target. No original file was patched.

## Reviewed boundaries and roles

The selected callback/engine boundaries were already present in the M1 catalog.
Complete read-only exports in `runtime-review/` and `input-address-review/`
support this diagnostic assignment; they do not promote source recovery counts.

| Boundary | Role and diagnostic ownership |
|---|---|
| `00479140` | Game simulation/cleanup orchestration callback, installed by original bootstrap |
| `00479090` plus owned tail `00450930..00450978` | Render callback; shared tail remains owned by this behavior |
| `00471e00` | Original render-worker entry, identified by the actual CreateThread argument |
| `004744f0` | Engine-candidate input polling and binding-list dispatch; authorship unestablished |
| `00475640` | Engine-candidate owning binding-node allocation/list insertion |
| `0047dae0` / `0047db00` / `0047db50` | CRT malloc wrapper/allocation path; excluded from gameplay assignments |
| `0047f7a0` | CRT heap initialization path calling HeapCreate; diagnostic platform boundary, no new source implementation |
| `004738a0` | Shutdown request helper; inspected statically, not executed by this trace |

`00475640` really allocates 0x2130 bytes through the original CRT, initializes
the node and prepends it to `004ef2fc`. Input polling walks that list. Before
dispatching bindings it shifts each pointer by 28 and tests its high nibble.
A nonzero result executes `004755ce: MOV [00000004], ECX`, with ECX=-1.
This is an original deliberate invalid write, not a debugger-inserted failure.

## Bounded investigation and controls

Question: can the original's address assumption be satisfied through a recorded
process/allocator input setting while leaving the original assertion, allocator,
binding list and simulation body intact? Positive control is real completion of
the first simulation callback, renderer callback and surface Flip. Stop at that
boundary; this is not a search for byte matches or prolonged gameplay proof.

The first trial `startup/dbb63b9aaab8.json` created the game render worker and
entered simulation tick zero, then faulted at `004755ce`. The improved failure
capture `startup/e95185853a02.json` records EAX=ESI=binding head=`12ee9af8`,
committed private read/write memory, and a write fault at address 4. Thus a valid
allocated pointer exceeded the original 28-bit check. That first trial lacks a
verified restoration event; it remains failed evidence, not a successful probe.
Subsequent fault captures restore/verify breakpoint bytes before termination.

A per-child bottom-up-ASLR-off experiment (`startup/0a085eeb1173.json`) also
failed at the same assertion. Windows reported ASLR policy flags zero. The final
default-policy control also reports zero, so this evidence does not attribute
the problem to bottom-up ASLR. This hypothesis is rejected for these runs.
The optional launch setting uses Microsoft's documented
[process creation mitigation attribute](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-updateprocthreadattribute),
queries the actual child policy, and changes no persistent system/app policy.
The x86 dynamic declarations/constants are checked against Microsoft's
[WinBase.h](https://raw.githubusercontent.com/microsoft/win32metadata/main/generation/WinSDK/RecompiledIdlHeaders/um/WinBase.h).

The next explicit diagnostic adaptation changes only the initial-size input at
the original CRT's `0047f7a9` HeapCreate call: flags remain 1, maximum size
remains 0, and initial size changes from 4,096 bytes to 128 MiB. The exact
indirect-call bytes and original three arguments are checked first. The actual
HeapCreate/HeapAlloc/HeapFree and original malloc body execute normally. No
pointer/global/return-success value is supplied by the debugger, and the
original high-nibble assertion still executes. This affects allocation/failure
conditions and is an explicit reference-profile adaptation, not unchanged
historical environment behavior or a release backend decision.

The first heap-size trial (`startup/0a1b09078440.json`) completed simulation and
render callbacks. The refined final trial (`startup/75fef4730703.json`) also
captured the first actual surface Flip result. A control with exactly the same
C/Python source hashes and CFG bytes but no heap adaptation
(`startup/b8497ba5a485.json`) still faulted at the original assertion. Its
binding pointer was `12f89af8`; the adapted run's was `0902a020`, below 256 MiB.
This supports the tested adaptation for first-tick bring-up. It does not prove
that every future allocation will remain below that boundary.

## Actual final observations

| Event | Captured result |
|---|---|
| Bootstrap | Actual return 1; 454 models, 283 navigation nodes, 7,831 used entities |
| First main-thread simulation entry/return | Tick 0 to 1; 7,831 to 8,468 used entities |
| First renderer callback | Executes and returns on the actual `00471e00` worker |
| Original surface Flip return at `00471fc7` | HRESULT `00000000`, S_OK |
| Main and render callback/Flip controls | x87 `027f` at each sampled boundary |
| Focus/shutdown words | 3 / 0 at these boundaries |
| Input | Real polling runs; SetCursorPos returns FALSE under the existing cursor guard |
| Sound/music | No aux/wave volume guard reached; no playback/loop acceptance |

Thread IDs, heap addresses, elapsed host time and counts are actual per-run
observations. Debug stops change timing, scheduling is not pinned, and cursor
sampling remains real while warps are denied. These are not deterministic input,
seed, timing or simulation-equivalence traces. The traced message entity's step
callback is `00442bf0`; this is not evidence of an interacted-with menu.

84 initial breakpoint sites plus two actual callback-return sites restore and
verify before controlled termination in the final run. Surface Flip is observed
after the original call; no success is injected. No pixel inspection or visual
fidelity claim follows from S_OK. The diagnostic terminates its own child at
this first presentation boundary, so game shutdown/cleanup is not exercised.

## Reproduction and remaining requirements

With the recorded original-written seed, staged source marker and pinned local
DDrawCompat prerequisite:

```powershell
python tools/m3_startup_probe.py --seed-config build/m3/startup/profile-c57437263011/BHUNTER.CFG --render-profile software-high --show-reference --ddrawcompat --through-first-frame --crt-initial-heap-mib 128
python tools/check_m3_runtime_evidence.py
```

The failed control omits `--crt-initial-heap-mib 128`; it intentionally exits
nonzero and preserves failure evidence. Earlier variants are validated through
their archived snapshots, never by transferring evidence to edited sources.

Next requirements are actual menu/control and feature inventory, deterministic
input/seed/time/state capture where possible, movement/collision/objective/
failure/retry scenarios, sound/CD playback and looping, network modes and normal
shutdown/reload. The observed first-worker precision does not establish later
driver transitions, PC24 equivalence or full gameplay numerical behavior.
