# Independent source/raw follow-up

Reviewed complete dependency decompilations/raw boundaries and current `bh_present_begin.c`, `bh_capture.c`, `bh_display_shutdown.c` plus worker software-context glue.

004508a0 is already admitted `bh_texture_draw_flush` in `src/bh_texture_draw.c`. Capture calls it for every image; it is not a missing native-format derivation helper. Original0045bc40 reads previously configured pixel-byte/mask/shift globals after Lock, rather than deriving them from DDSURFACEDESC. Explicit capture-format bindings therefore retain the original dependency without requiring a new format helper.

Reported one concrete C-language arithmetic mismatch: `++textures->frame` in native capture can overflow signed long; existing owners preserve original32-bit wrap using an unsigned increment and cast. Implementer was notified. The source added native API failure counters but originally ignores EndScene/BeginScene return for its final success result; any changed error propagation is an explicit diagnostic API choice, not original equivalence evidence.

Native capture validates pitch but initially does not compare right/bottom with actual Lock descriptor dimensions. Implementer was advised to guard this before reading the buffer. Original has no such guard either; faithful valid bindings remain a contract. Source negative-pitch rejection is a deliberate supported-domain restriction. No original/native Lock error-path success was inferred from healthy offscreen checks.

Begin's restore order, texture invalidation, native BeginScene, software color/auxiliary lock order, failed auxiliary unlock and success state agree with original behavior. Nine source texture classes match the established context contract. Display shutdown releases viewport/device/auxiliary in order, clears references, posts WM_CLOSE, closes/clears log and performs both conditional remove calls; no dropped owner found. Compiler/CRT empty callback0047d680 is intentionally excluded.

Worker software glue copies successful begin mappings into the renderer then restores renderer-changed mappings to fade/present before capture/end and synchronizes locked state afterward. This correctly reconnects split source contexts for ordinary successful frame execution. A separate possible persistent clip-bound synchronization question was sent to the worker implementer: capture resets viewport globals but display.left/top are copied fields; the source must synchronize these if subsequent clear bounds share the original globals.

No source or ledger edits by this reviewer. This follow-up is static review, not build or execution evidence.

Implementer follow-up: signed increment changed to unsigned-wrap convention; native Lock crop now checks integer-truncated right/bottom against actual descriptor width/height and unlocks on rejected bounds. Added source checks cover LONG_MAX wrap, width/height rejection and valid subsequent Lock/write. This reviewer has not rerun those tests; linked evidence remains with implementing agent/root.

## Whole frame service follow-up

Reviewed `bh_frame_service.c/.h` and current focused fixture against all47 raw00479140 instructions. Normal collect/counter/acquire/begin/step/refresh/sound/packet/publish ordering agrees. Signed nonzero stop gate and acknowledged stop<=1 cleanup branch agree, including negative values and pre-cleanup increment. Slot metadata is copied before mutex-backed publication, while the canonical borrowing descriptor remains separate from each worker descriptor.

Corrected2026-10-25: raw004791c9 loads current list004f5500 after simulation/sound, adds0x2010 and passes that PACKET first, with destination004f5518 second, to original00475870. The source API reverses the argument order to(state,packet); existing C and fixtures already do that correctly. Source reads both borrowed fields late rather than caching before callbacks. Callers replacing the current list must update its packet binding. The prior destination/global-packet labels in this review were reversed; raw proof is retained in `../../loading-display/next-review/PACKET_ARGUMENT_CORRECTION.md`. Original global allocation/maximum live indexed span still requires its own construction evidence.

No concrete service implementation bug found in the supported binding contract. Recommended one focused discriminator: an actual source entity callback changes the service packet and destination between acquisition and packet application, then assert only the new destination receives new packet data. Existing no-op entity fixtures do not distinguish this lifetime requirement. Test additions belong to the implementing agent; reviewer made no source changes.

## Registration pair follow-up

Whole00471df0 stores the supplied callback identity and returns; whole00479120 installs00479090 through that setter then returns1. Source `bh_frame_callback_set` and `bh_frame_callback_start` preserve both behaviors with a documented explicit renderer-context ABI and NULL-context guard. The registered actual source consumer is invoked by the worker rather than bypassing registration. No fake callback or no-op replacement was introduced. Registration fixture checks identity/NULL storage; actual consumer execution belongs to the frame-worker fixture, so both scenarios are attached to both owner records. Static source review found no concrete pair bug. Raw PE verification covers complete29 bytes/eight instructions across the pair, independently checked with the original inventory.

The service fixture now includes an isolated refresh observer that swaps packet/destination late, then checks the new state receives the new packet while old state remains untouched. This is discriminating isolated test instrumentation and not a restored gameplay callback claim.
