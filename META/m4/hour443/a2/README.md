# Whole menu text capture0042bf80

`src/bh_input_text.c` restores the complete890-byte /268-instruction ordinary
menu text-capture entity step0042bf80..0042c2f9. The full raw boundary and its
immutable reference SHA256 are in `owners.json` and `original/`. This is an
application input/menu callback, with no CRT/import/EH/stack helper or shared
tail counted. Historical game/engine attribution remains `engine_candidate`.

The new source uses actual packet application00475870, transitions00475990,
cached mode004756b0, sound request0047bd40, row refresh0042c7d0 and entity release
00477070. It dispatches directly to real profile0042de70, network address
0042eae0 and phone0042ee50 owners. The new profile owner uses the parent's real
BHConfig clone-ownership API; no private allocation node or guessed callback
is supplied here. Adjacent admitted key capture0042be70 is not counted again.

## Exact state and ordering

Source contexts replace the actual global input list/transition state, parent
menu and shared text buffer/counter/limit. The temporary entity stores original
phase+1bc and a separate source-private borrowed context pointer. All owners and
the buffer outlive capture; row callbacks retain that entity/context until the
step returns. Setup/cancellation adapters add no owner count.

Every step first applies the actual cached input packet. Phase0 primes indices
0..254 and changes to1. Phase1 processes each newly-down key, starting with the
generic .6-volume/unit-pitch menu sound. Return sets phase2; Escape sets3. The
same scan continues, so later Escape overrides Return; finalization occurs on
a subsequent step. Backspace emits .7 pitch, and an empty-buffer attempt adds
.5 pitch. Append emits another unit-pitch sound; a full-buffer attempt instead
emits .7 then .5. Leading space is rejected. Sounds queue actual voice state
without playback, and a sound error does not suppress subsequent text effects.

The raw `MapVirtualKeyA(key,2)` return **replaces the scan index** before its
increment. This unusual behavior is retained and tested against the real native
translation API. The append retains the original low-byte conversion. No key
polling, physical input collection, capture, cursor recentering or input
injection occurs. Negative/dead-key indices are rejected before the original
unsafe next lookup; tests establish the host ASCII mapping, not every locale.

After every new key, all ASCII a..z already in the buffer are uppercased. Real
CRT `sprintf("%s_",...)` formats the final17 bytes plus underscore into bounded
display scratch, then the actual row owner refreshes the current selected row.
`format-reference.json` retains the immutable original format bytes00490420.

Phase2 trims trailing spaces, flushes transitions0..254, switches actual cached
input mode0, clears the commit binding **before** calling its actual owner,
then releases the entity. Empty trimmed profile text consequently reaches the
real empty-input callback path. Phase3 performs the same flush/mode/clear, calls
the optional selected row refresh, and releases without committing text. Other
phase values retain the original no-op behavior.

Source preconditions reject invalid bindings, mismatched length/counter,
unterminated buffers or limits that would overflow actual commit records.
Supported maxima are profile15, address63 and phone95 bytes, with an extra NUL
in caller storage. Buffer/owner aliasing or independent release during a row
callback is outside the lifetime contract. Original unsafe buffer/index errors
are explicit limits, not emulated adjacent-memory corruption.

## Validation and integration

`python META/m4/hour443/a2/build_check.py` builds
`build/input-text/bhunter-input-text-check.exe`, which passes243 checks.
The new text owner/test, actual profile owner and actual config TU compile
fresh with pinned VC5 SP3 flags; snapshotted admitted dependency objects supply
the remaining real source closure. Every compiled source/header context and
retained object hash is recorded in `standalone-build.json`. Removing the text
module produces unresolved symbols. No byte-match evidence is inferred from
this functional result; current parent full/clean builds remain admission gates.

Checks exercise all three actual commit callbacks; profile cloning with valid
owned ring/baseline lifetime; held-key priming and index255 exclusion; leading
space, success, backspace, empty/full errors and actual sound pitches; global
uppercase and17-byte scrolling; translated-index skipping; deferred and
overridden completion phases; all-space trimming; cancellation without record
mutation; failure continuation; actual textbox queue draining and native
offscreen page pixel readback. A private300-glyph PCX is created only under the
standalone build working directory and removed afterward. Actual row refresh
bridges delegate to the recovered row implementation; no constant-success
callback substitutes for a dependency.

Parent integration adds `src/bh_input_text.c` and `tests/input_text.c`, alongside
C's `src/bh_menu_profile.c`, then calls:

```c
int __cdecl bh_input_text_checkpoint(unsigned long *checks);
```

The standalone macro is `BH_INPUT_TEXT_STANDALONE`. New evidence is confined to
this directory; no ledger/checkpoint/integration file or admitted TU was edited
by this agent. No game/original code execution, automated gameplay, physical
input/UI capture, playback or networking was performed.
