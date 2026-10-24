# Native cached atlas/glyph-sheet loader 0046f490

Classification corrected before implementation: this is a native DirectDraw
offscreen atlas owner, directly called by00470380 font/page initialization.
It is not a software pixel-buffer loader or raster kernel. Engine-candidate
provenance remains unknown. Its complete3,810byte/1,087instruction body is
ordinary owner logic; verified malloc/free/case-comparison CRT dependencies are
reused, not counted as newly recovered game code.

BHAtlasImage preserves original x86 record size0x13ac: next+00, literal
name[128]+04, transformed RGB palette[768]+84, official DDPIXELFORMAT+384,
rectangle_count+3a4,512{u16x,y,width,height} rectangles+3a8, real owning
IDirectDrawSurface+13a8. Explicit context binds original cache, real native
draw/allocator, current palette policy and independent scan step.

Cache keys are the full literal filename compared case-insensitively. Cache hit
precedes format/palette/scan-state consumption. There is no extension rewrite,
texture prefix or search policy. The owner opens that exact filename through
real admitted VFILE/PCX reader, preserving original12-byte header checks,
xmax+1/ymax+1 dimensions and RLE behavior rather than standardizing PCX origin,
planes or stride. It reuses real admitted0044f1c0 palette conversion and map.

All outputs are actual native offscreen/system-memory surfaces, original caps
0x840.8bit output stores mapped indices directly at real surface pitch; the
palette is saved in the atlas but no native palette is created/attached.16/24/32
outputs pack transformed palette components using each mask's highest bit
position and original integer shifts/masks. The original mask-length loop uses
signed division by2; source preserves that expression but explicitly rejects
RGB masks containing bit31 because the actual native backend rejects that
surface profile. No broad signed-mask native format claim is made. They write native rows at actual
surface pitch and temporarily retain tightly packed mapped indices for rectangle
extraction. Scratch indices are freed after extraction, as original.

Rectangle extraction selects the first mapped pixel as transparent, then finds
consecutive nonblank row bands and consecutive nonblank column groups within
each band. Record order is row band then left-to-right column. Vertical column
sampling advances by original global004e9750, the active screen pitch set by
0045cdf0/0045cbc0. It is independent of the atlas/index pitch. In native mode
that global can remain zero; zero repeats the first band row and is explicitly
preserved/tested. Source does not silently substitute correct-looking atlas pitch.
Every sampled span is checked against available index storage before reading;
arbitrary steps that would read outside storage safely reject.

Source failure/lifetime adapters are explicit: dimensions1..4096, name<=127,
standard contiguous/disjoint8/16/24/32 formats, at most512 rectangles; allocated
resources clean up on failed decode/native/allocator/topology checks and cache
publishes only after success. The original publishes earlier and can retain a
partial entry after failed native Lock. No claim is made for that defective
failure-state cache behavior. Dispose releases actual native surfaces and records;
no original destructor recovery is claimed.

`python META/m4/atlas-image/build.py` compiles actual source/test TUs with pinned
SP3 compiler/headers and links current real m0 owners, stock VC5 LINK, official
DirectDraw SDK library and system libraries into
`build/atlas-image/bhunter-atlas-image.exe`. All3,439 checks pass across actual
native8/16/24/32 output and all4 palette policies: independent native pixel
values; exact3glyph geometry/order; mapped-key transparency; palette snapshot;
literal-name/cache semantics; no native8bit palette attachment; zero/matching/
unsafe scan step; odd-width native row pitch versus packed scratch; truncated
real RLE; rectangle overflow and each owner/scratch allocation failure. There
are no fake COM interfaces or original-code calls. A fresh signed-mask correction
rebuild passes3,439checks, including explicit preallocation rejection of bit31
RGB masks. Earlier3,436check evidence is not transferred to the changed source.

`audit.py` verifies complete instructions against immutable EXE and focused
source hashes/results. No original/game execution or media writes. Font/page
caller integration, actual game screen-pitch binding and complete game rendering
remain parent work. Current native dependency objects are recorded by hash and
require parent full integration rebuild for admission.
