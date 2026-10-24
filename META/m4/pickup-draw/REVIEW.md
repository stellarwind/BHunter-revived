# Pickup drawing dependency family

Reviewed 2026-10-24 from the immutable original disassembly/decompilation, with
the pinned `Bhunter` Ghidra project and saved pickup exports. No original code
was mapped, executed, patched or copied into the source target.

| Address | Boundary and role | Evidence | Source owner |
|---|---|---|---|
| `004412f0` (`004412f0`–`004413a2`, 179 bytes) | Game pickup default draw, high confidence | Writes pickup pose at `+3c`, calls model and glow producers, invokes numeric label only when `+110` is nonzero | `bh_pickup_draw` |
| `00440c40` (`00440c40`–`004412ee`, 1711 bytes) | Game pickup quantity/unit label, high confidence | Reads type `+100`, world position `+0c`, two projected vertical offsets, eight decimal quotient slots, emits atlas UV rectangles | `bh_pickup_label` |
| `004558d0` (`004558d0`–`0045595a`, 139 bytes) | Renderer/engine candidate, high confidence for behavior, unknown provenance | Transforms world point through current projected matrix; rejects below near depth; divides xy and focal by depth | `bh_pickup_project` |
| `00478f60` (`00478f60`–`00479065`, 262 bytes) | Renderer/engine candidate, high confidence for behavior, unknown provenance | Nine bucket insertion with original 0x54 record and rectangle callback; writes fields at `+04`…`+34` | `bh_pickup_rect_queue` |
| `004776c0` | Renderer/engine candidate, high confidence for behavior, unknown provenance | Reads rectangle payload, calls `00454fc0`, returns command's current `+50` next link | `bh_pickup_rect_execute` |
| `00454fc0` (1103 bytes) | Renderer/engine candidate, native path, unknown provenance | Builds rotated screen quad, flips UV, clips, scales grayscale, calls primitive submit | `bh_pickup_rect_draw` partial native adapter; full original owner remains WIP |

All six boundaries are executable behavior, not CRT, compiler support, import
thunks or shared tails. The four renderer routines are kept `engine_candidate`
until provenance is established. The native rectangle path composes the
already recovered `00453300` screen clipper with the explicit hardware branch
adapter for `0046e390`. Original software rasterization remains unfinished;
attempting it through the adapter fails before queue publication. Thus
`00454fc0` is not counted as a fully recovered original function. The
`004776c0` queue consumer's rendering contract inherits that backend limit.

`BHPickupDrawContext` binds the original scattered queue, camera, viewport,
atlas image identity, glow identity, display depth and primitive state. The
model producer `00478170`, billboard producer `00478280`, math transform,
screen clipper and native texture queues are real source definitions. The
rectangle producer owns its inlined append in this translation unit; the shared
command kind is a source selector, never an original callback pointer. The
source draw uses the pickup entity's `+fc` rotation, `+100` type, `+10c` offer,
`+110` token and `+114` model handle. Two label layers use the original
screen-depth-dependent scale, eight-slot signed decimal extraction, special
type `0x15` icon path and type `0x14` unit suffix. A typed
`BHPickupRenderBinding` installs default and special `BHEntityStep` adapters
in the constructor's `+e8` draw slot. Each adapter resolves the source owner
from the entity and invokes the corresponding real draw owner.

`python META/m4/pickup-draw/build_focused.py` compiled the real source owner,
linked it with the existing source objects into `pickup-draw-test.exe`, ran 66
focused checks with zero failures, and verified a missing-owner link failure.
Fixtures cover exact record layout/bucket links, projected near rejection,
native quad and texture run publication, unsupported software rejection,
UV rectangle geometry, signed grayscale, depth/fade, signed decimal digit UVs,
special icons, default model/glow/label sequencing, and constructor-created
default and special callback execution. The focused result is
in `focused-build.json`. This does not claim a full source-built game or
software rectangle renderer.

Final integration review added first-label fade early return and explicit 32-bit wrapped price multiplication, with tilted-camera and overflow regression fixtures. Current full build evidence is `../pickup/validation.json`.
