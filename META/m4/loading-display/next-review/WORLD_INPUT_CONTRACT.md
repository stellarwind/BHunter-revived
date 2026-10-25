# Read-only LEV construction and input setup frontier

No implementation, ledger modification, native/game execution or original-code
execution was performed in this review. `world-input-review.json` independently
verifies all bytes of three complete bodies against the immutable EXE and lists
current source variants. These observations are not source admission evidence.

##004100d0: complete LEV construction,1216bytes/367instructions

The existing `bh_scene.c` variant remains a binary-reader WIP. The whole owner
opens/seeks/tells a VFILE, reads signed placement count and32-byte records,
resolves each16-byte name case-insensitively against the live block registry
(stride0x174), and acquires real pooled entities. It sets entity kind1, preserves
original placement index, raw coordinates/flags and block index, copies eight
short defaults from block+64h, sets script/world fields, derives quarter-turn
rotation from flags&3, converts coordinates tofloat and links a circular world
ring in successful-placement order. Unknown names log through verified CRT
printf and consume the four remaining words without constructing a placement.

Each constructed placement invokes missing00445a00(name,position,rotation).
After ring closure the owner invokes missing00445e70. It then consumes remaining
20-byte link records: placement-index lookup around the ring, eightshort writes
into entity+334h or into a discarded temporary when no matching index exists.
This matches the existing reader's opaque eight-short payload; they must not be
silently reinterpreted as eight navigation indices.

The owner traverses the ring, obtains block+40h script templates and creates
real timer-template entities where present; it binds original00442de0 step and
00442ec0 query, conditionally executes the actual timer activation for a
particular script handler/type20, updates ring collision lifetime, installs
first-placement step0040e420, closes the VFILE, sets world mode2, then runs the
complete admitted geometry004084a0 pass followed by portal00408940 matching.

Direct restored source dependencies: VFILE open/close/seek/read/tell,
`bh_entity_acquire`, `bh_entity_set_query`, `bh_world_geometry_build`,
`bh_portals_match` and timer00442de0 (plus assigned query00442ec0). CRT printf
0047de50 and strcmpi004882e0 are verified library roles, not missing game owners.
Remaining direct ordinary owners00445a00/00445e70 need their own classification
and whole-body restoration. Live block registry/script owner state is also
required; current `bh_blocks`/`bh_scene` reader subsets cannot fabricate it.
Installed unresolved callbacks0040d140 (refresh/editor interface) and0040e420
(world/editor step,6741bytes/2007instructions) prevent a whole real runtime
callback closure. Verified0047d680 RET is already represented by NULL callbacks
under the admitted entity contract and must not become an extra counted owner.

Original hazards requiring explicit source policies: negative/nonzero counts,
all names unknown leaving null ring endpoints, entity-allocation failure returns
without original close, unchecked short reads/unterminated names, invalid block
or link references. A safe source transaction must document changed failure
semantics, preserve placement IDs across skipped names and retain actual callback
lifetimes; passing current data-reader checks alone does not recover this owner.
No new full-world implementation is recommended before its real constructors
and callback closure exist.

##00412190 +004100b0: bounded editor-input family

00412190 is820bytes/265instructions and constructs an input list through actual
restored00475640(mode1), installs28 single-key records, sets slot8 using the
restored kind4 writer004757e0, then acquires one real entity and installs
004100b0 as step if acquisition succeeds. Count ends29. All names borrow the
same live004dfb5c buffer (Ghidra initial memory reads sixteenzero bytes); do not
invent descriptions. Slots13/14 use modifier0x11, all other modifiers/flags0.
Key sequence byslot0..28, with slot8 kind4, is:
`4e,2e,21,22,25,27,26,28,(kind4),02,01,5a,58,43,56,53,4d,71,6d,30,31,32,33,34,35,36,37,38,39`.
In particular, kind4 is not a generic disabled binding.

004100b0 is a complete25-byte/seven-instruction callback. It reloads live input
list004cda1c, adds0x2010, then invokes admitted00475870 with that list+2010 as the PACKET and
00496168 as destination state. Original00475870 has `(packet,state)` argument
order; the admitted source deliberately exposes `(state,packet)`. Raw entry
MOV EAX,[ESP+4] reads packet byte1, while ESI receives originalarg2 and is used
for indexed destination writes. Preserve this ABI translation explicitly.
No native input polling, physical cursor movement, audio or renderer is called.
A real source fixture could verify every binding byte and invoke the actual
installed callback through the admitted entity dispatcher using owned packets.
Allocation failure behavior and context/pool/list lifetime require explicit
source guards. The list's0x120-byte `state` member actually holds its encoded input PACKET,
written by admitted `bh_input_evaluate`. Destination00496168 is a separate
256-word state. Retain the live list packet and destination bindings; do not
reverse them merely because source and original argument orders differ.

Role refinement matters: these are editor controls, evidenced by Ctrl-C/V,
page/arrows, digits and the same destination state's many references from world-editor
0040e420 plus editor strings/clipboard code in0040d140. They are ordinary game
application owners, but not player-control/bootstrap closure. This pair is
small and all direct callees are restored, so it is a reasonable bounded next
source family only with the editor role stated. It does not by itself advance
faithful source-built world gameplay as much as real script-construction work.
Current Ghidra has no004100b0 function; its complete raw boundary is verified,
but no database boundary was created during this read-only review.

##Recommendation coordinated with block-loader review

The block-loader agent reports00411f00 requires complete3351-byte/1080-
instruction parser004111e0, five missing typed readers, and multiple script
factories/installers including unresolved VM00443ff0. It is not a small closed
next owner. Its int/float token-reader pair004109c0/00410ba0 is separately bounded
and free of application callbacks, but should be taken as a deliberate parser
prerequisite rather than called whole loading/world recovery. Prefer that
parser prerequisite for a gameplay-directed next block; use the editor-input
pair only if explicitly choosing the smaller self-contained application family.
Both options preserve the larger whole-world blockers above.
