# Five whole loading wedge owners

`src/bh_loading_blit.c` implements complete0044a220(top),0044a520(right),
0044a380(bottom),0044a8a0(left) and0044a720(top_last), with explicit borrowed
image/dimension bindings and target allocation/row0/pitch. Original full-body
role/boundary/ledger review precedes source; raw bodies and constants are in
this folder. Historical proprietary-engine attribution remains unresolved.
No CRT/import/compiler support/shared tail is counted. `__ftol` is reused
through source signed64 conversion and explicit low32 extraction, not asm.

Let w=trunc(width/2),h=trunc(height/2). Each owner stores its rescaled progress
arguments to binary32 through the existing real `bh_round_float` TU before
geometric arithmetic. Top/top_last use scale8; others use4 after subtracting
their exact sector start. Integer numerator recurrences and separate divisions
retain raw operation order. The upper endpoint in right/bottom/left adds1.0
**before** truncation: raw FSUB489e6c subtracts-1.0. Do not replace this by an
integer+1 after conversion, particularly for negative endpoints.

Signed target column pitch is `trunc(pitch/pixel_bytes)*pixel_bytes` for2/4
bytes, while its initial address uses the original full signed pitch. This
preserves odd and negative-pitch behavior. Row-oriented owners retain exact
pitch. Explicit base/byte length/row0 keep negative logical pitch within an
actual allocation. Sources use the BAD second page and active normal LUT;
depth1/2/4 stores match original narrowing. Depth3 and other depths perform no
stores, as the five original functions do. Source guards require accessible
distinct spans, dimensions1..4096, finite from/to within+/-1e6 and the pinned
PC53/nearest/masked profile. Invalid/out-of-span writes fail safely; caller
ownership/cycle or extreme numerical behavior is not silently guessed.

`build_blit.py` compiles actual new source/test TUs plus real round/FP units,
links real current integration objects into `bhunter-loading-blit-check.exe`,
then executes focused checks. All source/header/fixture/object hashes and
commands are retained in `standalone-build.json`. No admitted image/header
variant changed. Root owns current combined integration/admission separately.

The focused scenario passes5056 checks:

- 100 static independent rational geometric fixtures cover all five owners,
  four dimensions including odd dimensions, full/partial/equal/reversed/
  overshoot progress, all1/2/3/4-byte depths, padded and negative pitches.
  Complete actual target bytes and untouched padding, store counts and source
  immutability are compared, not merely sampled output pixels.
- Real protected end pages exercise exact source and target ends for all five
  owners and four depths. Bad bindings, short source/target and nonfinite
  conversion guards execute separately.
- 40 full320x240/640x480 static golden-footprint/store-count fixtures cover
  sector endpoints and representative nonbinary32 progress intervals. All
 1/2/4-byte outputs have independent CRCs, including odd-pitch typed column
  behavior. The Python oracle uses sequential binary64 operations in raw
  arithmetic order, corresponding to moderate PC53 x87 rounding. It is a
  source/raw-contract oracle, not an original-execution or byte-match claim.
  `full-fixtures.json` records comparison against exact rational geometry;
  the selected40 fixtures have zero endpoint differences. This is scoped
  evidence, not a universal rational-equivalence or PC24 claim.

Independent raw/source audit confirmed all five endpoint transforms, numerator
updates, left-column order, signed pitch and conversion semantics. No game,
original code, UI, input, audio or API fakes execute for these checks.
