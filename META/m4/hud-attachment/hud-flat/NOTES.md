# Flat player status HUD0041a3d0

Classified before decompilation as game/high confidence: complete player HUD
owner called from player scene0041c860, with status/ammunition/counter logic.
No CRT/import/EH/compiler/shared-tail behavior.3826bytes/1123instructions through
0041b2c1 are pinned byte-for-byte against immutable original PE by
`export_review.py`; see `owners.json`, raw JSON/TSV and `constants.json`.
No original code was mapped, loaded or executed. This is functional C source,
not a raw/relocation-normalized byte match.

## Dependencies and explicit source bindings

The real `src/bh_hud_flat.c` TU uses shared observed-field `BHHudStatusState`
and `BHHudWeaponAmmo` from `bh_hud_status.h`. Weapon amount has verified+10c
offset; player reads are gauges+24c/+23c, selected bytes+2b6/+2b7, weapon table
+258, flags+2b8, signed counters+230/+234, pending+238 and mutable countdown
bytes+2b9/+2ba. The compact status view deliberately does not assert a guessed
full player layout or claim to restore constructors/weapon callbacks.

`BHHudFlatContext` borrows the actual HUD context/queue, live frame near_z,
signed viewport width from the attachment context, signed viewport height
(original004e2dec), atlas004f54e4 and weapon icons004f0054. Width scale is
width*.0015625; radar frame.unit is not involved. Every emitted record calls
the real00478f60 `bh_pickup_rect_queue` owner in bucket8, angle/flags0,
modulation1, depth near_z-(-.01double). Existing image loading, UV rectangle
consumer, screen clipping and native rendering remain real separate owners.
All object/image lifetimes are borrowed through queued consumption, with no
overlap/reentrancy. No success stubs or original pointers/vtables are used.

## Full behavior

Two gauge backgrounds always draw; fill requires amount>=0 and either strict
amount<double0.8 or flags bit8. Amounts, sizes and UVs are never clamped.
The observed float0.8 exceeds the exact double0.8 threshold. Fill is bottom
anchored and uses1-amount for height and atlas extent. Primary/secondary ammo
retain signed32 division/remainder by100/10, suppress leading zeros, retain
internal zeros, always emit units and use distinct atlas rows. Hundreds may
exceed9, and negative values preserve negative atlas coordinates. Secondary
weapon icon rows subtract selection base5.

Both counters use verified CRT `pow(10.0,6.0)` plus truncating cast, then seven
signed decimal divisions down to1, with leading-zero suppression. CRT helpers
are reused library code and are not claimed gameplay owners. Countdown bit8
suppresses its number before either byte is decremented. Nonzero countdowns
decrement as bytes; only countdown_b reloads16 when zero afterward and pending
is nonzero. Full queues still execute every later attempt and mutation;
source queue errors are recorded and also permit later attempts/mutations.
Binding/nonfinite guards reject before publication. Unsupported malformed
original inputs are not claimed as recovered crash behavior.

## Numeric review and exact static probes

PC53/nearest/masked moderate finite profile. The two HUD variants differ in
their spills: this flat owner retains1-amount for fill bottom placement and
height (0041a518/1a549/1a557/1a57d), whereas world HUD explicitly reloads stored
remainder for placement. First primary hundreds/tens retain scale*25 and
scale*60; units/secondary reuse cached float bottom/offset. Width/height command
arguments are stored float, but half dimensions remain double. Icons subtract
stored height while adding retained raw half-height. First counter label uses
retained top; later labels/numbers reuse stored top. Counter X increments round
to float each iteration. These distinctions survived full-body review.

`generate_cases.py` constructs seven narrow static arithmetic probes from these
FST/FILD sites; `static-cases.json` retains expected words and incorrect
simplification words. Six odd-width probes distinguish retained dimensions and
first/later placement. The seventh uses width641/gauge bits0x3eee2995 and
distinguishes correct fillY0x43cd4386 from rounded-remainder0x43cd4385. These are
instruction-derived analytical fixtures, not original-runtime differentials.
PC24, exceptional/huge numerical inputs and software rasterization remain open.

## Actual validation

`tests/hud_flat.c` checkpoint has82checks,0failures, including exact scale1
positions/UVs, cropped bars, digit ordering/icons, leading/internal zeros,
signed32 extremes, negative/over999 ammunition, half viewport, live non-one
near depth, overrange gauge flags, blink/countdown/pending transitions,
full/malformed queue behavior and all seven exact precision probes.
`standalone-build.json` records actual compile/link/run commands, source hashes
and object dependency hashes for named `bhunter-hud-flat-check.exe`. The real
flat HUD/queue/pickup-draw/fp/round TUs are compiled, and the remaining real
integration source objects resolve pickup-draw's other definitions; no undefined
symbol allowance or fake dependency is used. Parent evidence records current
full integration and native composition independently. No result here is
transferred to a differently built source variant.
