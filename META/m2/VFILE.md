# M2 archive/VFILE implementation contract

Started/completed for this family 2026-10-23. Status: all nine functional C variants admitted into the linked integration harness under the contracts below; no game admission or byte-match claim. M2 as a whole remains open. M1 roles/boundaries: `../m1/REVIEW.md`, `../m1/review-decisions.json`, and the corresponding complete `.asm.txt` bodies under `../m1/review/`.

## Dependency and ownership decisions before coding

- Nine assigned entries remain `engine_candidate`; historical library/game authorship is unknown. This family owns `src/bh_vfile.c` and its checked shared declarations in `src/bh_vfile.h`. No compiler/runtime functions are reconstructed.
- Recover the mount/open/read/seek/tell/close/EOF subfamily first, compile its actual translation unit, link with math and the test drivers into `build/m0/bhunter-integration.exe`, then extend to preload and line reading with separate checks.
- Reuse real CRT malloc/free, FILE/fopen/fclose/fseek/ftell/fread/fgets, case-insensitive lookup and diagnostic printf/fflush. The optional progress callback has a real cdecl-float contract and nullable owning definition. Tests use an explicitly isolated recording callback; production does not replace loading UI with a stub.
- A module-owned singleton holds the observed archive directory/count, backing FILE, access-order/counter, recycle/cache heads, callback and debug flags (`004f95f8..004f9618`). Additional private allocation/active-handle tracking supplies checked teardown and busy-mount rejection; these are lifecycle support, not fabricated original globals.
- Preserve the recovered x86 32-byte VFILE, 256-byte directory and 256-byte cached-name node layouts. Source owns these definitions once. No guessed vtables or code loaded from the EXE.
- Test entry points are separate from the original pinned M0 fixture source. The integration build renames that driver's `main` at compile time and calls it from a new combined checkpoint; historical fixture files/hashes and matching C++ context remain intact.

## Observed behavior and explicit safe extensions

- Lookup precedence is cached handle, loose FILE, then archive if the mode starts with `r`. Loose mode is passed to CRT unchanged; archive reads are raw bytes regardless of text mode. Cached opens return the same wrapper, resetting cursor/EOF; close preserves cached payload.
- Archive open at `0047cdf7..0047cdfd` pushes start, zero, FILE: `fseek(FILE, 0, start)` with an invalid origin for normal entries. Its result is ignored. Preserve the backing-position side effect on the declared VC5 CRT. Reads always seek to the wrapper cursor. Archive tell at `0047d255` instead refreshes the wrapper cursor from shared `ftell` and subtracts its start, so interleaving handles has observable effects.
- Archive/memory seek clamps to start/end and resets EOF, including the original unchanged-position behavior for an unknown origin. Reads consume a partial final element and report the number of complete elements; exact-end reads do not set wrapper EOF until a request exceeds the end. Loose EOF remains CRT bit `0x10`, expressed through `feof` instead of guessing FILE internals.
- Original zero-size division, multiplication/signed-add overflow, unchecked directory allocations, null writes, line read-ahead, and short backing reads are unsafe edge cases. Source returns a diagnosed failure/zero and bounds accesses. These are documented safety deviations, not claims of byte-identical behavior.
- Mount validates little-endian directory count, NUL-terminated nonempty names and every start/end against the complete file/directory bounds before publishing state. The format has no version field; arbitrary wrong-version payloads with a valid directory cannot be identified here. File/offset sizes are limited to signed 32-bit historical FILE positioning.
- Transactional failed mount leaves the prior mount usable. Remount/unmount rejects live archive wrappers, preserves cached payloads, releases replaced directory/access arrays, and exposes a deliberate detach operation corresponding to WinMain's archive FILE close after bootstrap. Full shutdown releases loose handles, wrappers and cached payloads; its pointers are invalid after teardown. Original source-game phase ordering remains unverified and will not be admitted by these isolated checks.
- Archive/memory line reads retain the observed CR/LF pairing and maximum-window cursor rules where valid. **The caller supplies `window+1` bytes**, explicitly stated in the shared header. A full no-newline window consumes all `window` bytes as observed, with an extra NUL written into that additional byte; no character is discarded. End-of-entry reads stop at the entry boundary, terminate actual bytes and set EOF on a short window. A delimiter in the last byte of a window remains omitted from output and consumes that byte; its possible paired delimiter is handled by the next call. Loose mode retains CRT `fgets(window)` behavior and text/binary translation.
- New lifecycle operations are rejected during progress callbacks so a callback cannot free directory/backing state while open is in progress. Callbacks may inspect their rejected error; it does not poison a successful outer open. The module remains serialized and has no concurrent-thread guarantee. Output buffers must not alias private cached payload storage.

## Linked validation and evidence

Synthetic archives and loose files: precedence/modes/case, first-access progress exactly once, shared FILE cursor behavior, seek/clamp/EOF/partial elements, interleaved wrappers, valid and malformed mounts, repeated reload/teardown, preload shared ownership and line endings. Guard bytes and isolated temporary fixtures must catch boundary writes. Real `Bhunter.bin`: validate all 1,229 records and compare all payload bytes through the recovered reader with independent raw FILE reads; cover BSP/PCX/TXT/WAV/LEV/PTH and representative line/preload paths. The normal build must still run without game files; real-asset validation is an explicit option. No extension to the original math image mapper.

The seven-function core checkpoint passed first (`vfile-core/result.json`). Preload/line tests and safety-edge checks were then added and the whole current source translation unit rebuilt and relinked. The final real-asset checkpoint (`vfile/real-archive-result.json`, `vfile/link-map.txt`) passes **20,475 combined checks**, including the original 14 math checks, with zero failures. Every one of the 1,229 real payloads compares equal to independent raw FILE reads: **12,839,409 payload bytes** across 505 BSP, 459 PCX, 203 TXT, 60 WAV, 1 LEV and 1 PTH entries. An additional **1,168,813 bytes** from six representative assets compare equal through preloaded memory. Two real TXT entries also exercise cached and embedded normalized line reads.

Synthetic checks cover all directory truncations from 4 through 259 bytes for a one-record archive, nine invalid count/name/offset variants, zero-entry/zero-payload files, a 247-character valid name, overflow/zero reads, partial element consumption and buffer guards. The shared backing-position quirks, persistent global access counter across mounts, cache precedence/shared cursor, callback-time lifecycle rejection, double-close detection, text/binary loose modes, all three exact diagnostic strings and repeated mount/preload/shutdown cycles pass. Missing math and VFILE modules each fail linking with LNK2001.

`vfile-clean/clean-checkout.json` records a clean Git source export with no original files, no compiler inside the checkout, and a path containing spaces. The normal build passes from a different working directory using only synthetic data. Original M0/M1 evidence is retained separately. The immutable EXE/BIN/MDF/MDS preservation manifest and historical fixture/compiler context were not refreshed.

Reproduce:

```powershell
.\build.cmd
.\build.cmd --archive GAME_DATA/Bhunter.bin
python tools/check_m0_clean.py --evidence-dir META/m2/vfile-clean
python tools/m0.py verify-reference
```

## Admission limits and next dependencies

- `META/function-status.json` records nine separate `func` implementations, real target, source hash, ABI/fixtures and limits. The six new lifecycle/configuration helpers are recorded separately as source support adapters and do not count as recovered original functions. M1 catalog and historical matching variants retain their own evidence.
- This is disassembly-guided functional recovery plus independent data comparison. No original VFILE code was loaded or executed; no binary-backed adapter was added. No complete original-runtime differential or instruction-match claim is made.
- Validation covers the declared x86 VC5/SP3 C + stock VC5 linker harness. There is no modern-toolchain, sanitizer, forced-allocation-failure or deterministic I/O-fault-injection evidence. Teardown paths are implemented and repeated cycles pass; this is not a measured proof of every lifetime/failure path.
- All six asset classes pass through byte access; BSP/PCX/WAV/LEV/PTH payload decoding and script behavior are still unrecovered. Structural directory validation cannot identify an arbitrary incompatible payload format with otherwise valid offsets/names.
- Full source-game cache use after bootstrap and shutdown/thread ordering remain unverified. This family provides an explicit detach lifecycle but does not invent when source gameplay should invoke it.
- M2 still needs the numerical contract/expanded math fixtures, other allocation/container/path/config foundations and bounded payload readers required by the actual slice. Runtime PC24 transitions, CD audio/media and networking requirements remain open.
