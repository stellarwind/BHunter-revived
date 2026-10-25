# Complete radar marker and corrected rectangle UV dependency

2026-10-25. Complete0041b650 is now maintainable source in `src/bh_radar.c`,
compiled in its real TU, linked with real producers and exercised through real
native world/screen rendering. An existing rectangle UV error discovered while
closing this dependency is corrected as a **new source adapter variant**.
No original code was executed and no game was launched. Parent integration
build/scenario admission is required separately.

## Boundary and provenance review before implementation

| Entry | Complete body | Classification and outcome |
|---|---|---|
|0041b650|0041b650–0041bbe9,1434 bytes|game/high confidence; actual radar marker called by player scene0041c860 at0041d1af and entity HUD traversal0041bbf0 at0041bccd; recovered whole owner|
|0041b2d0|0041b2d0–0041b64c,893 bytes|game/high confidence; HUD frame setup plus attachment draw/cleanup. Remains not_started because00417f70/00417f30 are missing|
|0041bbf0|1029 bytes|game/high confidence; HUD/entity traversal, event3 queries and projected corner/label indicators; remains unimplemented|
|0041a3d0|3826 bytes|unimplemented larger player HUD dependency; not selected in this block|

0041b650 and0041b2d0 are complete contiguous RET-ended functions with ordinary
callers and explicit game HUD state. Neither is CRT, compiler EH/unwind/stack
machinery, import code or an independently counted shared tail. The restored
owner calls only the real recovered quad producer00478730 and screen rectangle
producer00478f60, twice each across branches. FSQRT is mathematical/runtime
behavior, not a separately counted gameplay function. The original fourth
argument is tested with integer TEST ECX,ECX at0041b6a1, not a float comparison;
the decompiler's float signature and raw-bit1 representation are misleading.

## Source bindings and complete behavior

`BHRadarFrame` explicitly owns the caller-supplied values of the scattered
original HUD globals. It is not an assertion that they formed a binary struct.

| Source value | Original binding |
|---|---|
|origin|004df1b8..004df1c0|
|axis_x|004df218..004df220|
|axis_y / quad up|004df228..004df230|
|quad normal|004df1a8..004df1b0|
|yaw_x|004d18f8/004d18fc/004d1900|
|yaw_y|004d18e8/004d18ec/004d18f0|
|unit/radius/screen X/screen Y|004d1a78/004decec/004ce2a8/004ce444|
|flat / visibility word|004d1a9c/004f556c|
|marker / range-bar image handle|004f54dc/004f00b4|

`bh_radar_bind` is a source binding adapter, not HUD setup0041b2d0. It borrows the
frame, queue and source image handles. The marker owner reads only player and
target positions; their typed vectors replace original entity+0c accesses.
No guessed entity globals, fake vtables, weapon callbacks or fabricated model
definitions are supplied.

The complete owner retains both world quad and flat rectangle paths, absolute
height rejection/fade, extended height clamp, radial threshold and normalization,
visibility word bit8 gate, exact-kind1/2 size tests, low-bit atlas quadrant
selection, marker suppression and the separate exact-kind1 range bar. Suppressing
the main marker does not suppress that bar. Kind5 shares quadrant bit1 but does
not inherit kind1's half-size/opacity/bar behavior. Queue exhaustion still drops
each independent publication without cancelling later ones; the source context
reports attempts, actual publications and drops separately.

The numerical contracts include:

- Height scaling consumes the retained Y subtraction; deltas used by yaw rows
  separately store binary32. Extended mode means any nonzero raw fourth word.
  Absolute height scales by0.01, clamps to1 only in extended mode, then that mode
  multiplies by0.7. The ordinary inclusive100-unit boundary retains a zero-size
  marker; values above100 are rejected.
- Yaw dot products scale by0.0025. Each squared-radius term multiplies the retained
  coordinate by its separately stored binary32 coordinate, then stores the sum.
  The cutoff is stored squared radius>double0.9, not radius>0.5. Extended outside
  markers normalize both stored coordinates, then replace radius with0.5.
- An independent finite fixture expects opacity bits3eb5bc94; using both stored
  coordinates for squaring incorrectly yields3eb5bc96. This precise path is tested.
- World placement retains `(axis_x*x + origin) + axis_y*y` ordering, with0.1
  marker and0.065/0.085 bar offsets. Flat range-bar X subtracts a negative0.65
  coefficient; screen depth subtracts negative0.001 from live near_z (1.001 at near1).
- Kind1 forces marker opacity1 but its bar uses radius. Screen integer5*unit
  retains x86 unsigned32 wrap before signed conversion. Source guards reject
  invalid/nonfinite bindings and numerical overflow rather than claiming the
  original's malformed-input behavior. PC24 remains open.

## Rectangle UV fidelity correction

Static review of full original00454fc0 proves the positional UV contract is
**U-start,U-extent,V-start,V-extent**: local104 is param9+param10; local98 is
param11+param12; U and V flips use param9 and param11 respectively. The existing
native adapter instead interpreted the same words as U-start,V-start,U-extent,
V-extent. Existing tests passed equal middle values or white/weakly observed
textures, masking the error. Radar supplies visibly different U-extent/V-start,
making this a real closure defect.

The native adapter `bh_pickup_rect_draw` now uses the original pair order.
`bh_pickup_rect_queue` retains exactly the same positional raw words and
`bh_pickup_rect_execute` retains the same forwarding order. Their parameter
labels/documentation now describe that order. The private label wrapper's
labels were corrected too; its actual constants and positional values already
matched the original pair order and have **not been reordered**. Thus queued
pickup label payloads and pickup owner submissions remain unchanged, while
their actual atlas sampling is corrected.

The previous source/header/test snapshot and hashes are retained in `prior/`.
This correction must be recorded as a new current adapter implementation
variant with fresh tests. Historical linked/byte evidence remains bound to its
old sources; no byte-match evidence is transferred. Whole00454fc0's software
dispatch remains unfinished, so this native composition stays an adapter.

## Focused execution evidence

`check_rect_uv.py` freshly compiles real `bh_pickup_draw.c` and focused test TUs,
links with real source objects/system libraries, and executes:

- `build/radar/pickup_draw.exe`:91 checks, including asymmetric U/V values under
  all eight flip/transpose combinations.
- `build/radar/rect_uv_native.exe`:22 checks. A real native four-color quadrant
  atlas independently renders red, green, blue and yellow in four rectangles.
  The prior wrong interpretation loses the lower-quadrant selections; a white
  texture cannot satisfy this positive-control design.
- `build/radar/pickup_native.exe`:77 checks with the real archive pickup model,
  glow and numeric atlas through constructor/scheduling/resource/render owners.

`check_radar.py` freshly compiles `bh_radar.c`, the corrected real rectangle TU
and the two radar test TUs, links named source-only targets and executes:

- `build/radar/radar.exe`:39 checks. All gates/modes, exact kind tests, independent
  coordinates, fade/size/UV/depth payloads, suppression and exhaustion, plus the
  retained/stored precision distinction.
- `build/radar/radar_native.exe`:57 checks. All four marker quadrants reach
  actual native pixel readback through both the real world quad queue/consumer
  and corrected screen rectangle queue/consumer. A suppressed outside marker
  produces a separately checked blank image.

`verify_evidence.py` checks full original instruction bodies against the pinned
immutable PE, constants and tested source hashes, then writes `evidence.json`.
These are source fixtures, not automated game execution. Real DirectInput/CD
audio, software rasterization, complete HUD setup/attachments/player callback
and playable source game remain separate work.

Parent integration adds `src/bh_radar.c`, `tests/radar.c`,
`tests/radar_native.c`, `tests/rect_uv_native.c`, SDK flags and checkpoints
`bh_radar_checkpoint`, `bh_radar_native_checkpoint`, `bh_rect_uv_native_checkpoint`.
Required source map symbol is `_bh_radar_marker`. The existing pickup-draw and
pickup-native checkpoints must run against the corrected current TU as fresh
evidence. Named focused links now include normal `winmm.lib` because the shared
real source integration object set also contains the newly restored joystick
collector; no new input execution is performed by these tests.


## Runtime near-depth correction

Final00451720 review proves0049175c receives current near_z, not a fixed1.
`BHRadarFrame.near_z` now binds that live original value; flat marker/bar depth
is stored near_z-(-.001). Source tests exercise near .125 and expected .126;
native radar tests use actual view near .01. Initial source/test snapshots are
retained under `near-correction-prior/`. Both focused radar targets were freshly
rebuilt/relinked/executed for this corrected source variant (39/57 checks).
