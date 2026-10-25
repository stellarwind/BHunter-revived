# Twelve camera modes and look-at

The actual table at original00492d40 has twelve distinct owners. All twelve and
look-at00479c70 are restored in src/bh_camera_modes.c, with real smoothing,
vector and rotation dependencies. Explicit context replaces scattered original
globals. Authorship remains unresolved engine-candidate provenance, not verified
library source. The RET owners and table entries are independent behaviors;
compiler floating-remainder dispatch remains excluded from gameplay recovery.

Modes preserve cockpit offset, chase/tail, reverse, top/bottom/side views,
velocity trail, timed circle variants and stationary fly-by camera behavior.
The first flag, target history, seed yaw, elapsed time and shared CRT random
sequence retain their roles. Mode9 consumes exactly three random calls when
repositioning and none while holding. Mode7 and mode9 normalization quotients
are double stores. Chase modes have intermediate binary32 stores before the
second basis contribution. Raw assembly review found these hidden stores;
source functional validation does not imply original-byte equivalence.

Look-at constructs normalized target-eye and right vectors from the supplied
up vector. Yaw/pitch use x87 atan2 equivalents; yaw sine/cosine are double
stores. Final roll operands have separate binary32 stores. Degenerate zero
vectors preserve the existing recovered normalization semantics. Full outer
camera guards constrain valid source use to finite moderate values at masked
PC53/nearest. PC24, extreme transcendental arguments, arbitrary degenerate
frames and original gameplay visual equivalence remain unvalidated.

Tests use independent Cartesian camera positions for all12 modes, analytical
look-at yaw/pitch over a100-direction grid, aliasing, rotated up, timed orbit,
trail, and real CRT random-count/reposition fixtures. The separate camera-state
scenario drives every mode through full update, blending, slew and velocity.
Main integration/native evidence and admission live in ../scene-callers/.
No original code or gameplay is executed.
