# Sphere/frustum hierarchical mask owner 0045a7c0

Complete independent owner 0045a7c0-0045aa54 contains 661 bytes and 217
instructions with no outgoing calls. Its ordinary full body, global planes,
depth config and recursive rendering caller 00477880 classify it as
engine_candidate, medium confidence. No CRT/EH/import/shared tail or available
source-library attribution is established.

`bh_view_cull_sphere` uses existing BHMainCamera/BHViewConfig contexts without
changing their source variants. Plane slot0 supplies signed forward depth;
slots2/3/4/5 supply right/left/top/bottom signed distances. Reserved plane1 is
never read. Each plane dot uses original (x*nx+y*ny)+z*nz-offset grouping.

Mask bits1/2 are near/far. Presence of either bit executes BOTH depth tests,
including rejection by the other boundary. Depth below near-radius or above
far+radius rejects with0xffffffff. Depth strictly above near+radius clears
through AND0x3e; depth strictly below far-radius clears through AND0x3d. These
are exact six-bit masks that discard unknown high bits when they execute;
they are not replaced by a conventional clear-one-bit operation. With neither
clear executed, high bits remain. Side bits4/8/16/32 reject when signed distance
is strictly greater than radius, or subtract their bit when distance is
strictly less than negative radius. Subtraction preserves unknown high bits.
Both internal/external tangencies retain the relevant bit. No up-front masking
or normalization changes unknown bits or the returned original mask value.

Requested optional depth is written even for mask0 and before any rejection.
Absent output does not change classification. Full instruction review finds
an FST float store that retains the x87 value; source comparisons therefore
continue using PC53 double distance rather than reloading stored binary32
depth. A positive-control fixture at depth1+2^-30 produces output1.0 while
correctly clearing the near bit; exact depth1.0 retains it. This prevents a
silent spill from changing strict-boundary behavior.

Lazy reads preserve original contracts: mask0/depthNULL does not read either
context; unknown-only masks/depthNULL return unchanged without reading planes;
config is needed only when a depth bit is present. Plane0 is read for requested
depth even when no depth bits are active. Storage validity, finite inputs,
nonnegative radius, original normalized plane meanings and caller PC53 are
contracts, not guessed validation. No clipping state or pointer is retained.

`python META/m4/view-cull/build_focused.py` compiles eight real TUs and links
`bhunter-view-cull.exe` with the real main-camera/view-camera/math/rotation/
round/FP owners. All 245856 checks pass: 32 varied real poses, 40 spheres per
pose and all 64 masks, independent sphere-interval classification and signed
depth controls, optional output, all tangent/rejection thresholds, both exact
AND masks, unknown high bits, lazy NULL contexts and retained-depth precision.
Removing the cull owner produces actual LNK2001. No original code executes
and no game launches occur.

`audit.py` verifies every instruction, explicit AND/SUB/FST anchors, all caller
instructions and call site, source hashes, focused target result and all four
immutable reference hashes. Integration and ledger admission belong to the
parent; this does not claim the recursive world renderer or gameplay complete.
