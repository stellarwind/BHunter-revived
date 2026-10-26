# VM, parser and placed-world audit at453

Read-only audit against453 active linked owners. No source or ledger was edited,
no new implementation was assigned, and no original/game/input/audio/network
execution occurred. `findings.json` retains graph summaries, exact evidence
paths and input hashes. Existing raw/notes were preferred; no fresh decompile
or broad Ghidra metadata repair was performed.

1. **00443ff0 is a real high-impact callback gate, with incomplete cataloguing.**
   Its absence from function-map/call-edges is not evidence of a typo: immutable
   PE0044586e pushes00443ff0,00445873 pushes the new entity, and00445887 calls the
   admitted query installer00476e90. The target begins a normal cdecl stack frame
   and entity-query event dispatch. Terminal RET4456e2 and following switch data
   are also real; the5875-byte code envelope still needs complete reachable
   code/data ownership review before becoming a whole-function boundary claim.
   Graph fan-in/out is unavailable, not zero.

   Current retained VM evidence leaves four direct constructor dependencies:
   **0043d2c0,0043bec0,0043bca0,0043ddc0**. It additionally installs missing
   **0040fe80,00410020,0043c220** queries; their actual pointer immediates are
   PE-verified at44516b/44521f/44531d/445413. Thus “only four callees remain” is
   incomplete. The last two support block damage/owner and radar-related state;
   their full callback semantics/boundaries still need their own review.
   The next concrete producer is0043d2c0: all eight static direct owners are
   admitted, and target tick/query/draw/physics now exist. Its work is actual
   constructor/model/placement/registry binding, not a missing library callee.

   Once the real dispatcher closure exists, its221-byte allocator004457c0 gates
   **four known direct pending callers**: three statement builders and the
   parser. VM→factory→three builders→parser→block-list is at least **seven
   missing whole owners**, not seven automatic admissions. That dependency
   cascade, not a large outgoing degree, gives this vein leverage.

2. **004111e0 completes applied sidecar scripts after that gate.** Its3351-byte/
   1080-instruction parser has four true missing direct definitions:
   410d90/410f60/411080/4457c0, totaling1301 bytes. Integer/float argument readers,
   wait-for handlers and timer owners are already admitted.0047def0 is the
   independently verified CRT `_isspace`, not another gameplay assignment.
   The parser's sole static caller00411f00 is the block-list prerequisite of
   startup004474c0. The current `BHBlockLibrary` explicitly retains **unapplied**
   sidecars: it lacks the original0x174 template/variable/opcode graph contract.
   That typed source work remains after real callback closure; a654-byte outer
   loader or data adapter cannot substitute for it.

3. **004100d0 is the placed-world ownership bridge.** The1216-byte/367-instruction
   loader has14 distinct static direct targets, all source-real or verified CRT.
   Nevertheless it installs missing room callbacks0040d140/0040e420 and can
   immediately execute location scripts through timers/VM. Its genuine work is
   circular placed-world ownership, live variable slots, model/catalog identities,
   matrices, timer graphs and current-room registries. Only one direct caller
   appears in the graph: startup474c0. Leverage comes from producing the world
   state consumed throughout gameplay, not a fabricated caller count. Related
   reset/instantiate00410590 has all five static callees real but the same
   immediate opcode20→timer→VM requirement; its two known callsites are in
   missing mission owner0041e850. A location-only branch is not whole closure.

Whole startup004474c0 is consequently a later integration owner, not an easy
prerequisite:9584 bytes/2572 instructions,39 distinct direct targets and four
missing direct callees100d0/11f00/21d20/2a9e0. Retained source also references25
additional missing catalogue code owners beyond those calls, excluding CRT and
the save thunk. Installed menu/player callbacks and actual resource/global
binding are hidden by static call-degree summaries. Old notes still calling
explosion402e50, rail-wagon470c0, basic-pickup441b80 or numeric readers missing
are stale and must not drive new assignments.
