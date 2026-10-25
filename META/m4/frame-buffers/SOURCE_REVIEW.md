# Independent narrow source review

2026-10-25. Reviewed `src/bh_frame_buffers.c/.h` and
`tests/frame_buffers.c` against all three pinned original instruction bodies,
their constants and the actual004798bf startup call. This review made no source
changes, ran no game/original code, and did not execute the test target.
All three complete body SHA256 values were independently checked against
immutable `GAME_DATA/BHUNTER.EXE`:183,168 and119 bytes respectively.

**No material mismatch found within the stated finite-positive-rate,
single-producer, correctly owned mutex/slot lifecycle contract.**

## Checked behavior

- Initialization retains binary64 `1000.0/fps - 1.5` and lower clamp1.0. The
  original loads all three constants as actual doubles and compares the
  retained result after FST-double. Under the stated PC53 profile, the source
  double expression/store/comparison has the same precision contract. Startup's
  double30.0 and294000-byte allocation size are correctly decoded. Each of three
  slots gets real allocation, busy0 and signed age3.
- Acquisition uses a5ms native mutex wait and retries exactly WAIT_TIMEOUT.
  Its signed best age starts at-1; only busy0 slots with strictly greater age
  replace it. Equal ages retain the earliest slot. Negative ages at/below-1 are
  ineligible until they age back into the signed-positive domain. Selection
  publishes active, marks busy1/age-1 while holding the mutex, releases it, then
  returns the selected buffer storage.
- The original no-eligible path reloads saved incoming ECX into ESI; it is not
  evidence for a semantic fastcall argument or an alternate valid buffer. The
  source explicitly rejects that path instead of inventing such a binding.
- Publication increments every age with defined32-bit unsigned wrap and
  bit-preserving signed storage, including INT_MAX→INT_MIN and active-1→0.
  It clears active busy, copies active to latest before ReleaseMutex and leaves
  active unchanged. Stable0..2 source indices stand for the original stable
  three-slot descriptor pointers; neither active nor latest points to a
  temporary stack/local allocation. Actual buffer ownership lasts until dispose.
- Accepting WAIT_ABANDONED_0 retains a valid mutex-acquired path. WAIT_FAILED and
  unexpected wait results are explicitly guarded, while the original proceeds
  after every non-timeout result. Release failure is surfaced. These added
  diagnostics do not fabricate successful acquisition/publication.

## Source guards and scope

Fresh binding/dispose-before-rebind, finite positive fps/nonzero buffer size,
present typed callbacks, allocation-prefix rollback, duplicate-start rejection,
one outstanding writer, valid active index, no eligible slot and wait/release
failure handling are explicit source ownership/binding guards. They should not
be represented as tests of the original's unchecked failure behavior. The
rollback correctly releases only newly allocated prefixes and publishes no
buffers/callbacks on failure. Dispose refuses an outstanding writer and frees
the owned buffers/mutex only when the caller has stopped consumers.

The owning contract is **one producer**. The `writing` precheck occurs outside
the mutex and is a nested/unmatched-use guard, not a multi-producer reservation
protocol. Two concurrent producer calls must not be advertised as supported;
the original single global active descriptor has the same architectural
restriction. Future worker/consumer owners still need their actual busy-slot
and latest-pointer synchronization/lifetime behavior. These three functions
and the source binding adapter do not implement a worker thread or complete
rendering startup.

## Test review

The tests use an actual Win32 mutex, real allocation and real recovered draw
queue storage/producer behavior. The bridge's release publishes its borrowed
frame lease rather than freeing slot memory. Repeated round-robin/tie behavior,
signed-age selection, wrap, active/latest retention and nested/dispose guards
are explicitly observed. A real contention thread produces actual timeout
retries; failed allocation prefixes exercise rollback. The supplied production
and message callbacks have actual source behavior and are clearly test
bindings, not substitutes for the original registered game callback bodies.

The fixtures do not establish a restored worker, simultaneous frame consumption,
general scheduler timing fidelity or non-PC53 retained-comparison behavior.
Those limits are consistent with the implementation's stated admission scope.

Reviewed snapshot hashes:

| File | SHA256 |
|---|---|
|src/bh_frame_buffers.c|9252358b8af6ff298fb3867af3cd68a51bb805fdc8832baf57bbffd999e1fc64|
|src/bh_frame_buffers.h|7deb7aa1ebf60a27392c21f1efc76a10102d90722aa25da43ef8d5c16658ab82|
|tests/frame_buffers.c|0497ed291990fc143d221a1c989c9ebc459bc7f3ff89b0bfeffe4e88ac51faa9|
