# Independent whole sound-loader review

0047b950 is an ordinary sound-engine resource loader, engine_candidate with unknown historical authorship. Complete781bytes/261instructions through0047bc5c RET verify against immutable PE bytes and independent catalog counts. It has real VFILE/file allocation/RIFF parse/DirectSound buffer/resource/RNG behavior, not CRT/compiler support or an import/shared tail. Existing0047b950-wave-data-subset-c is WIP and must be explicitly superseded by the whole variant without transferring its old partial evidence.

Raw stack interpretation matters: after38h local bytes and four saved registers, arguments are name atESP+4c, audible range at+50, volume/level at+54, streaming low16 at+58 and interval at+5c. Name is copied from the original name argument after freeing the temporary file image, not a pointer returned by the RIFF reader. Decompiler locals/register naming and its one-argument signature are misleading.

File handling opens rb, seek/tell/rewind, allocates full file length, reads, closes even on allocation failure, parses format/data, creates native DirectSound buffer flags0xe0, requires nonnull buffer/data/nonzero bytes, locks, copies both lock regions, unlocks and frees the temporary image before resource metadata. Success ignores Unlock result in original. Missing source allocation/native errors may use explicit safe cleanup guards; no Play call belongs here.

After resource allocation, GetFrequency success is converted from unsignedDWORD through zero-extended qword to float; failure uses22050.0f bits46ac4400. no_more is computed from the resource's preexisting voice_limit==1 BEFORE current global policy overwrites voice_limit, so recycled payload ordering must remain. Buffer count1, streaming from low16 argument, sample_bytes from first lock region only, old cached state reset over current limit, and original resource index is returned. Loops initialize playing0, volume/frequency/pan caches-100000 and voicesNULL for every current limit slot.

Base volume uses original ln-level scale -1000/log(.5), clamps <=-10000 including unordered to-10000, then rounds to float. time_scale=1.0/interval is stored as float. Exactly one CRT rand call occurs only after successful resource creation/initialization; phase multiplies signed rand by stored float time_scale then double1/32768 and rounds once to float. Caller RNG lifetime must be explicit; no srand/reset per load, no extra draw on failure. PC53 checks cannot claim original PC24 equivalence.

This is static boundary/contract review only. No original/game/audio playback was executed. Source review follows when files exist; admission requires current whole-owner named-target compile/link and source checks with matching real/clean source hashes.

## Final source and fixture review

Reviewed `bh_sound_load.c/.h` and `tests/sound_load.c` against the whole raw body. No concrete implementation bug found in the documented supported domain. The five original arguments, original name identity/lifetime, DSBUFFERDESC size20/flags0xe0, checked native-format forwarding, first-span sample count, unsigned frequency and22050 fallback, old voice-limit/no_more ordering, selective cache reset, ignored Unlock error for publication, exactly one post-allocation shared CRT rand and stored-float phase ordering agree.

Source adds explicit safe handling for bounded name/file/format spans, failed allocation/native creation/Lock, resource allocation rollback and caller PC53; no instruction-match/PC24/full invalid-state equivalence is claimed. PCM16-byte format zero-extension supplies equivalent zero cbSize safely; checked non-PCM format extensions are forwarded to the native API rather than silently decoded/rejected as PCM-only.

The focused fixture verifies actual native buffers, all60 real WAV PCM CRCs/rates, no playing status, allocator failures, shared RNG consumption and recycled old-limit metadata. Native split-lock, GetFrequency failure and Unlock failure are retained statically; healthy host tests do not establish forced driver-error coverage. Root identified the initially mounted archive was not detached by the fixture; implementing agent is correcting it and rerunning before freeze. No other fixture or reentrancy issue identified.

This reviewer did not execute source checks or alter source/ledger. Named-target integration and real/clean validation belong to root's final frozen build. `record_sound_loader.py` remains unrun by reviewer.
