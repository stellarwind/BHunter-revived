# Complete RC placement and rail-path data family

`src/bh_rail_paths.c/.h` and `tests/rail_paths.c` recover four whole ordinary
application data owners:004458a0/00445910/00445a00/00445e70,2156bytes and604
instructions. Fresh raw bodies match the immutable original PE in full; raw
and decompiler evidence is retained alongside `raw-review.json`. The functions
have no VM, entity, renderer or native callback dependency. RC_ is the literal
placement prefix; adjacency to script constructors is not their classification.

The source owns one actual200*44-byte segment allocation through an explicit
allocator. It retains both endpoint vectors, rise, length, actual next/previous
links and visited state. Registration transforms yaw-only endpoints, stores
binary32 coordinates before translating, and resets linkage while preserving
deferred metric fields. All ten literal placement modes and partial expansion
at capacity are retained. Linking preserves first-match selection, head-prefix
swaps, repair of displaced neighbors, endpoint reversal, closed-chain linkage
and final length-before-rise calculation. It does not fabricate full LEV entity
construction or an original-global ABI.

The nearness metric retains the original mixed spills: X/Z differences multiply
their own rounded binary32 value; Y squares while retained in the wider working
precision. Sum ordering and strict0.25/unordered semantics match the raw C0 test.
All three repeated inline link comparisons use that same metric. A focused
positive discriminator proves that replacing it with ordinary squared distance
would change a decision. CRT isdigit is called as an actual function with the
raw sign-extended character; verified CRT routines are not extra game owners.

Caller math profile is PC53/nearest/masked. The source uses pinned CRT sin/cos
with explicit coordinate float stores, rather than the existing matrix helper
that first rounds trigonometric values tofloat. Native x87 FSIN/FCOS last-bit,
extreme-angle and PC24 equivalence remain unvalidated. Source yaw support is
finite absolute values<=1000000, including all actual LEV quarter-turns. Owning
count/link/alias guards reject unsafe states instead of unchecked original
writes. The allocator's storage must remain stable until disposal; context and
segment pointers are explicit source lifetimes.

Focused target `build/rail-paths/rail-paths.exe` compiles the actual new TU,
links the current production definition list and runs89 clean checks or6043
real-archive checks with no failures. Commands, source hashes and linked-object
hashes are in `rail-focused-results.json`; reproduction script is
`build/rail_paths_focused.py`. Tests cover all literal endpoints, independent
quarter-turn geometry, mixed-spill threshold controls, NaN, strict equality,
reversed/duplicate/disconnected/closed chains, displaced-neighbor repair,
analytic3-4-12 length, capacity overflow and guarded allocator lifetime.

`rail-level-census.json` independently parses the immutable archive directory
and raw TEST.LEV placement bytes:5815 placements include119 RC_ prefixes, of
which76 digit modes expand into132 segments and43 RC_P placements are no-ops.
The actual source LEV reader supplies the same live placements to the complete
rail consumer; the linked source result forms one closed132-segment loop,
visiting every segment exactly once with reciprocal pointers and matching
geometric endpoints. This is a world-data integration result, not a game run.
The fixture detaches its own archive and shuts down its VFILE ownership.

No original code, game, gameplay automation, editor, audio, physical input or UI
capture executes. Root full-build/admission evidence is separate. Other raw
owners in this frontier remain read-only candidates; their disassembly or these
four source checks must not be transferred into their admission evidence.
