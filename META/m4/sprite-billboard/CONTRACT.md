# 00455960 billboard sprite producer

Complete original boundary `00455960–00455e31`: 1,234 bytes and 291 decoded
instructions. The saved full disassembly and decompile are
`../screen-clipper/original/00455960.tsv` and `.c.txt`. This is an ordinary
sprite geometry producer: it transforms a world point, builds a four-vertex
screen polygon, calls the screen ring clipper `00453300`, then dispatches the
image and color to `0046e390`. It is distinct from verified CRT, compiler,
import, EH and shared-tail code. Historical engine provenance remains open.

The decompiler omitted the ninth argument. The disassembly at `00455de2`
loads `[ESP+0x144]` after a `0x11c` local frame and an ESI push, which maps
to the final float modulation after image, three position floats, two size
floats, angle and byte flags. That float is multiplied by original binary32
`-255.0f` at `00489f18`, truncated by `__ftol`, then its grayscale triple is
subtracted from `00491758` modulo 32 bits. The eighth argument's low byte
controls U flip (bit 0), V flip (bit 1) and UV quarter-turn (bit 2); high bits
are ignored. No texture flag mask is read by this body, so caller filtering
stays with the recursive owner.

The source context explicitly binds original camera projection/focal, depth
and screen bounds, clipper scratch, color and synchronous primitive emitter.
World position passes through recovered `bh_mat34_transform_point`; depth is
checked before division. Projected half sizes use focal/depth and pinned
binary32 `0.5f`, then a circular screen bound controls early rejection.
Angle zero uses the original rectangle branch; other finite angles form the
four corners after original float storage boundaries. The four vertices form
an A→B→C→D→A ring. The real `bh_screen_clip_polygon` consumes it; after
clipping, all surviving vertices receive the projected depth before emission.
The emitter consumes the ring immediately and cannot retain its stack nodes.

Source guards reject invalid contexts, nonfinite inputs/results and unsafe
color conversion. A clip or emitter failure returns an explicit error; the
original void owner had no error result. Rejected depth/screen sprites and
fully clipped polygons return success with `emitted=0`, retaining color.
`0046e390` is represented by the explicit emitter binding; its native path is
available, while the full software primitive dispatcher remains unfinished.
The emitter receives the actual borrowed image; no placeholder image is made.

The named focused target `bhunter-sprite-billboard.exe` compiles the new real
translation unit with pinned VC5, links real matrix, screen clipper, round and
FP definitions, then runs 50 source-only checks. They cover an independent
rectangle and quarter-turn oracle, UV flags, depth, clipper ring, modulation,
depth cull and emitter failure. No original code or game execution occurs.
