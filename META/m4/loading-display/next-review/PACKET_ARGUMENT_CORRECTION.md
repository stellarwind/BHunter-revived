# Original packet ABI and source binding correction

Read-only review; no admitted source, tests, ledger or earlier evidence edited.
The earlier undersized-list destination warning in agent messages is withdrawn.
It arose from assuming the explicit source API retained original argument order.
The corrected WORLD_INPUT_CONTRACT.md is durable and removes that false claim.

Original00475870 has `(packet,state)` order. Raw00475870 loads arg1 toEAX,
reads byte[EAX+1] as record count, and advances EAX through the packet. After
PUSH EDI/PUSH ESI it loads originalarg2 from[ESP+10h] intoESI, and writes indexed
DWORDs through ESI (for example004758d8 and00475937). By contrast the admitted
source intentionally exposes `bh_player_input_apply_packet(state,packet)`;
its checked wrapper uses the same explicit source argument order.

004791c9..dc late-loads list pointer[004f5500], adds2010h, PUSHes immediate
004f5518, PUSHes the computed list packet, then CALLs00475870. Therefore the
faithful source correspondence is:

- `s->packet` = packet buffer at current input-list+2010h (list->state).
- `s->player_input` = independent destination state at original004f5518.
- list identity/packet binding must remain live after entity/sound callbacks;
  current source explicit borrowed packet field is reread at that late call.

004100b0 similarly PUSHes00496168 then input-list[004cda1c]+2010h: source
correspondence is `apply_packet(editor_state496168,current_editor_list->state)`.
The input-list0x120-byte state member is the encoded packet, not the256-word
destination. There is no discovered undersized-state write bug.

Current admitted service source is semantically correct for its explicit
bindings. `src/bh_frame_service.c:52` calls the checked source API as
(s->player_input,s->packet,s->packet_bytes). Its fixture at
`tests/frame_service.c:100` binds `player_input=&player` and
`packet=list->state`, then lines108..109 assert the produced packet updates
only the independent player state. The observer fixture also substitutes both
correct semantic fields late. No production constructor/binder in this module
maps original numeric globals or reverses those actual source pointers.

The defect is misleading header/evidence prose. Smallest corrective variant:
comment/documentation-only correction; no executable body or fixture behavior
needs alteration. Exact stale locations:

- src/bh_frame_service.h:28 identifies player_input as4f5500+2010 (wrong).
- src/bh_frame_service.h:29 identifies packet as4f5518 (wrong).
- src/bh_frame_service.h:49..53 assigns embedded-current-player identity to
  destination and calls list-packet storage merely a fixture choice (wrong).
- META/m4/frame-pipeline/service/README.md:7 repeats both reversed bindings.
- META/m4/frame-pipeline/review/SOURCE_REVIEW.md:23 repeats reversed raw roles.
- META/m4/frame-pipeline/README.md:7 could use precise "late input-list packet
  and destination-state bindings" rather than ambiguous current-player phrase.

Raw BOUNDARY_REVIEW.json's untyped475870(context+2010,&global4f5518) call is
correct and should be retained; it never labels the first argument destination.
The original raw files also remain correct. Source comments should say packet
tracks the current input list, while destination is the distinct global state;
no actual startup/list-construction alias is claimed beyond those raw bindings.

Resolution2026-10-25: root corrected the listed header comments and old service/review prose. Current service C/test behavior stayed unchanged. The final loading-display evidence is refreshed through full real/clean builds; pre-correction exact evidence is preserved under `../history/pre-packet-annotation/`. The stale-line list above records the finding and is historical after that correction.


##Known bootstrap index scope; original destination span remains unresolved

Full raw00479200 is retained and PE-verified (1800bytes/428instructions).
At0047922a..3c it creates system input list004f5500 with mode1. Conditional on
original004f5568!=0,00479243..b2 installs only these bindings:

|Index|Admitted writer|Raw binding|
|---|---|---|
|0|00475810 string/kind5|name at00490f10|
|1|00475810 string/kind5|name at00492d3c|
|2|00475810 string/kind5|REORDER at00492d34|
|3|00475770 single key|key0x77(F8), flags0/modifiers0|
|4|00475770 single key|key0x78(F9), flags0/modifiers0|

Thus this bootstrap branch produces count5 and known indices0..4; when the
condition is false the newly created list remains count0. No generic256-control
system list or1024-byte original destination allocation is established by this
bootstrap. Admitted input evaluation iterates only the live list count and emits
those binding indices into its encoded packet (`bh_input_evaluate.c:16..48`).
Original packet consumption writes DWORD slots from explicit packet indices;
for the unchanged bootstrap system list these are state offsets0,4,8,12,16
relative to004f5518. This is an access-scope observation, not an allocation-size
claim or proof that no later owner can modify the list/packet.

A bounded Ghidra xref query of004f5518 and the four adjacent known-slot addresses
is saved in `system-state-known-xrefs.json`; it identifies the packet consumer
call004791d6 for the base and no additional indexed-slot metadata references.
Absence of analyzed xrefs is not evidence that other original accesses do not
exist. Nearby bootstrap sound stores (e.g.004792ef->004f5514 and
00479312->004f5544) further warn against treating an API's generic state struct
as original global allocation proof.

`BHPlayerInputState.values[256]` is an explicit source destination capacity for
the reusable packet API and isolated source fixtures. It must not be read as a
recovered original004f5518 allocation/span. Original destination ownership and
full accessible span remain unknown beyond these proved accesses. No broad
global scan, original execution or new source owner was added for this note.
