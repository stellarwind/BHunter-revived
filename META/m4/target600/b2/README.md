# Actor vehicle lifetime 004397e0

Whole game cleanup owner212bytes/56instructions,004397e0..004398b4. Fresh raw Ghidra instruction bytes equal immutable PE reference. One distinct finalRET; no shared tail/compiler/library count. Ledger checked before implementation.

bh_actor_vehicle_lifetime.c/.h bind actual source entity pool, existing room actor heads, sound queue and explicit global actor head/count. Preserves sound-stop then event17broadcast then independent room/global unlink then decrement/release. Actual existing script actor query callbacks consume removal broadcasts and clear watched actor references. Borrowed owners remain alive; callbacks must preserve list topology during this synchronous removal. Source checks initial pool/list/room ownership before side effects. Payload retained on free, as original.

20checks pass in real linked target including real streaming sound stop/generation invalidation, event17watched target and killer clearing, differing global/room head order, middle/tail/head removal and repeated release rejection. Omission link control fails. No original game/code, native playback, gameplay automation or capture executed. Constructor/attack/query/tick family remains open. See owners.json and standalone-build.json for ABI, fingerprints, limits and exact commands.
