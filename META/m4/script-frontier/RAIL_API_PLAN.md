# Four-owner RC rail/path source plan (no implementation yet)

Whole originals004458a0/00445910/00445a00/00445e70 total2156bytes,
604instructions, all contiguousRET-terminated and PE-verified. Direct external
calls are verified CRT isdigit0047dec0/_strnicmp00488370 only; raw square root
and FSIN/FCOS are instructions, not missing engine callbacks. No entity or VM
callback is installed or invoked by this family. Prefix0049060c is exactly
`RC_`; the adjacent literal is RCWAGON. Exact runtime rail-use role should be
confirmed against later consumers, rather than called generic script parsing.

Proposed new module `bh_rail_paths.c/.h` (separate from admitted BHScene reader):

- BHRailSegment: BHVec3 start,end;float rise,length; next,previous pointers;
  long visited. This matches11DWORD/44byte stride on pinnedx86.
- BHRailPaths: owning200-segment allocation via actual BHSceneAllocator,
  signed count and signed chain_count; diagnostics. No fake global layout.
- Adapter init/dispose owns the single allocation. Context and allocation must
  remain at stable addresses from registration through linked-path consumers;
  no shallow-copy/reallocation after links are constructed.
- `bh_rail_near(const BHVec3*,const BHVec3*)` owns004458a0.
- `bh_rail_add(BHRailPaths*,start,end,position,rotation)` owns00445910.
- `bh_rail_placement(BHRailPaths*,name,position,rotation)` owns00445a00.
- `bh_rail_link(BHRailPaths*)` owns00445e70.
- Checkpoint `bh_rail_paths_checkpoint(const char *archive,unsigned long*)`,
  scenario rail-paths, real LEV placement adapters borrowed for integration.

Counts/global identity:004dfb54 counts registeredsegments (cap200 exactly),
004dfb58 counts selectedchainheads. Segmentstorage begins004d2128. Add transforms
both endpoints aboutY using rotation[0] only: x'=x*cos+yaw z*sin,
z'=z*cos-x*sin, y'=y. Each transformedcoordinate storesfloat BEFORE translation
adds position and storesfloat again. It increments count before writing; sets
next/previous/visited zero, leaving rise/length unchanged until final linking.
Original count==200 returns; explicit source guard must also reject negative or
>200 rather than write outside owning storage. Overflow does not advance count.

Placement accepts case-insensitive threebyteRC_ prefix followed by CRT isdigit,
then digit0..9 selects hardcoded local endpoints. Modes0..6 add one segment;
7..9 add three in original order. Raw input is only rotation[0], not a fabricated
matrix. Use public source vectors and borrowed actual LEV placements; maintain
caller original flag&3 yaw conversion when composing real level fixtures.

Link scans first unvisited segment in original order, swaps it with segment at
chain_count, and repairs both existing neighbors of the displaced segment.
The selected start is marked and chain_count increments. It scans all unvisited
segments for endpointmatch (strict squared-distance threshold0.25), rejecting
same/opposite duplicate coincidence as raw specifies. A reverse endpoint match
swaps both candidate endpoints before linking. First matching segment wins;
chains may close to their selectedhead. It finishes by writing rise=end.y-start.y
and Euclidean length for EVERY segment, including reversed/swapped entries.
Start/end pointer repair and original ordering are observable; avoid sorting or
rebuilding all links in a different order.

FP profile: pinnedPC53 nearest/masked, moderate finite inputs and actual LEV
quarter-turn angles. Raw FSIN/FCOS hold extended results; existing matrix
rotation rounds trig tofloat and therefore must NOT silently replace this code.
A maintainable CRT sin/cos translation with explicit coordinatebinary32 stores
is acceptable functional source with documented extreme-angle/PC24/last-bit
transcendental uncertainty, consistent with existing geometry policy.
004458a0 has mixed spills: raw dx and dz are each multiplied by their own
binary32-rounded difference, while dy squares without a floatspill; sum order
is(dx*rounded_dx+dy*dy)+dz*rounded_dz. FinalTESTAH1 returns true for less OR
unordered (NaN), not just C '<'. Link's inline endpoint comparisons need their
own raw spill/ordered/unordered audit before implementation; do not substitute
one guessed generic distance test everywhere. Constants/hardcoded mode endpoint
floats must come from rawstores, including6.66/13.33 bitpatterns.

Meaningful tests: independent literal modepoint tables, all10modes and0/1/2/3
quarter-turns; cap200/drop-tail modes7..9; caseprefix/non-digit no-op; single,
reversed, closed/disconnected/duplicateendpoint chains; stable firstmatch and
postswap neighbor repair; exactthreshold/equality/NaN path decisions; expected
rise/length and owninglifetime. Real immutable LEV reader outputs can feed all
RC_ placements, but this does not recover full LEV entity/script construction.
No renderer/native/game/input/audio execution needed.
