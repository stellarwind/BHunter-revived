# Three-buffer render handoff

Complete00471c00,00471cc0 and00471d70 are restored in `src/bh_frame_buffers.c`.
They are ordinary engine callback/buffer owners, not verified library/CRT,
compiler support, import thunks or shared tails. Authorship remains unresolved.
`PRE_REVIEW.md`, `owner.json` and complete raw exports pin all470 bytes and118
instructions against the immutable EXE. `SOURCE_REVIEW.md` independently checks
the actual source variant and records guarded-domain differences.

The actual startup call supplies producer00479140, message helper00473c40,
30Hz and294000bytes (3500 original84-byte command records). The initializer
stores the borrowed callbacks and double1000/fps-1.5 delay (minimum1), then
allocates three buffers with busy0/age3. Acquire waits on the real mutex in5ms
increments, chooses the earliest strictly greatest signed nonnegative age,
marks that slot busy1/age-1 and retains its active pointer. Publication increments
every age with32-bit wrapping, clears active busy and publishes the latest slot.
Its active pointer remains retained after publication.

The source context/lifetime adapter owns a real Win32 mutex and real allocated
buffers. Source guards diagnose no eligible slot rather than treating the
original saved-ECX fallback as a guessed gameplay parameter. Allocation rollback,
wait failure, nested acquire and unmatched publication are explicit guards.
This is a serial-producer contract; the writing flag is not a reservation for
multiple producers. External consumers must stop before disposal.

Fixtures compose the actual draw-queue allocator with acquired frame storage,
submit real billboard commands, publish through queue disposal, invoke the real
input-message helper and cycle all three buffers. A separate actual Win32
thread holds the mutex to exercise timeout/retry and orderly release. Partial
allocation rollback, signed ages, ties and integer wrapping are checked. No game,
original code, worker render loop or external input is executed. The original
producer00479140, consumer/worker00471e00 and wider application/thread lifetime
remain unimplemented; storing callback definitions does not admit those owners.
The current tests bind actual source queue/input callbacks instead.

Parent integration, clean-build snapshots and ledger evidence are retained in
`../frontier-90min/`. No instruction-match claim or PC24 equivalence is made.
