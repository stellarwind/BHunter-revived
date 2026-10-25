# Complete camera/path interpolation owner 004341a0

2026-10-25. Classified before implementation as `engine_candidate`, authorship
unknown: a normal complete leaf function used by cinematic camera update
`00429fb0` and several game path/movement owners (`00434920`, `00437ae0`,
`00434810`, `0043a1d0`, `00435e80`, `004357d0`, `00439910`, `004366d0`). No
verified library signature, import/thunk, EH/unwind helper or shared-tail role
was found. Its contiguous 543-byte, 185-instruction body ends at `004343be` with
RET. `extract_review.py` verifies the complete exported instruction body against
the immutable original PE and records its hash in `owners.json`.

The six cdecl arguments are float parameter, output vector, first endpoint,
first tangent endpoint, last tangent endpoint, last endpoint. At parameter <=0,
the first pair is linearly extrapolated. At parameter >=1, the last pair is
linearly extrapolated using 1 minus parameter. In between, smoothstep
`h=3*t*t-2*t*t*t` blends the two linearly interpolated endpoint pairs. This is a
quartic interpolation, not cubic Bezier interpolation. No original code or
unrecovered callback is required by this complete owner.

Recovered binary32 lifetimes are explicit: square, 1-t, all three first-line
coordinates, blend h, and final output coordinates. The last-line X/Y sums
remain retained double values, while its Z sum is stored/reloaded binary32.
The complement of the stored h is retained for output X and stored/reloaded
binary32 for output Y/Z. The upper extrapolation's 1-t is double, despite the
old decompiler's float local. Code uses the existing `bh_round_float` owner to
make actual stores explicit without changing the pinned compiler flags or
borrowing byte-match evidence.

`src/bh_camera_curve.c` is the real translation unit, exposed through
`src/bh_camera_curve.h`. Output may replace any entire input vector; partial
overlaps remain outside the explicit source contract. Moderate finite inputs
are checked under masked PC53/nearest; nonfinite payload bit identity and PC24
remain unvalidated. Whole-vector aliases retain all original valid reads.

`tests/camera_curve.c` performs 73 checks: exact endpoint/extrapolation and
rational quarter/midpoint controls, endpoint derivatives, four whole-input
aliases, and sixty control evaluations over four captured real TEST.LEV
route-point fixtures. The latter use independently expanded quartic weights
as the analytical oracle. Reusing goal/portal/projected/anchor geometry as the
four input points does not assert every original caller selects them in that
order or claim cinematic camera update completion.

`check_standalone.py` compiles the actual source/test TUs with pinned VC5 SP3,
links named source-only `build/scene-callers-review/camera_curve.exe` with real
definitions and runs 73 checks with zero failures. Exact commands are saved in
`standalone.json`. Parent current integration and affected checkpoint checks
are still required for final admission. No gameplay or game launch occurred.
