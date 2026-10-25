# Independent camera mode arithmetic review

2026-10-25. Reviewed `src/bh_camera_modes.c` against complete original TSV/C
exports in `../camera-modes/original`. Source changes coordinated with parent
integration owner. This is static review evidence, not a byte-match claim or
independent runtime differential test.

Confirmed argument mapping: the six original mode arguments are state, target,
Euler angles, basis X, basis Y, basis Z. Mode9's inferred `in_stack_00000018`
is the sixth argument: at `0047a7de`, current ESP is entry ESP minus 0x44, so
`[ESP+5c]` is entry ESP+18. Its source correctly consumes `input.basis_z`.
Common offsets +24/+28/+2c carry remembered target position, not a vtable.

Corrected source issues discovered through instruction review:

- `00479c70` stores cosine/sine of yaw to **double** locals (`00479d09`,
  `00479d13`); both later pitch and roll calculations reload `FMUL double`.
  Removed narrowing to binary32 implied by the initial decompiler types.
- Modes1/10 explicitly store target plus scaled basis Z to binary32 before
  adding scaled basis Y. Source now reproduces those intermediate stores.
- Mode2 rounds 15 times basis Z in Y/Z, retains it in X, and separately rounds
  twice basis Y before its final addition. Source now preserves those stores.
- Mode7 retains pre-store vector components for one factor of each squared
  norm term and reloads binary32 components for the other. The norm accumulates
  X, Y, Z in that order. The normalized scale is stored to **double** at
  `0047a3e0`, not float. Final Y/Z scaled offsets are binary32 before target
  addition; X retains its product. Source now preserves these distinctions.
- Mode9 stores both normalization quotients to **double** at `0047a866` and
  `0047a983`. Initial norm uses retained times rounded components; initial Y
  offset is binary32 before target addition. Distance testing uses unrounded
  X squared and retained times rounded Y/Z. Repositioning rounds remembered
  motion Y/Z before scaling, keeps its Z norm product unrounded, and retains
  the X target-plus-offset double through the first random jitter. Its Y/Z
  offsets and target additions have distinct binary32 stores. Source now
  mirrors these lifetime/store boundaries.
- Modes8/11 movement-radius sums accumulate X, Y, Z. Their vertical candidate
  multiplies sine by cosine, then retained radius, then .1. Their phase/radius
  width distinctions remain explicit.

`symbolic_fp_review.py` is a diagnostic straight-line x87 expression tracer
used to cross-check these findings. It stops at the first call and does not
emulate CPU control flow or execute original code. It is not a general original
subsystem adapter. The raw TSV instructions remain the authoritative evidence.

The completed parent integration test run is still required after these source
changes. PC24 differences and extreme transcendental behavior remain open.
