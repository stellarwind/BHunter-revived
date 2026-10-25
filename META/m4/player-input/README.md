# Player input initialization and state contracts

Seven complete original bodies have functional source in `src/bh_player_input.c`:

| Original | Source behavior | Role |
|---|---|---|
| 00421740–00421bba | 33 player bindings from the current profile plus four fixed keys | game |
| 00475640–004756a3 | Allocate and prepend a selectively initialized binding list | engine candidate |
| 004756e0–00475766 | Four-key axis binding | engine candidate |
| 00475770–004757db | Single-key binding | engine candidate |
| 00475810–00475862 | Kind-5 string binding | engine candidate |
| 00475870–0047596f | Compact input packet application | engine candidate |
| 00475990–004759bb | Signed key-state counter transition | engine candidate |

The engine attribution is unresolved. Their real entry boundaries, callers,
binding layouts, packet formats and independent returns distinguish them from
CRT, compiler unwind helpers, imports and shared tails. `review.json` retains
the classification, complete original exports and hashes. Every instruction in
each exported body was compared with the pinned immutable EXE. No byte-match
claim is made for the reconstructed source.

The allocation call at 0047dae0 is a 20-byte CRT `__nh_malloc` wrapper with the
CRT new-mode global. It is excluded from recovered gameplay functions. Source
uses real CRT `malloc/free`, or a caller-supplied paired allocator. Original
new-handler global behavior is not reproduced by this ownership adapter.

`BHPlayerInputOwner.first` owns the former global list head at 004ef2fc.
`BHPlayerInputList` is exactly 0x2130 bytes on x86, including 256 binding records
of 0x20 bytes and opaque state bytes. Construction clears only each binding's
kind, count and state bytes +2010/+2011, alongside linkage and flags. It retains
all other allocation bytes. Disposal is a separately named source adapter.
The current-player +188 list and current profile are explicit borrowed inputs.
Profile byte offsets through +47 are preserved, using the existing 76-byte
`BHInputProfile` layout. Empty labels replace the verified zero byte at 004dfb5c.

Axis argument order is intentional: the first supplied key goes to record +6,
the second to +4, followed by +8/+a. Single-key and string rebinding retain the
other key words. All names stop after 19 characters and retain bytes following
their terminator. Setup retains unused slots 29–31 and existing higher count.

The packet decoder uses header byte 1 as an unsigned record count and ignores
byte 0. Tags 0/1 assign 1/0 to batches of byte indices; each such record occupies
a multiple of four bytes. Tags 2/3/4 copy four raw bytes into one state slot.
Unknown tags consume one byte without changing state. A separate checked
adapter validates the entire declared packet before application; truncation
rejects without partial writes. The raw decoder retains its valid-input
precondition. State transitions use unsigned arithmetic to preserve original
32-bit wrap while applying the original signed-positive predicate.

Run `python META/m4/player-input/build_evidence.py` for the named standalone
source-only target `build/player-input/player-input.exe`. The retained result
contains 77 passing checks: whole-list selective writes, real allocation and
failure ownership, all 33 setup mappings, unsigned profile values, names and
rebindings, all packet tags, every truncation of a mixed packet, padding lengths,
unknown tags, count 255, and signed counter extremes. Actual translation units
compile using the pinned VC5 SP3 compiler, link with the working VC5 linker and
real CRT definitions, then execute. No original code or game is executed.

004756b0 still requires unrecovered device poller 00474020. Neither polling,
DirectInput device lifecycle, nor complete player/scene construction is claimed
by this family. Main integration and admission evidence are recorded by the
owning restoration block after adding this checkpoint to the real harness.
