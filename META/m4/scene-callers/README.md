# Scene caller dependency restoration

The timed scene-caller dependency block restores 52 complete owners: 234/727 admitted original functions in 203 translation units; 3,536,976 archive checks, 3,044,022 clean checks and 99 missing-module link controls. Every admitted variant compiles in its real translation unit, links against real definitions in build/m0/bhunter-integration.exe, and passes affected source-built checks. Real and clean source hashes agree. No original code or game execution occurred.

Timed source-restoration block, 2026-10-25. The user explicitly authorized parallel agents and required stopping after one hour. `session.json` records the time boundary. All execution is confined to named source-built checks and offscreen native fixtures; no game was launched.

The requested owners `0041c860` and `00428f50` are full scene render callbacks, not constructors. They remain unimplemented. Their reviewed boundaries and actual construction/callback/resource relationships are in `../scene-callers-review/REVIEW.md`. This block restores complete dependencies rather than replacing their missing bodies with no-op callbacks.

The source closure includes camera command ordering, all twelve camera modes and look-at, state initialization/update/correction, actual player spawn settling, collision/portal segment traversal, actor basis samples, input profile binding/packets/events, route advancement and flyby motion/credits, real background ships/flares, atlas glyph/image copying and queued text, horizon color tables, and collision resource initialization. The lazy collision cache is an additional complete owner; existing query translation units retain their earlier explicit metadata binding and are not silently rebound to its arrays. Explicit contexts replace original globals; ownership/bounds/dispatch bindings are separately recorded adapters. Unknown engine provenance remains unresolved. CRT/compiler helpers/imports/shared tails are excluded from new function counts.

`owners.json` identifies every admitted complete original owner and its source, review, scenario and contiguous original body. `original/*.tsv` is verified instruction-for-instruction against the immutable EXE and function map. These are provenance observations, not raw-byte-match claims for the source implementations. `validation.json`, `integration-real.json`, `clean/clean-checkout.json` and both link maps bind the current implementation variants to actual compilation, real definitions and successful checks. Source snapshots preserve exact tested bytes. The clean checkout has neither original assets nor a bundled compiler; its source hashes must equal the real-data build.

The initial clean attempt correctly failed because the build driver required a positive count from the new archive-only route fixture when no archive was present. Its failed report is retained in `clean-initial-failure.json`. The fixture now explicitly reports `archive_skipped`; only that exact no-archive/zero-failure condition is allowed. Synthetic route checks remain required in both builds.

Native BMPs exercise real two-room construction through source player/camera settling and camera queue consumers; cached/after-gate output matches the first view, a second room differs, and the disabled-world control is blank. Actual background ship BSPs and white/blue flare aliases produce pixels; missing bindings produce blank controls. Native offscreen checks are not gameplay or startup recovery. No pixel comparison here establishes universal original fidelity.

Numerical validation covers bounded moderate masked-PC53 contracts, including explicit x87 stores/phase boundaries and trigonometric formulas reviewed against instructions. PC24 behavior, extreme/nonfinite/transcendental edges and human gameplay fidelity remain unresolved. The full horizon renderer, software primitive/raster kernels, HUD, input polling, weapon lifetimes, full player/flyby constructors and startup registry/script bindings still block the two scene callbacks and a playable game. CD audio and DirectPlay obligations remain in the restoration plan.

Audit with `python tools/check_scene_callers.py`. The block ends with updated per-implementation ledger evidence; M3 remains skipped and M4 remains incomplete.

## Focused checks in the admitted closure

Archive-only native/real-graph fixtures retain explicit absent-asset controls or skips in the clean checkout. Totals below are assertions, not original instruction matches or gameplay completion.

| Source scenario | Archive checks | Clean checks |
|---|---:|---:|
| actor-basis | 911 | 911 |
| atlas-text | 1,124 | 1,053 |
| camera-curve | 73 | 73 |
| camera-modes | 295 | 295 |
| camera-queue | 23 | 23 |
| camera-route | 15 | 15 |
| camera-route-real | 31 | 0 |
| camera-state | 1,452 | 1,452 |
| collision-cache | 128 | 20 |
| collision-resource | 40 | 37 |
| flyby-camera | 52 | 52 |
| horizon-color | 131,350 | 131,350 |
| input-evaluate | 37 | 37 |
| player-camera | 1,030 | 1,030 |
| player-input | 77 | 77 |
| player-input-events | 267 | 267 |
| scene-background | 32 | 32 |
| scene-background-native | 154 | 1 |
| scene-callers-native | 101 | 1 |
| text-queue | 282 | 282 |
| world-segment | 52 | 52 |

All listed scenarios report zero failures. See [resumption frontier](NEXT.md) for remaining owner/dependency boundaries.
