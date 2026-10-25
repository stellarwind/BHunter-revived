# HUD setup owner 0041b2d0

The complete 893-byte / 233-instruction body ends at 0041b64c (exclusive end
0041b64d). It is game-specific HUD/room-radar orchestration, with high-confidence
boundary/role evidence: the sole external caller is the complete player scene
callback at 0041d189; all outgoing calls are the complete screen-image queue,
world/portal graph radar renderer and guarded recursive graph reset. There is
no callback installation, compiler EH/unwind helper, import or independent
shared tail in this owner. The older “attachment/weapon” frontier description
does not establish a separate attachment allocation hierarchy: the root +c4 is
the already constructed world/portal graph, and this owner calls no weapon
behavior. No separate gameplay function is counted for its inline root reset.

`review.py` verifies every instruction byte against the pinned immutable PE,
checks contiguous coverage and pins all nine referenced numerical constants.
The historical complete disassembly/decompile/xrefs are retained under
`META/m4/radar/original/0041b2d0.*`; fresh Ghidra boundary/xref queries confirmed
the same boundary and caller in this work block. `raw-review.json` records the
whole raw-body SHA-256. This is reference verification, not source byte matching.

## Real source bindings and numerical contract

`src/bh_hud.c` / `.h` implement the full owner. `BHHudPlayer` is a verified
prefix view rather than a reinterpretation of the generic BHEntity: position
+0c, right +18, up +24, forward +30, yaw +3c, borrowed world root +c4 and raw
nonzero/zero flat selector +e8. The unknown bytes retain their exact positions.
The input is not modified; graph traversal marks are owned by the constructed
world component and are deliberately modified then reset through real owners.

The scattered output globals map as follows:

| Original | Source binding |
|---|---|
| 004df218..220 | `frame.axis_x` |
| 004df228..230 | `frame.axis_y` |
| 004df1a8..1b0 | `frame.normal` |
| 004df1b8..1c0 | `frame.origin` |
| 004df1c8..1d0 | `context.world_center` |
| 004d18f8/18fc/1900 | `frame.yaw_x` |
| 004d18e8/18ec/18f0 | `frame.yaw_y` |
| 004d1a78 / 004d1898 / 004decec | unit / diameter / screen radius |
| 004ce2a8 / 004ce444 / 004d1a9c | screen x / screen y / flat |
| 004cda28..30 / 004d1998 | attachment context player position / yaw |
| 004e2998 / 004f00c0 | attachment signed viewport width / HUD background image |

PC53 nearest/masked operation and moderate finite inputs are the admitted
source contract used elsewhere in the restoration. Raw-body FST/FSTP review
preserves three important distinctions lost by decompiler simplification:
normal.z remains unrounded for center.z, center.x/z remain retained for final
radar placement, and center.y is stored/reloaded before final placement.
The .8, -.1, -.8, .707 and -.707 placement constants are binary32; the -.02,
.043 and -.083 offsets are binary64. In the flat branch, signed width/80
truncates toward zero. None of the subsequent integer arithmetic can overflow:
the largest absolute unit is 26,843,545, diameter 429,496,720 and screen-x
magnitude 1,905,891,743. World mode preserves all prior screen metrics.

Every setup still runs with a null graph root. For a live root, player origin
and yaw are bound immediately before the real recursive renderer; guarded
reset occurs afterward, including after a source renderer error. Full queues
silently drop draws while setup/traversal/reset continue. Live near-plane and
visibility state are owned elsewhere and remain unchanged. Queue storage,
frame, input state and graph must not overlap; borrowed graph/queue/image
lifetimes extend through queued consumption. No allocator, fake vtable,
constant-success callback, original-backed adapter or game loader is supplied.

## Validation scope

`tests/hud.c` exercises the real setup translation unit with real queue,
screen-image producer, recursive graph renderer/reset and downstream marker
owner. It covers the complete setup with a null root, both rendering modes,
background-before-connector ordering, cyclic graph reset, a full queue,
renderer failure cleanup, exact player layout, retained live globals, signed
viewport boundary/extreme values and source argument/numerical guards.

`fixtures.py` generates 32 fixed precision fixtures by interpreting the
reviewed floating-point instruction records **as data** with a generic stack
evaluator. Explicit FST/P operations round binary32; ordinary Python binary64
operations implement the chosen moderate PC53 arithmetic contract. Each
fixture checks all 21 world/yaw/center output words. This is static contract
evidence, not execution of original instructions or an original differential
test. PC24, exceptional/very-large transcendental behavior, native visual
fidelity and the full player scene caller remain outside these tests.

Implementation/integration admission and the actual current target results are
recorded separately by the block's final evidence and global function ledger.
No original game code or game process is executed by this family.
