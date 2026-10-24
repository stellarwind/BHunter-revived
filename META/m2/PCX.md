# M2 bounded PCX data and palette family — 2026-10-24

`src/bh_pcx.c` compiles, links and executes with the existing foundations in `bhunter-integration.exe`. Every one of the 459 real PCX assets decodes through three VFILE backends. Raw indices and palettes are preserved, and the original palette/index transformations are separately available. No texture surface, image-cache entry or graphics object is fabricated.

## Original boundaries, roles and dependencies

The complete bodies were read before implementation. Evidence is in `media/readers/`, `media/pcx-consumer/` and `media/pcx-review.json`. All are engine candidates with unresolved historical engine/library authorship, not verified proprietary code.

| Address | Observed responsibility | Source disposition |
|---|---|---|
| 0044f0a0..0044f1ac | VFILE open, 12-byte header probe, dimensions xmax/ymax+1, final 768-byte palette, seek compressed stream at 128 | Bounded data subset, WIP original owner |
| 0044efa0..0044f095 | Buffered next-index RLE, persistent run count/value, remapping table; C0 skips without consuming a value | Bounded bulk-decoder subset, WIP original entry |
| 0044f1b0..0044f1be | Close the owning global VFILE stream | Not claimed as a recovered standalone entry; new adapter borrows its handle |
| 0044f1c0..0044f4ed | Three palette/index transformation branches | Complete functional variant with explicit buffers and observed identity-map precondition |
| 0046f490..00470371 | Image cache, inline RLE, palette transform, DirectDraw surface conversion and rectangle extraction | WIP original owner; data subset only |

The larger `0044f550` image owner is additional caller evidence, not a new completed implementation. It and `0046f490` initialize identity remapping before the palette helper, as does stream-open `0044f0a0` for the loading-screen caller. The source helper explicitly starts identity; arbitrary pre-mutated mapping tables are outside this variant's contract. CRT allocation/memory functions and the already linked VFILE implementation are reused. Original global decoder buffers, COM handles and cache ownership are not guessed.

## Reader and lifecycle contract

`BHPcx` owns its pixel allocation and inline raw 768-byte RGB palette. Initialize fresh state with a paired allocator or real CRT defaults; successful replacement releases the old allocation only after complete decode. Failure retains prior pixels, dimensions and palette and sets an explicit error. Dispose is repeatable. Input must not overlap the owner's pixels; caller preserves private ownership/allocator state.

All 459 assets have manufacturer 10, version 5, encoding 1, eight bits, one plane, xmin/ymin zero, bytes-per-line equal width, and a final 12 marker plus 768 palette bytes. Width/height are xmax/ymax plus one, as consumed by the original. The bounded reader accepts this explicit dialect, including odd widths. Padded rows, alternate versions/origins/planes/encoding/bit depths are diagnosed rather than silently reinterpreted. The header's other uninterpreted fields are not invented as semantic flags.

Limits are 64 MiB input, 4096 each dimension and 16 MiB pixels, checked before arithmetic/allocation. The entire 128-byte header and final palette span must exist; compressed input is confined to the region between them. Literals and 1..63 runs retain byte values. The observed C0 skip consumes input without an output/value and is bounded by the compressed span. Runs may cross row boundaries, as the original stream allows; none do in the real archive. A missing value, insufficient pixels, overlong final run, wrong palette marker or unused bytes after the completed image is diagnosed. No compressed read can consume the marker/palette. All real streams end exactly at that marker with no zero-run skips or surplus bytes.

This is a safety adaptation of unchecked original streaming, not its original 1024-byte refill ABI. `bh_pcx_read` borrows an open VFILE, reads its remaining payload, leaves success at end, attempts cursor rollback on failure and never closes it. Temporary and final allocations have separate lifetimes; shared FILE/cache use remains serialized by the caller.

## Palette and transparency behavior

`bh_pcx_palette` copies input RGB and initializes identity indices before the observed transform. Output may alias its RGB input. It leaves raw `BHPcx` data unchanged so consumers choose the mode explicitly.

- Mode 0 reserves palette/index zero, maps original zero to the nearest nonzero color using Manhattan RGB distance (first wins ties), and brightens wholly dark nonzero entries just enough for their maximum channel to reach eight.
- Other modes except 2 treat the supplied full packed color word as the key. Every matching nonzero index maps to zero. If zero is not already the key, the last matching slot receives its old color and becomes zero's destination; without a match, nearest-color fallback applies. Dark nonzero entries receive the same lift.
- Mode 2 blackens colors whose three channels are below five, maps black nonzero entries to zero, and remaps a nonblack original zero to the nearest nonzero slot.

Every branch finally clears RGB entry zero. Mapping is a single lookup; it is not recursively normalized. Source retains integer comparisons, duplicate-key overwrite order, thresholds and tie behavior. Pixel format packing, lighting tables, surface pitch and rectangle extraction remain their original owners' later work.

## Executed checks

Independent `tools/check_pcx_evidence.py` parses the pinned archive with struct/RLE and a separate integer palette model. It records exact input/pixel SHA256, CRCs, dimensions and four transform settings per real image. A further 96 seeded data-only palette cases exercise dark, duplicate-key and arbitrary color/mode branches. Default checker mode is read-only; control regeneration requires `--write-controls`.

The source-only PCX scenario passes **2,098 checks**. It exercises every short fixture prefix against a no-access page, version/dialect rejection, malformed marker/RLE/value/lengths, C0 skips, original cross-row behavior, maximum runs, odd widths, input/dimension caps, the exact 4096x4096 pixel cap, raw palette preservation, allocation-failure rollback, repeated replacement/disposal, nonzero borrowed cursors, all temporary/output allocation failures and alias/tie/threshold/duplicate-key palette behavior.

Real PCX scenario: **25,053 checks**, **1,377 decodes**, **24,442,368 pixel bytes** compared to independent RLE controls, entire palettes compared to direct archive bytes, and palette/map/mapped-image CRCs compared in four settings for every backend. No surface object is created. Combined source target: **23 translation units**, **376,611 real-data checks**, **36,131 clean source-only checks**, zero failures. Eleven missing-module controls reject LNK2001, including omission of `bh_pcx.obj`.

Evidence: `media/pcx-controls.json`, `media/pcx-integration-real.json`, `media/pcx-link-map.txt`, `pcx-clean/clean-checkout.json`, `media/pcx-validation.json`. The source ledger separates the complete palette variant, three original WIP subsets and four data/lifecycle adapters. Original references remain immutable; the numerical profile, historical matching context and isolated ten-function math mapper are unchanged. Full M2 completion is determined by the separate requirement-by-requirement exit audit, not by this family alone.
