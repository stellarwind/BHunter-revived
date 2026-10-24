# Queued resource producer and callback

Reviewed original boundaries: `004775f0–0047763f` (80 bytes) and
`00478170–00478256` (231 bytes). Complete decompile and instruction exports
are in `original/`. `00478170` is called from world/entity/effect owners and
stores `004775f0` as the new draw command callback. The callback has one
outgoing call to the restored `0045adf0` resource dispatcher. These are
engine-candidate rendering behaviors with unresolved proprietary/game
provenance, not CRT, imports, EH helpers, or independent shared tails.

The producer uses the same nine-header draw arena as the existing primitive
producers. It checks count against capacity before appending, maps bucket
`0..8` to headers `1,2,3,4,5,0,6,7,8`, and maps an out-of-range bucket to
header 0. It inserts a `0x54` node after that header's tail and copies the
old successor into the new node's link at `+0x50`. The producer writes the
callback at `+0`, position floats at `+4/+8/+c`, angle floats at
`+10/+14/+18`, modulation float at `+1c`, and a resource descriptor pointer
at `+20`. Full capacity returns without payload writes or count increment.

The callback reads the descriptor at `+20`, promotes the six binary32 pose
values to doubles in original argument order, passes modulation unchanged
to `0045adf0`, then reads and returns the link from `+0x50`. A draw that
changes this link affects the returned value. It has no resource lifetime
owner and ignores the draw result. The source representation uses
`BH_DRAW_MODEL_RESOURCE` instead of an original function pointer and stores
an opaque handle at `BHDrawCommand.resource`; a caller-supplied lookup binds
that handle to a native `BHDrawResource` during synchronous execution. The
descriptor, its nested model/image bindings, lookup user data and renderer
context must outlive the callback. No original pointer is cast into a native
pointer or dereferenced by the shipping source.

`bh_draw_submit_resource` preserves raw float bits and queue ordering. It
adds argument and topology diagnostics consistent with `BHDrawQueue` and
returns `1` queued, `0` full, `-1` invalid. `bh_draw_submit_execute` copies
the command arguments before lookup, then returns the source renderer's
status and publishes the current next link after draw. The added lookup must
not mutate the command, and the `next` output must not alias command storage.
For a missing descriptor binding, it publishes the current link and reports
failure without drawing. The original callback always read the link after
its void draw call. The source resource dispatcher also has finite-input and binding
guards. Thus malformed/missing native bindings report failure rather than
imitating original undefined behavior. Floating point runtime PC24
transitions remain unresolved; the linked check uses the established PC53
source profile.

Dependency owners: `BHDrawQueue` arena and reset in `bh_draw_queue.c`,
`BHDrawResource` dispatch in `bh_draw_resource.c`, and billboard/model/oriented
primitive owners reached by resource tags. This block implements the inline
append in its own translation unit so earlier admitted queue source variants
are untouched. Full source integration and admission evidence are recorded
by the parent in `META/function-status.json`.

## Billboard callback `00477640`

The complete 63-byte body spans `00477640–0047767e`; reviewed decompile and
instruction exports are in `original/`. It is an engine-candidate rendering
callback installed by the already restored `00478280` producer, not CRT,
compiler support, an import, or a shared tail. Its only outgoing call is the
restored `00455960` billboard owner. It reads the image pointer at command
`+4`, position floats at `+8/+c/+10`, width/height/angle at `+14/+18/+1c`,
the final modulation float at `+20`, and low-byte flags at `+24`. The
decompiler omits the final modulation argument from the function signature;
the disassembly pushes `+20` first for the ninth argument, then `+24` flags.
It reads `+50` after the sprite call and returns that current next link.

The native command stores an opaque image handle in `resource` and resolves
it through `BHDrawImageLookup` during synchronous execution. The image and
sprite context bindings are borrowed until the draw returns. The callback
copies its payload before lookup, masks flags to their low byte, invokes the
real source billboard owner, then publishes the current link. Lookup may not
mutate the command and the `next` output may not alias command storage. A
missing native image binding reports failure but still publishes the link.
The original callback had no such diagnostic return; it used its image
pointer directly and ignored the sprite draw result.
