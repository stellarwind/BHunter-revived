# Complete shared weapon state callbacks

Before implementation, classified `004309b0` and `00431050` as ordinary shared
weapon behavior, not CRT/imports/compiler helpers or shared tails. The verified
ten-weapon table uses these as methods at weapon+12c and +11c respectively.
Historical engine authorship remains unresolved; raw instruction coverage is
not a source count. The real `rand` and sound queue dependencies are reused,
not counted again. No weapon constructor or fake callback table was supplied.

`export_review.py` freshly exports complete 22B/7instruction and
108B/26instruction RET-ended bodies and verifies every byte against immutable
EXE SHA256 `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
`raw-review.json` records their distinct original hashes and bounded roles.
`DefineWeaponCommonBoundary.java` declares only these exact bodies in Ghidra's
analysis metadata; it does not patch the executable. Original code was not run.

Real source is `src/bh_weapon_common.c/.h`. Complete `bh_weapon_delay_tick`
decrements every nonzero signed+114 word with explicit32-bit wrap, including
LONG_MIN. Complete `bh_weapon_ready_reset` zeros only+e8/+f4/+f0/+f8/+fc and
sets+100=-1, preserving ammo, availability, delay and all other allocation
bytes. It then consumes exactly one actual CRT rand and calls the real sound
request owner using loaded resource004cda10, stored float level .9 and pitch
stored float(double .9-rand*double -0.000006103515625). The multiplier at
004892e0 has raw bytes `9a9999999999d9be`; base at004892e8 is
`cdccccccccccec3f`. Thus pitch rises from about.9 to1.1. The original passes
player+0c and player+48 pointers; source preserves live pointers even across
the sound owner's real allocator calls. Sound request's returned handle is
discarded by original; source reports failure after preserving reset/RNG effects.

`BHWeaponCommonState` is the verified weapon+e8..117 subrecord over the actual
source BHEntity allocation. Compile-time offsets pin embedded shared
BHWeaponHudState at+f8 and ammo+10c/delay+114. Unknown words retain offset names.
This is a useful actual state alias for future registry binding, not a full
weapon layout, constructor, ownership registry or recovery of installed firing/
query methods. The source accessor does not allocate/initialize/install methods.

`tests/weapon_common.c` passes **111 checks** in the real-TU linked target
`build/weapon-common/bhunter-weapon-common.exe`; source/object hashes, compiler
commands and exact run output are in `focused-result.json`. Delay checks cover
zero/positive/negative/extreme values and compare whole entity bytes. Reset
checks compare whole allocation mutation masks across16 seeds. Actual source
sound owners take a real DirectSound interface and upload a real PCM buffer,
then queue positioned/moving voices without Play. Independent CRT recurrence
and pitch formula predict both the next random and actual voice frequency.
An isolated allocator delegates malloc/free and mutates player vectors before
voice creation to prove the original live-pointer contract (snapshotting
velocity would fail). Missing-sound tests retain exact reset/random effects.
No gameplay, original code, input/controller loop or sound playback executed.

Full current integration and final ledger admission belong to parent evidence;
focused linked checks alone remain candidate evidence until that admission.
