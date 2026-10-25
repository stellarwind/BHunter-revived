# Complete0047b950 sound loader contract

Read-only support for the source owner implemented by the worker. Complete
body `0047b950..0047bc5c`:781 bytes/261 instructions, raw-PE verified in
`../window-dispatch/next-review/0047b950.disasm.json`. The ledger's PCM subset
is WIP, not a complete implementation. This ordinary engine resource owner is
not CRT/import/compiler support/shared tail; historical attribution is unknown.
No original code/game/native input/playback executes in this review.

## Five real arguments; no option/name parser

Ghidra infers only the first argument and then loses stack/register identities.
The actual cdecl signature is `(name, float audible_range, float level,
unsigned short streaming, float denominator)`; the streaming argument occupies
a complete4-byte stack slot, with only its low16 bits used. After local/saved
register space the offsets are `+4c,+50,+54,+58,+5c` respectively.
`startup-callers.json` independently pins all15 startup sites, five immediate
PUSHes each, actual filename strings and `ADD ESP,0x14` context. Example:
`sound\\beepbeep.wav`,4.0,1.0,1,8.0.

`0047bb01` loads the original filename from `ESP+4c`; subsequent SCAS/REP
instructions copy that exact NUL-terminated string to resource+0. There is no
option/name parser or cache search in the whole function. A source name-length
guard is needed for the64-byte destination; do not infer parser behavior from
the corrupted decompiler's aliases.

## File and native buffer ownership

Requires nonNULL actual DirectSound device. Open VFILE rb, seek end/tell,
rewind, allocate complete file payload, read once, close VFILE. The original
ignores read/scanner return values; bounded source validation must document
its safety guards. `bh_riff_scan(bytes,size,&format,&samples,&sample_bytes)`
is the complete admitted scanner. Original scanner output pointers feed a
20-byte DSBUFFERDESC: flags0xe0, sample byte count, borrowed format pointer.
No Play occurs in this loader.

CreateSoundBuffer output must be nonNULL, sample pointer/count nonzero; Lock
offset0/count=RIFF data length/flags0. Copy first span then second span from
samples+first span length. Unlock both spans. On a Lock/precondition failure,
Release buffer. Free the complete file buffer before allocating the resource.
Original Unlock HRESULT is ignored and a successful uploaded buffer proceeds;
if source reports failure separately, preserve and describe the lifetime choice.

Important quirk: global004f55f4 is assigned **first Lock span length**, and
resource.sample_bytes(+5c) receives that value, even when a second span exists.
Do not silently substitute total sample length. Tests at offset0 normally see
one span, so full split behavior also needs static evidence or a legitimate
native split fixture; no fabricated DirectSound interface.

Required direct source definitions are already linked: resource allocation
0047b330, RIFF0047b830 and VFILE open/close/seek/read/tell. malloc/free/rand are
reused CRT. All COM methods use real interfaces. No new original callback is
required. Resource allocation failure is an original unchecked condition;
source must release its newly owned buffer when rejecting that condition.

## Metadata, RNG and recycled records

Resource layout is already asserted by `bh_sound_queue.c` (size0xac):

| Offset | Exact mutation |
|---|---|
|+00|original name including NUL |
|+40|uploaded actual buffer pointer |
|+4c|buffer_count=1 |
|+4e|no_more=1 iff **old** resource.voice_limit(+54)==1; otherwise0 |
|+50|streaming=low16 argument4 |
|+52|existing resource index retained and returned zero-extended |
|+54|replace voice_limit with current global policy only after old-value comparison |
|+5c|first Lock span byte count |
|+84|actual GetFrequency DWORD converted unsigned through64-bit FILD to float; failure uses22050 bits0x46ac4400 |
|+88|stored binary32 attenuation described below |
|+8c|stored binary32 random initial phase |
|+90|stored binary32 reciprocal denominator |
|+94|argument2 float audible_range transported directly |
|+98|stream_head=NULL |

For i strictly less than the newly stored voice_limit: playing(+56+2i)=0;
cached volume/frequency/pan(+60/+6c/+78+4i)=-100000; voices(+9c+4i)=NULL.
Unused recycled slots and extra buffer pointers are not overwritten by this
owner. Existing source allocator zeroes fresh records but retains recycled
payload; the old-limit comparison must therefore remain explicit.

The x87 stack computes `(-1000 * ln(level)) / ln(0.5)`, then retains it only
when strictly greater than-10000; equal, less and unordered choose-10000.
`constants.json` pins all five raw doubles:48a398=-1000,
48a3a0=0.5,48a3a8=-10000,48a3b0=1,48a3b8=1/32768.
Both natural logs are FLDLN2/FYL2X sequences; base-volume has no intermediate
binary32 store before resource+88. The reciprocal is separately stored
binary32 at+90 before phase calculation. Exactly one CRT rand call occurs
after successful native upload/resource allocation and metadata setup:
`phase = float((rand() * stored_time_scale) * (1.0/32768.0))`.
Do not use32767 as divisor or add an intermediate float round of the product.
Finite moderate PC53 functional equivalence may be checked with analytic
fixtures and stored-float tolerance; PC24/universal byte identity is unclaimed.

## Existing viable validation fixtures

`tests/sound_queue.c:native_cases` creates real DirectSound, sets NORMAL
cooperative level, binds the actual source queue and reads its secondary buffer
with Lock/memcmp/Unlock without Play. Its allocator failure controls prove
new buffer cleanup. `tests/wave_manifest.h` independently pins all22 real WAV
archive spans, PCM metadata and sample CRCs; `tests/wave.c` has odd unknown
chunk/padding, loose/embedded/preloaded ownership and actual-file fixtures.
The new loader can use those patterns while retaining the originals unchanged.
Source-only assertions should cover exact name, all15 real caller arguments,
1/2/3 voice policies, old-limit recycling, deterministic rand consumption and
all per-resource initialized fields. No audible playback is needed.
