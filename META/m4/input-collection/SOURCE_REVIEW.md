# Independent source fidelity review

Canonical LF refresh: current-byte-audit.json pins the frozen reviewed source
and numeric fixture bytes. Normalized fixture changes are proven CRLF-to-LF
only against saved prior snapshots; static formulas/expected words are unchanged.
This refresh is static verification, not a transfer of prior execution evidence.

2026-10-25. Reviewed src/bh_input_collection.c/.h against complete pinned
004744f0 instruction listing, retained decompiler and constants.json. No source
or original gameplay runs, native cursor calls or source edits in this review.

No material valid-domain discrepancy found. Device0/1 enabled flags correspond
to004eed70/74; calibration latches004eed78/7c. Device axis snapshots are
004ef2dc..e8 and004ef2ec..f8. Four min/max pairs per device map interleaved
004ef300/304 through004ef338/33c into separate source arrays. Capability bits
1/2 control optional Z/R. Deadzone masks are bits1,3,5,7 and9,11,13,15.
Buttons map current+166..169 and+170..173 (004ef162..169), matching source.

Raw joystick values convert unsigned32 to extended/PC53 before subtract32768.
Deadzone branch subtracts5000 on positive side, adds5000 otherwise, then
clips its crossed sign to zero; scale is double4e-5. Nondeadzone uses actual
double1/30000. Active result stores binary32 before lower retained comparison
and upper stored comparison. Calibration always uses deadzone+retained clamps,
updates extrema as binary32, and tests retained difference of stored extrema
strictly greater than double0.5. Source captures prior calibrated status once,
so crossing threshold does not normalize any axis until the next poll. Optional
absent axes are zeroed as original; disabled device snapshots remain unchanged.

Mouse coordinate subtraction preserves32-bit wrap. Accumulation wraps16-bit
then unsigned-clamps values above1023. pair4 selects low16 of each separate
32-bit delta. Mouse filtering uses full signed32-bit deltas and actual double
-0.002/0.99; lower test consumes retained result after FST and upper reloads
stored float. Source follows these arithmetic/storage contracts.

Enabled input lists are evaluated in next-link order after collection; flags2
become1 and evaluate full state; other nonzero flags evaluate edge state.
Only afterwards are255 previous bytes compared/copied and changed count returned.
Source explicit changed output/success return replaces original count return.

Explicit source extensions: failed joystick/cursor API returns reject instead
of consuming original unchecked/uninitialized stack outputs; earlier valid
mutations may remain. SetCursorPos result is recorded without aborting, as the
original ignored it. Null binding/history/cycle guards are adapter validation.
Cycle detection replaces original debug high-nibble fault pass, which was not
functional input behavior. Initializer is an explicitly named binding/default
adapter and does not claim full original00473d60 device discovery/key setup.
Real APIs remain defaults; isolated test overrides must remain outside game
controller logic and native recenter must not execute as a diagnostic.

## Independent numeric fixtures

generate_cases.py reads pinned raw constants, never source C or original code.
Python binary64 supplies PC53 intermediates; struct binary32 rounds only at
original stores. tests/input_collection_cases.h supplies42 unsigned axis cases
covering deadzone edges, endpoints and DWORD high-bit extremes, plus77 mouse
filter cases with positive/negative near-clamp values and signed delta extremes.
numeric-case-evidence.json preserves retained/stored values and clamp decisions.
Calibration expected column always uses deadzone regardless active-mode column.

Apply active axis fixtures with calibrated device0, X position=raw, mask bit1
set/cleared by deadzone; compare snapshot.axes[0] bits exactly. For calibration,
reset latch/extrema for each case and compare observed extremum to calibration
column (initial snapshot axis remains zero). Mouse previous is raw binary32;
set source axis from bits, and mocked cursor coordinate from unsigned32 delta
plus160/100 with wrap. Compare final mouse axis bits. Tests must use isolated
source API overrides, never native recenter. Near-clamp retained/stored decision
differences can converge to identical final binary32 output; fixture data does
not claim final-value checks alone distinguish every internal compare path.
