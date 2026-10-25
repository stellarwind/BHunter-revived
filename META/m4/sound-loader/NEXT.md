# Next bounded resource family

After admission of the frozen whole sound loader, do not reassign `0047b950`
from its stale PCM-subset WIP record. **Still no playable source game.** Full
WinMain and `00479200` bootstrap remain incomplete.

Recommend the coherent loading-image family: **`00449e50` image loading and
`0044a160` three-image initialization**, with the small owning PCX-close helper
`0044f1b0` explicitly reconciled. This advances real first-slice startup asset
ownership without implementing the large loading-screen renderer.

| Owner | Complete verified boundary | Status/role |
|---|---|---|
|`00449e50`|`00449e50..0044a150`,769 bytes/267 instructions|missing engine image owner; unknown historical provenance |
|`0044a160`|`0044a160..0044a211`,178/42|missing game-specific three-image initialization |
|`0044f1b0`|`0044f1b0..0044f1be`,15/5|missing-ledger PCX stream-close wrapper; existing real source close operation available |

Pre-decompilation role/direct-callee/ledger review found no CRT/compiler/import
or shared-tail bodies among these. `next-review/` retains complete raw-PE
verified disassembly, fresh pseudocode, hashes and ledger matches. No native
input, audio, original/game execution or source implementation occurred.

Closure is bounded. `0044f0a0`=`bh_pcx_reader_open`, `0044efa0`=
`bh_pcx_reader_next` and `0044f1c0`=`bh_pcx_palette` are already admitted.
Older WIP reader records coexist; do not count them as missing. `0044f1b0`
only closes the owning VFILE, whose real definition is admitted; the existing
`bh_pcx_reader_close` already performs that operation with explicit safe
lifetime clearing. Reconcile it as reuse/functional ownership with separate
evidence rather than writing a redundant stub. malloc is reused CRT and
GetTickCount is a system import, neither a game recovery assignment.

The image owner requires an allocated destination arena, display width/depth
and established display bit shifts. At display width640 it consumes640 pixels
per source row. Otherwise it consumes every source pixel but stores the first
of each horizontal pair and skips alternate rows, producing320-wide output.
It initializes PCX palette mode0, then builds native packed-color and quarter
brightness lookup tables, or the exact indexed8 quantization tables. Preserve
all native/indexed and full/half branches; no rasterizer or DirectDraw device is
needed to validate these memory outputs. Source span/dimension guards must
reject insufficient arena space and truncated streams explicitly.

The initialization owner allocates **0x12c000 bytes**, loads MIDAS at0x96000,
copies its256-byte indexed/1024-byte native lookup tables to retained slots,
loads IEL at0xe1000 and copies its tables, then BAD at0. It records
GetTickCount()+5000 with32-bit wrap, state2 and zero progress. It does not
check original allocation/image failures, so source-safe ownership/error
behavior needs a stated contract rather than invented success.

Real archive fixtures are already independently pinned in
`tests/pcx_manifest.h`: BAD640x960 (614400 pixels), MIDAS640x480 (307200),
IEL640x480 (307200). These exactly fill the full-size arena at the original
offsets. Linked source checks can compare both full/half output, all lookup
tables, retained palette copies, allocator/stream failures and actual arena
lifetimes using those assets and synthetic display formats. No screenshots,
gameplay or broad reference replay is required.

Separate known owner `0047b500` DirectSound startup still includes real primary
buffer Play looping; the completed WAV loader does not admit it. Loading
renderer `0044aac0`, force-feedback construction, DirectPlay provider startup
and large world/resource bootstrap also remain separate unfinished owners.
