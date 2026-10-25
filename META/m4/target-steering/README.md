# World bounds and AI exit selection

Two complete source owners extend the weighted-target caller's real dependency
frontier. `004340f0` is world bounds clamping; `004344f0` chooses a connected
room/portal. Initial inspection of the slot stride suggested a possible weapon
selector, but established geometry owners prove that these are portal slots.
No weapon implementation or invented descriptor table is included.

The clamping owner uses the existing `BHWorldGeometry.minimum/maximum` fields.
Its 2,685 checks cover an ordered/reversed bounds grid, infinities, signed-zero
equality, axis-specific unordered comparisons, aliasing and unchanged world
storage for disjoint output. This generic geometry helper remains classified
`engine_candidate`, with library provenance unresolved.

The exit selector uses the real source model portal count, `BHPortal.neighbor`
and world `query_flags`, plus the actual actor class/RNG payload words. It
passes 288,848 checks over eligibility, null neighbors, actor classes, exclusion,
preferred neighbors, original slot order, fallback and exact RNG consumption.
A separate fixture creates reciprocal two-room links through the actual
`bh_portals_match` implementation and then selects those links. No navigation
callback, original global or weapon slot is fabricated.

Both owners compile in their real translation units, link with real definitions
and execute in named standalone targets. Each missing-module control fails on
the expected implementation symbol. Parent integration/admission is separate.

```
python META/m4/target-steering/audit_original.py
python META/m4/target-steering/build_evidence.py
python META/m4/target-steering/route_choice_build.py
```

`review.json` records complete contracts, provenance, source hashes and limits.
`original-audit.json` verifies every exported instruction in both owners and the
existing geometry classifier/constructor against the immutable pinned EXE.
Classifier `00410880` increments descriptor `+18` for `PORTAL.GIF`; constructor
`004084a0` builds 60-byte portal records and the `+1b0/+1bc` world bounds. The
runtime registry `00497230` has 372-byte descriptors and is initially empty in
the file. The selector uses the existing resolved source model instead of
inventing static registry values.

The decompiler's provisional `void` selector result is rejected by the complete
EAX path and actual return-consuming callers. Preferred matches return their
slot before exclusion filtering. If no eligible replacement remains, the
original excluded index returns unchanged, even when negative or unsuitable.
Otherwise exactly one wrapping 32-bit actor RNG transition chooses a rank among
eligible slots. The source geometry contract supports six real portals; the
original ten-word local scratch does not establish ten valid world portals.

No game or original code was executed. No byte match, full AI owner or playable
game is claimed. Signaling-NaN exception/status behavior is unvalidated; the
clamp retains the observed quiet-NaN branch results under masked exceptions.

Parent integration:

- Compile `src/bh_target_steering.c`, `tests/target_steering.c`; register
  `bh_target_steering_checkpoint(unsigned long *)` as `target-steering`, with a
  missing-`bh_target_steering.obj` control. Public symbol is
  `bh_world_clamp_point`, taking the real source world type.
- Compile `src/bh_route_choice.c`, `tests/route_choice.c`; register
  `bh_route_choice_checkpoint(unsigned long *)` as `route-choice`, with a
  missing-`bh_route_choice.obj` control. Existing entity, FP, rounding and portal
  owners supply its link closure. Include source/header hashes in full evidence.


Additional complete route owners:

- `00434810` / `bh_route_curve_length`: 276 checks in
  `curve-length-build.json`. It samples the actual admitted curve evaluator at
  0,.25,.5,.75, preserving three mixed-product chord lengths and each float sum
  store. The endpoint at1 is deliberately omitted, matching the original loop.
  Run `python META/m4/target-steering/curve_length_build.py`; integrate
  `src/bh_route_curve_length.c`, `tests/route_curve_length.c` and cases header as
  scenario `route-curve-length` via `bh_route_curve_length_checkpoint`.
- `00434650` / `bh_route_guided_exit`: 165 checks in
  `route-guided-build.json`, including16 independent mixed-product fixtures and
  all six axis directions. Real center/normal/cross owners choose the closest
  eligible opposite portal by lateral/up squared displacement. Parallel up and
  normal vectors retain the original zero-cross behavior. No RNG is consumed.
  Run `python META/m4/target-steering/route_guided_build.py`; integrate
  `src/bh_route_guided.c`, `tests/route_guided.c` and cases header as scenario
  `route-guided` via `bh_route_guided_checkpoint`.


The first closed owning route update is now source-built too:

- `004343c0` / `bh_vec3_snap_axis` returns retained length while replacing the
  vector with that length on the dominant signed axis. Its1,375 checks cover
  the signed grid, tie priority, signed zero and returned-vs-stored precision.
- `00434f40` / `bh_actor_route_promote` and `004355c0` /
  `bh_actor_route_advance` live in `src/bh_actor_route.c`. Their229 checks cover
  actual room-list transfer, world promotion, control rebuilding and duration,
  real random/guided selectors and actual sound voice expiry. The existing
  `BHPickupRoom.actors` head and verified actor+f4/+f8 links are used; they are
  distinct from entity pool links and the player event list. The sound fixture
  allocates/requests/stops a real source streaming voice without playback.
  Twenty-four independent PC53 cases pin controls, portal centers and duration/
  tick words. Base ticks use stored duration; narrow-aperture ticks use the
  retained quotient, matching the raw body.

Run `axis_snap_build.py` and `actor_route_build.py` in this folder. Integrate
`src/bh_axis_snap.c`, `tests/axis_snap.c` as scenario `axis-snap` via
`bh_axis_snap_checkpoint`; integrate `src/bh_actor_route.c`,
`tests/actor_route.c` and `tests/actor_route_cases.h` as scenario `actor-route`
via `bh_actor_route_checkpoint`. Public field-view adapters are explicit source
bindings, not extra original function admissions. Both original actor-route
owners consume integer portal indices; provisional decompiler float types are
contradicted by the complete argument/index instruction flow.

`original-audit.json` now verifies all nine newly restored owners plus the
existing classifier/constructor, every actor-list ownership anchor, and actual
scalar constant bytes. No game code, gameplay, playback or original-code
execution occurred. Parent full integration/admission remains separate.

Both tick conversions reject nonfinite/out-of-signed64-domain values before
casting. These source safety guards preserve all actor/list state on rejection;
they are not claims about malformed original behavior. Two focused controls
cover overflow in stored base duration and retained narrow-aperture duration.


The direct deadline callback and cleanup close another actual actor lifetime:

- `00435790` / `bh_actor_route_due` preserves the signed16 wrapped deadline,
  advances exactly once, then stamps reloaded tick plus the OLD wrapped elapsed
  value. Eleven boundary/wrap fixtures cover idle, due and overshoot paths.
- `00435720` / `bh_actor_route_release` stops a real voice, repairs the actual
  game room list, then calls the real entity pool release owner. Tests verify
  generation/query/focus/free-list changes and unchanged retained route payload.

Both live in `src/bh_actor_route_lifetime.c` and pass56 checks plus a missing-
module link control in `actor-route-lifetime-build.json`. Integrate that source
and `tests/actor_route_lifetime.c` as scenario `actor-route-lifetime`, checkpoint
`bh_actor_route_lifetime_checkpoint`. The deadline callback's missing live
Ghidra boundary is explicitly recorded: the retained58-byte map and complete
contiguous instructions/ownRET are independently verified against immutable
EXE bytes, and actual constructor code-pointer references are retained. No
function database edits or original execution were necessary.
