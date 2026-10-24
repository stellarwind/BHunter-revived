# Original DirectSound Play API trace — 2026-10-24

One idle reference launch `7283583a444c` runs 300 original simulation returns with no child keyboard or gameplay input. It captures actual DirectSound vtable calls/results rather than interpreting sound-start callback returns as playback success. The native probe compiles, links and executes; its new source snapshots belong to this run. The previous selected-feature run remains validated against its own archived snapshots.

## Reviewed boundaries and actual observations

0047bd40 creates/queues sound requests; it does not directly call Play. Original initialization owner 0047b500 and deferred update owner 0047c200 make the actual IDirectSoundBuffer calls at vtable offset 0x30, with GetStatus at offset 0x24. Their complete exports and ownership classifications are in sound-review/. Engine authorship remains unresolved; there is no source implementation assignment.

| Original callsite | Actual request | Observed return |
|---|---|---|
| 0047b5c4 | Primary buffer, reserved 0/0, flags 1 (DSBPLAY_LOOPING), before simulation | S_OK |
| 0047ca09 | Distinct secondary buffer, reserved 0/0, flags 0 (one-shot), callback 299 | S_OK |

The probe reads the actual buffer pointer and vtable method target before each original call, resumes the real method and captures EAX immediately afterward. No API success, sound handle or sample data is fabricated. Primary looping playback alone could be silence and is not sample-playback acceptance. The secondary one-shot request is stronger evidence of the original sound queue reaching its backend, but this trace does not identify its exact WAV payload or verify the mixer/output waveform.

The loop-buffer Play callsite 0047c644 and buffer-status callsite 0047c6f2 were armed but were not reached in this window. Accordingly no DSBSTATUS_PLAYING, playback completion, cursor advancement or full buffer lifetime is claimed. The 300-callback bound was retained; no second launch was made to turn that missing observation into a passing claim. No audible-output or CD playback/notify-loop acceptance follows from S_OK. The separate host MCI CD-device blocker remains recorded.

## Isolation and validation

Existing private SourceDir/profile, pinned local DDrawCompat, explicit 128 MiB original CRT initial-size input and relative cursor adaptations remain in use. Global-volume-write guards are retained and not reached; normal original per-buffer volume behavior is not replaced. The run ends at its observation boundary with restored/verified breakpoint bytes and diagnostic child termination. That termination is not normal game shutdown evidence.

`python tools/check_m3_sound_evidence.py` audits exact callsites/arguments, distinct buffers, native API return values, absence of status evidence/input, source snapshots, private key cleanup, protected Windows file/registry invariants and original EXE/BIN/MDF/MDS hashes without replaying the game. `sound-validation.json` records the narrow result. No normal integration source or source-game admission changed. M3 remains active for actual objective/destruction/retry, fuller sound lifetime/fidelity, network session and remaining behavior evidence.
