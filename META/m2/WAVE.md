# M2 bounded WAV data reader — 2026-10-24

`src/bh_wave.c` now compiles, links and executes in `bhunter-integration.exe` with the other foundations. This completes the bounded WAV data requirement for the current archive dialect. It does not construct DirectSound buffers, restore sound-resource playback, apply cues or replace CD audio. Engine/library authorship remains unknown.

## Reviewed boundary and ownership

Complete read-only Ghidra exports are retained in `media/readers/0047b830.{asm,c}.txt` and `media/original/0047b950.{asm,c}.txt`. The memory scanner at `0047b830..0047b91f` checks RIFF/WAVE, walks padded chunks, requires at least 14 fmt bytes and supplies requested fmt/data pointers and data size. It has no allocation/import dependencies. Its caller `0047b950` loads a VFILE payload, calls that scanner, creates/locks/copies/unlocks a DirectSound buffer and populates a sound-resource pool with playback parameters. The decompiler's overlapping stack locals and inferred unaffiliated registers are not adopted as source layouts. The complete sound owner is unrecovered.

The length-bearing source scanner and owning PCM decoder are deliberate data adapters. Original-address records stay **WIP**, while five support APIs are admitted as linked adapters. The original scanner's optional-output ABI and early-return behavior are not claimed recovered wholesale. `media/wave-review.json` also records PCX dependency reconnaissance; that is not PCX implementation admission.

`BHWaveView` borrows fmt/sample spans from the input. `BHWave` owns copied sample bytes and explicit metadata through a paired allocator (real CRT by default). Initialize a fresh owner, replace only after successful validation/allocation, and dispose before reinitializing. Rejection leaves previous samples and metadata intact, updating only the error. Input must not alias owned samples. No fake sound handles, callbacks or COM vtables are supplied.

## Format and bounds contract

The scanner requires little-endian `RIFF` / `WAVE`, an exact supplied RIFF extent and complete padded chunk headers/payloads. It retains the first fmt and first data chunks, including an empty data chunk; skips other chunks structurally; and validates the entire container even after finding both outputs. Unknown metadata bytes are not treated as PCM. Missing outputs, malformed lengths, partial headers/payloads/padding and foreign container dialects are diagnosed. Pad byte values are unrestricted.

These checks intentionally strengthen the original unbounded scanner. Optional-output early exits, its zero-sized-data/order quirks, acceptance of bytes outside RIFF and successful return before malformed trailing chunks are not emulated. Maximum input is 64 MiB, checked before dereference; chunk lengths are compared by subtraction before padded addition. The borrowed scanner accepts the observed original 14-byte fmt minimum, but PCM decoding requires the actual 16-byte PCM fields.

The owning decoder accepts PCM tag 1, one or two channels, 8 or 16 bits, and rates 1..192000. Block alignment must equal channels times bytes/sample, byte rate must equal rate times block alignment, and sample data must contain whole frames. Fmt may be the canonical 16 bytes or a bounded WAVEFORMATEX payload whose cbSize accounts for the remaining extension. Non-PCM, extensible, unsupported channel/width/rate and inconsistent metadata are rejected explicitly. This is the documented supported profile, not a claim of every codec accepted by original DirectSound.

Sample bits, order and interleaving are copied unchanged; there is no normalization, signedness conversion or resampling. Empty PCM retains its metadata and owns no allocation. The VFILE adapter reads the remaining payload from the current cursor, owns temporary and final buffers separately, leaves success at end, attempts cursor restoration on failure, and never closes its borrowed handle. Caller serializes shared backing/cache access and owns handle lifetime.

## Evidence and limits

Independent Python struct parsing (`tools/check_wave_evidence.py`) verifies the pinned archive and produces data-only controls. Default mode is read-only; `--write-controls` explicitly regenerates them. All 60 files have canonical PCM fmt: mono, 11025/22050 Hz, 8/16 bits. Their 1,920,771 sample bytes are accompanied by 74 LIST and 16 cue chunks. Exact asset and PCM SHA256, sample CRCs, chunk spans and metadata are in `media/wave-controls.json`; the C tests use `tests/wave_manifest.h`.

The source-only reader scenario passes **226 checks**: protected-page inputs for every truncated fixture prefix, invalid/overflowing extents, odd padding, missing/duplicate/reordered chunks, malformed trailing chunks after both outputs, 14-byte fmt versus PCM fields, codec/rate/frame/extension rejection, exact unsigned 8-bit/signed 16-bit sample bytes, stereo interleaving, empty data, allocator guards/failures, transactional reload/dispose, nonzero borrowed cursor and rollback of both temporary/output allocation failures.

All 60 real files pass through embedded, preloaded and loose VFILE backends: **180 decodes**, **5,762,313 sample bytes compared**. Each output matches direct raw archive bytes and independent PCM CRC/metadata controls. No playback object is created. WAV scenario: **1,791 checks**, zero failures. Combined target: **21 real translation units**, **351,558 real-data checks**, **34,033 clean source-only checks**, zero failures. Ten missing-module controls reject with LNK2001, including omission of `bh_wave.obj`.

Evidence: `media/wave-integration-real.json`, `media/wave-link-map.txt`, `wave-clean/clean-checkout.json`, `media/wave-validation.json`. Original EXE/BIN/MDF/MDS are unchanged, historical VC5 math comparison context is untouched, and no original code mapper is added or expanded. M2 remains active for the PCX reader and final gate audit. Original playback behavior, sound-resource lifetime, CD music, DirectPlay and runtime numerical scenarios remain explicit later requirements.
