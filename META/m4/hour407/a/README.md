# Script particle constructor and emitter closure

Six whole owners: particle constructor004025c0 and its installed actual
step004024c0/billboard refresh00402470; fountain step00443390; steam
step004434b0 and oriented steam refresh00443640. These are ordinary complete
behavior owners called by native VM effect paths. Original calls, explicit
entries/RET boundaries and every instruction are retained in `original/`;
they are not CRT/library/EH helpers, thunks or independently counted shared
tails. Authorship remains conservatively engine_candidate. Original media
were neither patched nor executed.

Dependency review established that004025c0 was missing, distinct from admitted
contact spark00407510. Its small callback family closes both emitter calls
with actual source definitions. All new owners compile in
`src/bh_script_particles.c`, linking actual entity pool, shared VC5 rand,
billboard/oriented draw producers and existing timer/sensor cleanup owners.

The84-byte particle state begins at+e8 and retains original reserved bytes.
Optional velocity/acceleration NULLs zero their actual vectors. Successful
construction consumes one random frame draw; allocation failure consumes none.
Angle/size rates scale by exact PE binary64 1/30. Alpha slope computes
alpha*(-1/30)/lifetime without an intervening float store; masked divide-by-zero
behavior remains visible. Each step subtracts lifetime and tests before the
float store's rounding can affect the expiry decision, releasing before
movement on zero/negative/unordered result. Live particles accelerate, move,
increment angle/size, damp their rates and add alpha slope. A subtle x87
contract is retained: X velocity's unrounded sum survives its own binary32
store into the X position addition, while Y/Z position reload their rounded
velocity stores. A16777216+1 cancellation fixture distinguishes those paths.

Fountain and steam pause every effect, timer and random draw while the
actor's one-level activity owner is inactive. Positive countdowns subtract
exact1/30 and detect expiry before rounded storage; zero/negative/NaN starting
countdowns remain unlimited. Expiry forces an emission without a probability
draw, then releases the actual emitter. Otherwise the shared CRT draw is
compared to11000/10000. Fountain draws spin, growth rate, angle then the actual
constructor's frame; steam draws three component jitters, spin, angle, frame.
Call-argument binary32 stores precede constructor scaling. Steam refresh
consumes two draws, queues actual oriented geometry with actor Y minus exact
binary64 1.6, fixed up(0,0,1)/normal(0,-1,0), signed integer dimensions and
unit alpha. Queue saturation remains a dropped draw; no particle is deleted.

Source adapters bind explicit persistent lifetime/particle/emitter contexts
in private payload+440/+448 and retain real original field ownership. They
install the admitted sensor query/deferred-delete owner and actual new steps.
Actor table, optional owner, pool, queue and borrowed texture identities must
outlive their effects. No invented global/vtable, empty callback or engine
stub is supplied. Full VM and world resource construction remain unadmitted.

`python META/m4/hour407/a/build_check.py` compiles all real dependency TUs
with pinned SP3 CL and stock VC5 LINK, links
`build/script-particles/bhunter-script-particles-check.exe`, passes446 checks
and proves missing-module link failure. Fixtures cover sixteen constructor
random seeds, exact scalar/vector fields, real queue payloads, positive tick
math/damping, X precision retention, zero/NaN particle expiry, true allocator
failure,24 forced emitter fixtures,38 gated probability fixtures, exact CRT
stream advancement, owner precedence/activity freeze, actual sensor/deferred
cleanup and steam refresh geometry/random order. Reference binary32 rate
values are explicitly stored before comparing to avoid VC5 x87 expression
precision artifacts. Native devices/audio playback, game/original code,
automated gameplay, UI/input and network execution are absent.

Parent integration adds `src/bh_script_particles.c`, `tests/script_particles.c`
and calls `int __cdecl bh_script_particles_checkpoint(unsigned long *)`;
scenario is `script-particles`. Parent owns real/clean full-build evidence
and ledger admission. No raw-byte match is claimed; PC24 and NaN payload
behavior remain open. M3 remains skipped and M4 incomplete.
