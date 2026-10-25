# Complete horizon color preparation owner 0045d7a0

2026-10-25. Reviewed before implementation in `boundary-review.json`: complete
ordinary `0045d7a0..0045db70` body, 977 bytes /303 instructions, RET ended.
Classification is `engine_candidate`, attribution unknown. It prepares color
lookup tables for actual horizon renderer `0045db80`. No verified third-party
library, CRT/import thunk, compiler EH/unwind helper or shared-tail role was
established. Real verified CRT allocation remains a dependency, not recovered
gameplay code. The parent canonical exporter verifies the complete original;
an independent complete JSON instruction export is retained at
`../scene-callers-review/original/0045d7a0.disasm.json`.

`src/bh_horizon_color.c` contains the full original preparation behavior. Its
only behavioral dependency is actual `bh_texture_tables_init` (`0044e9b0`),
called before every cache test. The source owner borrows `BHTextureContext`,
copies its actual allocator interface for stable release, and owns both lazy
allocations. Init/dispose and diagnostic fill/error fields are explicit source
adapters, not admission of original startup/destruction functions.

Mode exactly1 produces a256x256 byte table; every other mode produces a256x256
unsigned16 table. The two buffers share one full32-bit color key, reproducing
the original cross-mode stale-cache behavior. A selected buffer's existence
plus the shared key alone decides a hit. Neither changed display shifts nor
the other buffer's actual content invalidate it. RGB generation ignores the
top color byte while cache identity includes it.

Every one of256 input cube entries is retained, including252..255 repeating
the first four colors. R/B have six levels and G seven. Rows linearly combine
the constant RGB at row0 with the cube RGB at row255 using positive integer
division by255. Indexed output uses actual existing texture quantization tables
and the original6x7x6 packing. Packed output reads current display shifts on
each miss, masks their counts to the x86 low5 bits, adds channel terms rather
than ORing them, and stores low16 bits after defined unsigned32 wrapping.

Original allocation failure was unchecked. The source null-allocation guard
returns0 and reports `BH_HORIZON_COLOR_MEMORY` without publishing a new key.
This is explicitly a source guard; it does not claim original error behavior.
Context and allocator user lifetimes must outlive the owner and all users of
its borrowed table pointers. Dispose releases both buffers using the copied
real allocator and clears owner state.

`tests/horizon_color.c` passes131350 checks. Independent arithmetic oracles
check all65536 indexed and all65536 packed565 entries. Additional tests cover
the repeated cube tail, both real allocations/releases, original shared-key
staleness, high-byte cache changes, mode0/other non-one modes, stale format
cache, initialization even on a hit, shift-count masking, overlapping packed
channel addition, unsigned32 shifts/low16 wrapping, both allocation failures,
unpublished failure keys and invalid bindings. Test allocation fault injection
is isolated in this test TU; production source uses the actual borrowed texture
allocator and contains no test doubles.

The focused checker `check_standalone.py` compiles the actual source TU,
actual `src/bh_texture_tables.c`, and the test TU with pinned VC5 SP3 flags,
links named `build/horizon-color/horizon_color.exe` with real definitions and
normal CRT, and executes all131350 checks. Exact commands/output are saved in
`standalone.json`. Public entries for parent integration are
`bh_horizon_color_tests()` and `bh_horizon_color_checkpoint(unsigned long*)`;
scenario is `horizon-color`. The header requires the same DX SDK declarations
and Direct3D/DirectDraw0x0500 defines as existing texture modules.

Parent current integration/affected checkpoint and ledger evidence are required
for admission. Full `0045db80` horizon drawing and the two scene render callbacks
remain incomplete; this color owner does not count their behavior. No original
code, game startup or gameplay ran, and no byte-match claim is transferred from
another source variant.
