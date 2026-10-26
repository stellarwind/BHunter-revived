# Attached script sound prerequisite

Recovered whole004436f0, `bh_script_attached_sound_tick`,270 bytes and79
instructions ending at004437fe. It is the callback installed by script VM
opcode1 when its attached sound mode is selected. The boundary has distinct
entry/returns and actual sound-request/update/stop calls; it is an ordinary
engine/game behavior owner, not a library, EH helper or shared tail. Historical
authorship remains unverified, so provenance is conservatively engine_candidate.

The signed current+104 counter approaches target+108 by one before activity
is sampled. Rising1 requests resource+100 at literal binary32 gain1/15;
other changed nonzero levels update handle+fc at binary64 coefficient1/15
rounded to binary32. Falling0 stops the real voice and writes handle -1.
Equal counters perform no sound operation or spatial update. Both request and
update use actor+f0's actual position and a non-NULL zero velocity; the
decompiler omitted the fifth request argument, but disassembly preserves it.
The actor's one-level owner controls activity; cleanup still identifies the
original actor. Invalid handle plus any nonzero activity sets target15;
every other handle plus zero activity sets target0. Failed/stale calls retain
the original mutations and distinguish a stale handle from -1. Tick does not
release its entity. Existing actual00443f90 query cleanup stops/deletes it.

Dependencies are real linked source: `bh_sound_request/update/stop`, the
`BHScriptTimerActor` table already used by timer callbacks, the actual entity
pool and existing script-lifetime binding/query. The source binding adapter
sets verified fields and installs real methods; it neither supplies a fake VM
callback nor counts as an original constructor. It reuses existing persistent
timer payload+52/lifetime payload+400 context bindings. Context, actor table,
actor/owner, pool and sounds remain borrowed for the entity's lifetime.

`python META/m4/resume400/a/build_check.py` compiles the real translation unit
using pinned SP3 CL and stock VC5 LINK, links
`build/script-attached-sound/bhunter-script-attached-sound-check.exe`, and
passes221 checks. Omitting the owner module fails linking. Real pool-only
streaming resources exercise queued gain, position, zero velocity, unit pitch,
entire0..15 rise/fall, counter-equal spatial freeze, delayed target sampling,
owner precedence, negative nonzero activity, source-invalid request and stale
handle outcomes, signed boundary equality, unrelated payload retention and
actual event9/event20 cleanup through the real deferred delete owner.
No sound tick/Play, game or original code, input capture, automated gameplay,
native UI or network operation occurs. PC24 and native playback remain open.

Integration adds `src/bh_script_attached_sound.c`,
`tests/script_attached_sound.c`, and calls
`int __cdecl bh_script_attached_sound_checkpoint(unsigned long *)`.
Scenario name is `script-attached-sound`. Parent owns combined real/clean
integration and ledger admission.

## Parser/VM boundary findings

00410d90/00410f60/00411080 all call004457c0; that allocator installs the
full00443ff0 query owner. That roughly6KB dispatcher has25 opcodes and a
previously unassigned decoded region004440db..004456e2. Its installed sound
step is now closed, while other effect constructors/ticks, detach/launch,
collision/aperture updates and radar actor lists remain unimplemented.
Therefore no parser helper or allocator is admitted behind a callback stub.
00410590 block instantiation also recognizes VM opcode20 and immediately
invokes a timer for location construction, so its complete closure likewise
requires that full VM; the location-only branch cannot justify admitting it.
00410880 material classification is already admitted in
`src/bh_world_geometry.c` and was not counted again.

`ReviewScript.java` creates analysis metadata using reviewed owner boundaries
and exports instructions; it changes no original reference bytes. Its VM
range is a review envelope including data/alignment and not an admitted owner
boundary. This source prerequisite advances full script construction without
claiming that M4 or a playable source game is complete. M3 remains skipped.
