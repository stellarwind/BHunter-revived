# Debris constructor, physics and actual billboard closure

Three complete owners:00403120 constructor (451bytes/124instructions),
00402d20 physics/lifetime tick (242/81) and004027f0 billboard refresh (58/18).
Boundary/role review established the first routine is a separate debris
constructor, not the installed updater of full explosion00402e50. Its two
actual installed callbacks form a closed small family over existing source
pool, shared CRT rand, sqrt and actual billboard producer. These are ordinary
engine/game effects, conservatively engine_candidate; no CRT/helper/EH/import
or shared tail is assigned or counted. Original PE bytes are immutable.

Constructor count0 clears original opaque attribution word004dfb34 without
reading a point. Each successful entity captures its current value at+1b0,
copies point and lifetime, consumes exactly9 shared random draws and installs
both real callbacks. Frame is first draw masked4; next3 integer components
form a direction. Original sqrt length spills to binary32 BEFORE division.
A retained binary64 common factor computes(.9-r*(-.000006103515625))*speed*
(1/30)/stored_length, then each vector component stores binary32. Width and
height consume separate draws scaled by size/16384; angle uses1/5120 and spin
uses(r-16384)/32768 (positive PE factor00489250). Finishing the entire batch clears attribution; allocator
failure retains it and every already created entity, with no failed-slot rand
draw. An added source count return exposes ownership and is not an EAX claim.

Tick always decrements lifetime and stores float, but its C0 branch releases
on negative/unordered retained result (not the particle family's<=0 contract).
Remaining entities move by OLD velocity. Gravity computes unroundedY as
oldY-(-.010911111111111113), and its product with a float spill contributes
to the new speed. Drag is1-speed*.0025, floored to.5 including unordered.
Retained unroundedY participates in final dampedY multiplication, while its
separate binary32 store is preserved. The angle adds spin last. Refresh
forwards actual position, width/height, angle, frame and unit alpha to real
billboard bucket0 with caller-borrowed source image004df23c.

Explicit source context borrows real pool/queue/image, carries the raw
attribution DWORD and uses private payload+472. Other original state bytes
remain unchanged. No original pointer, fake visual method or guessed vtable
is fabricated. Negative/count>65535 source inputs reject instead of attempting
the original effectively unbounded allocation loop; supported construction
retains original count and lifetime behavior. Caller retains context lifetime.

`python META/m4/hour424/a2/build_check.py` freshly compiles the real new TUs
and all required source dependencies using pinned SP3 CL and stock VC5 LINK,
links `build/script-debris/bhunter-script-debris-check.exe`, passes142 checks
and requires missing-module link failure. Tests verify24 independent seeds,
all9 random draws and every resulting constructor scalar/vector, actual
multi-entity loop/attribution semantics, zero-count,NULL-unused-point, true
initial/partial growth allocation failures, gravity/drag and old-velocity
motion, real billboard payload/linkage, negative/NaN lifetime expiry and
balanced allocator cleanup. Reference normalization explicitly stores each
binary64 step through volatile variables so VC5 cannot fold constant speed30
against its reciprocal timestep and erase the original two rounded operations.

Full explosion00402e50 remains open: it installs00402af0 whose calls include
missing004032f0, plus separate refresh00402710. No partial explosion or guessed
callbacks are admitted. Query00402e20 is already admitted and not recounted.
No game/original code, gameplay automation, UI/input, native playback or network
operation occurs. PC24, signaling-NaN payload and original artwork remain open;
M3 is skipped and M4/playable source game remain incomplete.

Integration adds `src/bh_script_debris.c`, `tests/script_debris.c` and calls
`int __cdecl bh_script_debris_checkpoint(unsigned long *)`; scenario
script-debris. Parent owns complete real/clean integration and ledger admission.
