# Independent static review — HUD world connectors and actor markers

2026-10-25, read-only original review. No game/original-code execution. Source, tests, tools and global ledger were not edited by reviewer.

## Boundaries and owners

- `00417f30..00417f6c` is the complete 61-byte visited-mark reset. `00417f70..00418ea1` is the complete 3890-byte world-room/portal radar connector traversal. `0041b2d0..0041b64c` is complete 893-byte player HUD placement/setup. These are neither allocator/destructor families nor model attachments. Unknown engine authorship remains open; reset/traversal engine-candidate, player-specific setup game. Existing source world geometry provides bounds, query flags, six portal records and traversal mark; no new allocation contract should be invented.
- +fc is node visited, +114 query flags, +1b0/+1bc world bounds, +1cc six portal records stride60, +204 neighbor. Portal direction and neighbor words shown as floats by Ghidra are bitcast artifacts.
- Reset clears mark BEFORE child traversal in slot order. Already-clear parents suppress children; shared nodes/cycles valid. No resource free/link mutation.
- Exactly two live portals are drawn as one segment followed by first recursion and second tail visit. Other degrees draw center-to-portal slot order. Radial clipping mutates the shared center and affects later slots. Post-clip radii control recursion; zero height fade sets second endpoint radius400 and still permits first traversal in the two-neighbor path.
- No hidden installed callbacks in these three bodies. HUD deps are screen quad, graph traversal/reset. Queue/drop/visited state and borrowed image/world lifetime must be explicit.

## Attachment numeric review

Current source was independently checked for atan2 argument order, mixed stored/retained radius operands, flat X vs Y radial rounding, world double component stores, visited order and fade recursion. atan2(dx,dz) and atan2(mid_x,mid_z) match FPATAN stack order. World components are actually stored double at004186a8/b4 despite misleading decompiler casts.

One candidate correction sent to implementer: clip weights FST at00418352/60 retains the original quotients for updated X, whereas later Y/Z use reloaded stored float weights; analogous00418408/16. Source at review time rounded both weights before X. This note is static evidence, not a claim that a corrected implementation has been validated.

## Marker owner0041bbf0

Whole1029-byte/317-instruction body through0041bff4, all317 instruction bytes compared with immutable PE; see boundary JSON. Classified game/high confidence from player caller and actual kind/atlas logic; no CRT/EH/thunk/tail count. Disassembly CLI must use explicit `-n317`; default function disassembly silently produced10rows and was overwritten with full output.

Actor head004dfb28 uses null-terminated doubly linked +ec/+f0, distinct from allocator query-list +d4/+d8. Callback+d0 is cdecl(entity,packet); packet event DWORD3 at+0 and xyz float outputs+4/+8/+c. Existing query packet capacity40 applies. Return ignored; kind+fc is read AFTER callback and +f0 AFTER draws. Callback/self/list lifetime and mutation ordering matter; no direct-graph closure without this query.

Observed insertions:00427920 kind8;0043a920/0043b2d0 normalized1/2/6;004280e8 kind8 before00428122 insertion;00445348 kind6 before00445364 insertion (installs0043c220 query). This is strong supported-domain evidence, not exhaustive producer closure. Other data xrefs include undeclared boundaries/removals. Unsupported kind default still projects using stale atlas locals in original; do not invent a default atlas. A source supported-kind failure guard should occur after query to preserve callback effects.

Kinds1/2 produce radar kind3/0, extended0, skip0, atlas U0/.25 and V.25/.375. Kinds6/8 produce kind1, extended1, skip(player+2b8&2), U.5/0 and V.5/.25. Kind8 label uses signed short actor+114 times double.125 and separate image004cdb9c; others use marker image004f00b4. Projection004558d0 and rectangle00478f60 have real recovered definitions.

Constants/types are exact PE bytes in constants JSON: alpha factor100f, cap1f, threshold0.1double; depth subtract8f and clamp to near-(-.01double); layout -2f,-4.5double,-9.5double and label V scale.125double. Runtime near0049175c is live, not PE initialized value.

Alpha retained radius*100/signed-width is compared against1 before stored-float alpha is reloaded and compared against double0.1. Depth z-8 is stored float but compared retained against retained near+.01, then clamp stored if below/unordered. Every draw float argument rounds at final stack FSTP. Width-derived integer multiplication retains 32-bit signed/wrapped original domain contracts; list/width/image adapters are not constructor recovery.
