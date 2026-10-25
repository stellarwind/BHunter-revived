# Display begin, packed BMP and shutdown owner family

Complete owners `0045cdf0` (320 bytes/122 instructions), `0045bc40` (920/276)
and `0045b810` (1,057/322) are restored in `bh_present_begin.c` and
`bh_capture.c`. Shutdown adds complete `0045d750` (76/22) and `004738a0`
(102/26) in `bh_display_shutdown.c`. Classification precedes implementation: these are ordinary
engine candidates operating shared renderer state, actual surfaces and image
owners. Historical engine/library attribution is unresolved. Their call and
control flow does not identify CRT, compiler support, an import thunk or a
shared tail. CRT fopen/fwrite/fclose and conversion support are dependencies,
excluded from the recovered gameplay count. `00451660` was already admitted as
`bh_view_reset_viewport` and is reused unchanged.

`export_review.py` requests every original instruction, verifies contiguous
complete bodies and terminal RET against immutable PE raw bytes, and saves
decompilation, xrefs, boundary metadata and hashes. Source follows the raw
instructions where the decompiler loses stdcall stack/descriptor identities.
No original code executes. The 54-byte BMP header is independently pinned to
the PE. The files are evidence, not a byte-match claim.

## Ownership and dependencies

The begin owner borrows the existing `BHPresentContext`, its fade/render/texture
chain, color/auxiliary surfaces and all nine original texture class lists.
Healthy and lost surfaces are checked in original order. Software Lock uses
flags zero; the color pointer/pitch are published before attempting auxiliary
Lock. Auxiliary failure unlocks color; only two successful locks set the
existing lock flag. The explicit `BHPresentBegin` keeps auxiliary mapping.
Native mode visits every texture slot, restores only `DDERR_SURFACELOST`,
invalidates both sides of its residency association even if Restore fails,
and calls real Direct3D2 BeginScene. No fake vtables or callback substitutions
are used. Source guards reject missing required owners and duplicate software
locking. Diagnostic return values report native failures while retaining the
original later calls and mutations.

The capture owner borrows the same renderer and explicit viewport floats and
pixel-format fields. BGR format arrays have the reverse channel order from
the existing fade RGB arrays. Software output reads the begin owner's actual
published mapping. Native output increments the real shared residency epoch,
flushes every actual image batch with recovered `bh_texture_draw_flush`, calls
EndScene then BeginScene, locks color, writes, and unlocks on both success and
file-open failure. Real CRT stream owners implement output; no archive handle
or binary adapter is involved.

Each viewport float truncates separately toward zero before subtraction.
Rows run bottom to top. Software indexed pixels retain the original unusual
quotient/remainder conversion: `red=(index/42)*255/5`,
`green=((index%42)/6)*255/6`, `blue=((index%42)%6)*255/5`, narrowed to bytes.
16/32-bit fields retain extraction then left expansion without bit replication.
The original writer's packed rows have no 4-byte BMP padding. Its fixed header
file/image size fields remain 640x480-derived; only the two low width/height
bytes change. These quirks are preserved instead of silently modernizing the
format. Source guards reject invalid coordinates, fixed row-buffer overflow,
bad shifts/short pitch and unsupported pixel sizes. Real write/close failures
are reported, a source safety improvement over original unchecked writes.
Native viewport bounds are also checked against the actual locked descriptor
before reading pixels; invalid bounds unlock the surface. The signed texture
epoch explicitly wraps through unsigned32-bit arithmetic, preserving the
original INC behavior without signed C overflow.

## Validation and remaining limits

`build_check.py` compiles all six actual source/test TUs with pinned VC5 SP3
and DX7 declarations, links `bhunter-display-dependencies-check.exe` with real
current integration definitions and stock VC5 LINK, then executes both focused
checks. It records exact source/object hashes, commands, linked owners and
results in `standalone-build.json`. Duplicate stale `agent` objects are
excluded. Root owns admission and current full integration checks separately.

The begin check exercises actual offscreen Direct3D2 scene begin/end, nine
texture list traversal, actual software color/auxiliary locks, publication,
duplicate guards, real first-lock failure, real second-lock failure and
rollback. The capture check verifies every indexed byte, independent 16/32-bit
channels, fractional viewport truncation, bottom-up order and pitch, packed
odd-width rows, retained header quirks, real output failures, native barrier,
residency epoch, actual offscreen pixels and scene usability after failure.
BMP fixtures contain only synthetic memory/private offscreen pixels and are
deleted immediately after binary comparison. No desktop/window screenshots,
image emission, automated gameplay or game startup occurs.

Lost-surface Restore/cache invalidation is statically complete but not forced
on the real system-memory surfaces: inducing display loss would change the
user's display. Tests do not claim that path executed. Invalid cyclic texture
lists remain a caller ownership violation. Pixel bounds beyond the explicitly
borrowed accessible mapping remain a caller precondition. PC24, extreme float
conversion overflow, original runtime equivalence, byte matching, modern
backend and playable game admission remain unclaimed.

## Display shutdown closure

`0045d750` owns exactly three references, released and nulled in order:
Direct3DViewport2, Direct3DDevice2, auxiliary DirectDraw surface. It does not
delete the viewport association or release the color surface, draw owner or
Direct3D2 owner. `004738a0` sets its shutdown marker, invokes that owner, posts
WM_CLOSE with both message parameters zero, closes/nulls its optional real
CRT FILE, then conditionally deletes two temporary paths. The flag and path
strings are retained, so repeated cleanup repeats PostMessage and deletes.
The explicit context exposes these lifetimes without guessed globals.

The dependency `0047de90` is **not** putenv or free. Its complete body calls
DeleteFileA, gets/mapping system errno on failure, and returns zero/-1.
`verify_remove_crt.py` proves all45 original bytes match pinned VC5 SP3 LIBC
`_remove`, masking only actual COFF relocations. Independent precision wrapper
positive control and a changed nonrelocation-byte negative control pass. This
is reused CRT dependency evidence, excluded from source-restoration counts.
Original `0047d680` is one RET, a runtime null callback with no observable
behavior; the source omits its call instead of adding an empty stub.

The18 shutdown checks use owning AddRefs of actual private offscreen objects,
confirm nulling/idempotence and that independent retained owners survive,
and inspect WM_CLOSE on a private message-only STATIC window without dispatch.
All files are newly created private fixtures in the ignored build directory;
checks prove optional FILE close, enabled/disabled deletion and continued
second deletion after the first fails. No environment settings, user files,
visible windows or user applications are modified. The two temporary strings
remain borrowed exact paths; actual startup path construction is a separate
owner and is not fabricated here.
