# Complete in-place point/scale projection

Whole00455840 is129 bytes/46 instructions, ending004558c1. It calls the
actual admitted matrix point-transform004012e0, uses projected camera globals
004e3860/focal004e9c38 and near plane0049175c, and has separate reject/success
returns. Classification is engine_candidate: ordinary proprietary camera
behavior with unresolved historical authorship, not CRT/compiler/import code
or a shared tail. Exact immutable PE bytes/hash and body are retained in
`owners.json`/`00455840.asm.txt`.

The helper snapshots all three world input scalars before calling the real
source transform. Depth below near or an unordered depth/near comparison
rejects with all input bytes retained. Accepted points store projectedX/depth,
projectedY/depth, then focal/depth, overwriting the original world Z with scale.
Near equality passes. Scalar pointers may equal each other: original input
snapshots and sequential output stores preserve which final value wins.
The helper intentionally retains accepted zero-depth masked division behavior
when near is zero; it does not add a depth epsilon or screen clipping.

Source accepts the actual already constructed BHMainCamera/BHViewConfig in
place of scattered globals. The source pointer/FP-profile guards are not
additional original owner claims. Camera/config lifetime is borrowed and
must not overlap output scalars. Existing004558d0 (`bh_pickup_project`) remains
the different already admitted helper which retains depth and returns scale
through a fourth scalar; it adds no count here.

`python META/m4/hour407/b2/build_check.py` compiles actual source TUs with
pinned VC5 SP3 and stock LINK, links the named
`build/project-scale/bhunter-project-scale-check.exe`, passes21 checks and
requires missing-module link failure. Checks use actual matrix definitions and
actual viewport/settings/pose camera construction, including a known640x480
point mapping, behind/equal/in-front depth, unchanged rejected inputs, NaN
depth/near, accepted zero-depth masked division and scalar alias cases.
No dummy renderer, original-backed adapter or original code is used. No game,
automatic gameplay, input, UI, native playback or network operation occurred.

Parent full current real/clean integration is required for admission. Target
draw0043c3f0 has real model/rectangle queue dependencies and now this projection
helper, but its complete model origin, numeric label, gated centered/offscreen
marker paths need a separate reviewed family. No subset of that larger owner
is admitted here. PC24 remains open; M3 is skipped; M4/playable game remain
incomplete.
