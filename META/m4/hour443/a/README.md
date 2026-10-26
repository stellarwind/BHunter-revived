# Explosion construction and installed effect closure

Four complete ordinary application effect owners are restored in the new
`src/bh_script_explosion.c` translation unit. Their exact immutable PE bodies
total 2,127 bytes / 565 instructions. `owners.json` and `original/` retain the
boundaries, raw instruction bytes and SHA256 fingerprints. These are distinct
RET bodies with entity/effect/event behavior, not CRT/import/EH/stack helpers or
separately counted shared tails. Historical game-versus-engine authorship is
unresolved, so provenance remains `engine_candidate`.

| Original | Source owner | Complete behavior |
|---|---|---|
|00402e50|bh_script_explosion_create|Root sound, flash, main construction, blast, debris, damage|
|00402af0|bh_script_explosion_tick|Lifetime, random child construction, radius/strength/angles|
|004032f0|bh_script_explosion_secondary|Specified sound, full initialization, blast/debris, two spatial damage queries|
|00402710|bh_script_explosion_refresh|Eight actual world billboard producer calls|

No admitted source TU was edited. Query callback00402e20 is the actual existing
`bh_actor_target_removed` owner and is not counted again. Flash construction
uses the admitted binder for actual00403640 lifetime and004035e0 refresh.
Debris uses the complete00403120 constructor with its actual physics/refresh
callbacks. The source does not invent a renderer, VM callback, vtable or global.

## Context and numerical contracts

The context borrows the actual entity pool, draw queue, sound queue, debris and
flare contexts; all must outlive the resulting entities. The original default
sound004f54f8 and image004f0090 become resource IDs. Crucially,
`debris->origin_tag` is the single shared attribution word004dfb34, not an
explosion-only shadow. Original fields at e8/ec, f4..1b4 retain their meanings;
the source-private context pointer occupies otherwise unused payload+624.

Root and secondary perform sound request, optional flash acquire, then main
acquire in that order. Main failure retains the sound request and successful
flash and consumes no random draws or attribution. Main success captures and
clears attribution, consumes exactly48 lobe draws, emits the24-byte event1
packet, then consumes one debris-count draw and nine per successful debris.
The count is truncation of `2 + random*magnitude/32768`, using the exact PE double004892c8=-1/32768. Partial real allocation failure preserves prior effects.
Event0 is conditional on the current main attribution after blast/debris: root
uses amount .6 and radius2*magnitude; secondary uses .4 with radius
float(magnitude²)*.05, followed by .5 with radius magnitude*.5. Its damage target
is captured once before those two queries. Actual event17 clears the target.

PC53 nearest/masked arithmetic is required. Separate real `bh_round_float`
calls enforce the original float stores. Tick compares retained age+1/30
before storing/releasing; unordered age/duration does not expire. Child trigger
is ordered strict threshold>random, with threshold magnitude*probability*100.
Distance is float-stored before multiplying normalized random components, each
component is stored before adding position, and child remaining duration uses
stored age. Child magnitude grows1.5 while probability scales .25. Finite
child4.5 from parent3 is specifically covered; there is no arbitrary magnitude
ceiling. After child construction, radius grows using the old magnitude/30,
magnitude decays .95, then eight angles advance by their spins. Refresh size is
radius*.75 - radius²*(-.25), and alpha is1-age/duration with no clamp.

Constructors reject nonfinite input or negative magnitude before side effects.
An actual debris integer count outside0..65535 is an explicit source limit:
nonfinite/pretruncate count>=65536 skips that unbounded debris batch with LIMIT
after preserving main/sound/flash/blast, and continues damage broadcasts.
65535.fraction correctly truncates to65535. Original extreme arithmetic and
PC24 transitions remain fidelity limits. Released/recycled current entities
stop remaining source accesses with MUTATION; borrowed point/context lifetime
and legitimate pool topology remain caller requirements.

## Validation and parent integration

Run `python META/m4/hour443/a/build_check.py`, then `record.py`.
`build/script-explosion/bhunter-script-explosion-check.exe` passes297 checks
with zero failures. Fourteen actual source TUs compile under pinned VC5 SP3
flags and link with snapshotted admitted renderer dependency objects. Every
compiled source/header context and retained object fingerprint is recorded in
`standalone-build.json`; removing the new module fails the link. This is not
byte-match evidence and does not substitute for parent full/clean integration.

The fixture checks16 independent CRT seeds against scalar/lobe expectations,
the actual installed callbacks, shared origin, exact random consumption,
sound-request state, real admitted debris blast physics, distinct actual spatial
query radii, ordered secondary packets, whole child construction, positive
quadratic billboard outputs, queue saturation, unordered comparisons,
pre-store expiry, strict equal threshold, partial pool allocation failure,
integer-count boundaries and released/recycled current safety. Packet observers
and allocator/mutation fault injectors exist only in the fixture; production
callbacks resolve to real admitted source. No game/reference code runs, no
playback, networking, input or UI capture occurs.

Parent integration adds `src/bh_script_explosion.c` and
`tests/script_explosion.c` to its actual build and calls:

```c
int __cdecl bh_script_explosion_checkpoint(unsigned long *checks);
```

The standalone macro is `BH_SCRIPT_EXPLOSION_STANDALONE`. All four owner entries
remain standalone-linked candidates pending the parent's current full and
clean integration checks. Source-built gameplay startup/world/VM attachment
and original artwork/audio content fidelity are not claimed by this family.
