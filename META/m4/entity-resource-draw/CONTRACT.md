# Entity resource draw method: 004413b0

Classification: game-owned entity draw method, confidence high. The original
body spans `004413b0-004414c3`, 276 bytes and 81 decoded instructions; its
SHA256 is `f5ee4ae4f37a8582f9e1de883b3e661c552f24273ca803d9b216faa5aec48e77`.
`original/004413b0.tsv` and `.c.txt` are copied from the reviewed Ghidra
export in `META/m4/queued-resource/original/` for this implementation.

`00441a00` acquires the entity and assigns this method at entity `+e8` only
when resource type 9 and variant 0 are selected. `+e8` is inside the entity
payload. It is **not** the generic `+c8` refresh callback; that constructor
assigns verified `0047d680` (RET) there. `00409f40` calls `+e8` through child
entity lists at `0040a087` and `0040a0af`. Constructor and child-list traversal remain separate unfinished work. Parent
integration now supplies owned resource lookup and the recovered billboard
consumer; see `META/m4/queued-resource/validation.json`.

The callback borrows a live entity and an explicit queue/glow image binding.
Original entity offsets are position `+0c`, mutable draw angles `+3c`, phase
`+fc`, and resource handle `+114`. Source accesses the established `BHEntity`
layout without adding guessed shared fields. It stores the first angles and
submits a resource to bucket 3 with `0.35f` modulation, stores replacement
angles and submits a second resource to bucket 3, then appends a billboard to
bucket 3 with the borrowed glow image. The two model poses use stored binary32
positions `x-0.05` and `(stored x-0.05)-(-0.1)`; y and z are unchanged. Both
model commands copy the angle values at the instant of submission. The
entity retains the second angle pose. The billboard uses the original entity
position, equal size values and alpha derived from one retained sine result.

All three appends are attempted even after queue saturation; successful earlier
records remain linked. The source return value reports complete, partial, or
invalid binding to its caller, whereas the original callback returns void.
Floating inputs are not rejected or sanitized. Native `sin` under the pinned
VC5 PC53 profile is exercised; exact 24-bit x87 behavior and nonfinite edge
cases are not validated. Resource and image handles are borrowed identifiers,
not fabricated original pointers. No original executable code is loaded.

Focused linked target: `build/m4/entity-resource-draw/entity-resource-draw-test.exe`.
`build_focused.py` compiles the real translation unit and real queue producers,
links the source modules, runs 31 checks, and confirms removing the owner
produces unresolved-symbol link failure. `focused-build.json` records the
commands and source hashes. The parent integration owner handles the full
`bhunter-integration.exe` build and ledger admission.
