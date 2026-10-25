# Horizon and depth subdivision owners

Static/source-only work, 2026-10-25. Five complete engine owners now have
maintainable C implementations, focused real-TU links and executed checks.
No original code was executed and no game was launched. Parent integration
build/scenario results and ledger admission are separate from these focused
results. No instruction matching is claimed.

## Classification before implementation

| Entry | Complete body | Role and evidence |
|---|---|---|
| 004784c0 | 004784c0–00478533,116 bytes/37 instructions | engine candidate; actual render command producer uses shared84-byte arena/header0 and points to00477aa0 |
| 00477aa0 | 00477aa0–00477ad3,52/22 | engine candidate; actual queued callback performs render-mode barrier, horizon and next load |
| 0045db80 | 0045db80–0045e70d,2958/699 | engine candidate; horizon geometry/attribute clipping, software color preparation and native fog/depth/batching |
| 0046d630 | 0046d630–0046d9cc,925/314 | engine candidate; recursive depth subdivision, called by0046dda0 and itself, selected leaf callback004e2ddc |
| 0046d9d0 | 0046d9d0–0046dd9d,974/331 | engine candidate; scalar-carrying subdivision sibling, same owning caller/leaf dependency |

All bodies are complete contiguous RET-ended owners. None is verified CRT,
compiler EH/unwind/stack machinery, an import or a separately counted shared
tail. Engine attribution remains unknown; no reusable library was identified.
The two subdivision routines were described imprecisely as scan converters in
the preceding blocker: they subdivide depth ranges, while their selected leaf
callbacks contain actual scan conversion/rasterization. Existing verified CRT
float-to-integer conversion remains a runtime dependency, not a recovered game
function.

Fresh Ghidra get/xrefs/decompile/full-disassembly exports are retained for
004784c0,0045db80 and both subdivision owners. Linear disassembly was explicitly
requested beyond each whole body's instruction count, then trimmed to its
reviewed end. Automatic analysis does not define callback00477aa0 as a function;
its previously reviewed complete instruction body and pseudocode were copied
unchanged from `../scene-callers-review/original`, then independently compared
with the pinned PE. `verify_evidence.py` validates every byte/contiguous boundary,
the original EXE hash, constants and the tested source hashes.

## Real dependencies and ownership

`bh_horizon_queue.c` owns both producer and consumer. Existing real queue storage
remains the sole arena owner; BH_DRAW_HORIZON is appended to the source selector
enum without changing prior selectors. Sparse words are copied at original
+04,+08,+18,+1c,+20,+24, preserving unused words. Header0 commands retain order
with camera commands and precede numbered bucket0. Source binding/topology
guards are explicit additions; exhaustion drops without modifying the chain.

The consumer binds a source texture-handle resolver, executes real
`bh_render_mode(...,0)`, then complete `bh_horizon_draw`, and publishes the
command's current next link after the draw. Missing source image or reported
draw/barrier failure still forwards the link. Its native test uses an actual
texture cache image and no fake rendering device.

`bh_horizon.c` binds the recovered main camera, attribute clipper, render state,
texture cache, horizon color owner and a synchronous typed primitive emitter.
The original global projection is `camera.projected`; eye X/Z and yaw are
`camera.position.x/z` and `camera.angles.x`. The context does not pretend these
scattered original globals formed a struct. Its eight submissions consist of
four inner triangles and four outer quads. Nearer inner nodes have scalar0;
outer nodes scalar1. Software lower triangles select ordinary white-diffuse
dispatch; software upper quads select scalar/gradient dispatch. Native selects
gradient dispatch for all eight. Real color preparation executes for every
software call, retaining existing shared-key cache behavior.

The full mathematical owner is recovered with the original selected-emitter
dependency explicit, as with the admitted model traversal family. Its software
leaf contract is **not an implementation of the missing software renderer**.
Pure software fixtures use an explicitly isolated recording callback. Native
fixtures instead bind the real native-only source adapter and actual source
texture queues/native device. Whole0046e530,0046e390 and0046dda0 remain separate
unfinished dispatch owners; no native prefix is counted as their completion.

The native-only `bh_horizon_native_emit` adapter preserves native0046e530's
live near_z/z depth and1/z rhw, white diffuse0x00ffffff, and specular fog alpha
`(255+trunc(-255*scalar)) mod256`. The existing real texture-polygon owner
publishes actual TL runs transactionally. It rejects software mode and plain
software selections. A straightforward double temporary avoids a VC5 optimizer
ICE in a compounded cast expression; compiler flags/header context are unchanged.

Full horizon enters depth mode2, enables native fog/color, submits all polygons,
increments native residency epoch, flushes every cached image, executes actual
EndScene/BeginScene, restores the previous depth mode and disables fog. Native
API failures remain visible and cleanup/subsequent submissions continue. No
extra blend-state barrier is substituted for this sequence. RGB extraction
uses signed truncating division by65536/256 before masking, including negative
full color words; the color-table owner independently retains unsigned channel
extraction, as the original does.

`bh_depth_subdivide.c` owns both whole recursive functions. It binds the four live
screen limits and selected leaf callback explicitly. The callback can be a real
native source primitive binding in tests; that does not identify it as an
original software kernel. Stack cut vertices are synchronous borrowed values.
All four original edge links restore after both recursive halves, including
source callback failure. Callback reentry, mutation and retained pointers are
forbidden. Valid finite positive-depth convex reciprocal rings are the contract.
Source guards additionally reject malformed rings, invalid arithmetic/progress,
depth over256 and more than65536 leaf submissions.

## Numerical and lifetime contracts

- Subdivision stops only when `(max-min)*16 < min+max`; equality splits. Sum and
  difference remain retained through that comparison. The midpoint alone stores
  binary32. The nearer child runs before the farther child.
- UV and optional scalar interpolate linearly in depth. Screen coordinates use
  depth-weighted perspective interpolation and inclusive live screen clamps.
- Plain0046d630 retains the depth delta for UV division, then stores
  binary32(delta*mid) for screen division. Scalar0046d9d0 instead stores the delta
  before UV/scalar division and retains its product with midpoint for screens.
  Scalar X's lower clamp compares retained X after FST; other bounds compare
  stored binary32 coordinates. Separate `bh_round_float` calls enforce stores.
- A wide-depth independent fixture produces UV bits3e107359 for the plain owner
  and3e107358 for the scalar owner. This proves the variants' distinct rounding
  contract is exercised, rather than hidden by identical dyadic-only fixtures.
- Horizon inner offsets multiply retained sine/cosine differences and retained
  height/outer ratio. Outer offsets instead multiply their separately stored
  binary32 factors. Texture inverse scale stores binary32. UV normally consumes
  retained world-coordinate sums; first inner X and last inner Z reload stored
  binary32 coordinates. Raw instruction stores were reviewed explicitly.
- Native integer fog conversion truncates toward zero, including fractional
  scalar0.5 giving alpha128. Input guards do not claim malformed-original
  behavior. General PC24 equivalence, huge-angle FSIN/FCOS, invalid original
  pointers and live original scene fidelity remain unvalidated.

## Focused validation and parent integration

The five focused scripts compile real source TUs with pinned VC5 SP3 flags,
link against real source module objects and normal CRT/system/native libraries,
and execute named source-only targets. No original mapper or binary callbacks
are used. Commands/source hashes/output are saved in their respective JSON.

| Script | Named executable | Result |
|---|---|---|
| check_standalone.py | build/horizon/horizon_queue.exe |23 checks |
| check_depth.py | build/horizon-depth/depth_subdivide.exe |66 checks |
| check_depth_native.py | build/horizon-depth/depth_subdivide_native.exe |26 checks |
| check_horizon.py | build/horizon/horizon.exe |31 checks |
| check_horizon_native.py | build/horizon/horizon_native.exe |29 checks |

Depth native tests render the actual nearer quad/farther triangle via
`bh_native_primitive_emit`, real texture queues/state flush and native readback,
including a black-color positive control. Horizon native tests verify actual
white sky, brown-versus-blue fog edge and blank below-horizon pixels through
the real queue consumer; both pre-command and final horizon flush barriers,
depth restore and TL fog fields are checked.

Parent integration needs `src/bh_horizon_queue.c`, `src/bh_horizon.c`,
`src/bh_depth_subdivide.c`, the five same-named test TUs, SDK flags for all these
TUs, and checkpoints `bh_horizon_queue_checkpoint`, `bh_horizon_checkpoint`,
`bh_horizon_native_checkpoint`, `bh_depth_subdivide_checkpoint`,
`bh_depth_subdivide_native_checkpoint`. Required map symbols include
`_bh_horizon_queue`, `_bh_horizon_queue_execute`, `_bh_horizon_draw`,
`_bh_depth_subdivide`, `_bh_depth_subdivide_scalar`; the gradient adapter is a
separate `adapter` record, not an additional completed original function.
Source-game/software renderer admission is not claimed.


## Runtime near-depth correction

Final static review of00451720 proves0049175c is mutable: it stores the actual
second argument (near_z), while00491760 stores far_z. The PE initializer1.0 is
not a constant contract. The initial native horizon adapter incorrectly treated
that initial value as fixed. The current variant binds `BHHorizonNative.near_z`
and uses near_z/z for depth, retaining1/z for reciprocal W. A new .125 near-z
control independently checks depth .0625 at z2 while rhw remains .5. Previous
source/native-test snapshots are retained under `near-correction-prior/`; their
validation is not transferred to this corrected variant. All five focused
horizon/depth targets were rebuilt/relinked/executed after correction.
