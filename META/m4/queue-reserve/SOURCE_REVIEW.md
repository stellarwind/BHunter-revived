# Independent final source review

2026-10-25. Static review of frozen src/bh_queue_reserve.c/.h against complete
00477530/00477360 instructions, external jump table and PRE_REVIEW. No source/
test edits, native gameplay execution or new implementation. **No material
valid-domain mismatch found.** source-review-hashes.json pins current reviewed
bytes; verification separately rechecks original bodies/table and focused hashes.

Original capacity decision is signed JL at0047753f. Source unsigned count/full
checks are equivalent inside existing queue ownership contract9<=count<=capacity
<=65535. Arbitrary high-bit/negative original counts/capacities are outside that
contract; this review does not extend equivalence to them. Unsigned selector JA
at0047754c maps every representation outside0..8, including all negative signed
selectors, to header0. Original table mapping1,2,3,4,5,0,6,7,8 is copied exactly.

Reservation computes new slot from old count, increments count once, copies
old-tail next into new next, points old-tail next at new slot and updates header
tail. Source stable indices represent original stable arena pointers with same
0x54 stride/+50 next position. Source does not assign kind, resource or any
of18 payload words in newly reserved command; existing bytes remain intact.
Only link/header/count/error metadata changes. Full queue returns NULL without
allocation/link mutation. Invalid binding/count/topology failure diagnostics
are explicit source guards, not recovered malformed-pointer behavior.

Callback00477360 loads +08 mode regardless of kind/function word, invokes actual
0045b500 source bh_render_mode, then loads current +50 next. Source likewise
does not enforce a semantic kind test and publishes next after real call even
when source mode owner reports failure. Context/command/next must not overlap;
the source signature's returned status and separate next output are adapters
for original next-pointer return. No fake render state owner is introduced.
bh_queue_mode is correctly labeled convenience adapter only; it reserves then
sets BH_DRAW_STATE/+08, without claiming a third original function.

Named parent integration/clean execution evidence remains separate. This static
review establishes no byte-match claim, worker/gameplay implementation, or
validity of arbitrary pointers/counts outside the bounded source arena contract.
