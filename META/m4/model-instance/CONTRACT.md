# 0045aa60 model-instance draw owner

Boundary: `0045aa60–0045ade2`, 899 bytes / 268 instructions in the saved
`META/m4/model-draw/original/0045aa60.tsv` and `.c.txt`. It is an ordinary
rendering owner called from `0045adf0`; no CRT, import, EH, shared-tail, or
compiler-helper boundary appears in its complete body. Historical engine
provenance remains unresolved, so the role is an engine candidate.

`bh_model_instance_draw` keeps the original double position and angle ABI at
the boundary. Six original plane checks use the double position before any
float stores. A rejected sphere leaves the model transform, frame, color,
opacity, and camera snapshot untouched. The diagnostic `drawn` flag is reset
on every successful/cull call to describe that call; it is not original state.

For an accepted sphere, the owner copies the identity matrix at `0048c030`
to model offset `+8`, applies local translation then Y/X/Z rotations after
individual float argument stores, increments the frame, computes grayscale
color, constructs the model camera, and dispatches lit or unlit BSP traversal.
The instruction sequence `FLD [ESP+90]; FMUL [00489f18]; CALL __ftol`
uses the *final float argument*: `00489f18` is pinned `-255.0f`.
`004e3898 = 00491758 - trunc(opacity * -255) * 0x10101` in unsigned 32-bit
color arithmetic. The same float is retained at `0049177c` and used by lit
traversal as its scale. `00491778` is a distinct image flag mask; it remains
the context's `texture_mask`.

The context binds original scattered globals to real owners: `BHMainCamera`
view/planes, `BHViewConfig` near/far, `BHModel` transform/radius, draw cache,
clipper and synchronous primitive emitter, light list/configuration, and the
fixed local reference point at `00495e30`. The caller owns all borrowed
storage and must keep it live during emission. The point-light path transforms
the reference to world space and prepares lights with the model's rigid
inverse. The final copy of `004e2da8` to `004e9448` is represented by
`last_camera_view = camera->view` **after** successful traversal, not a camera
reset. Lit traversal adds node shading to `00491758` (the base color), not to
the earlier grayscale value; the final color reflects the last shaded node.
The original has no error return; the source owner returns failure for
invalid bindings, nonfinite color conversion, camera, light, or draw errors.
Earlier emitted polygons are not rolled back. Color and counters reflect a
partial traversal on draw failure. Frame wrap is rejected because the source
draw cache reserves zero as its initial invalid token; the original increments
its 32-bit epoch without that guard.

Source scope is one model instance. Recursive world/entity placement owner
`0045adf0` remains separate and also requires sprite functions `00455960` and
`004569b0`. The full software primitive dispatcher is still unfinished.
PC53/nearest/masked is the established numeric profile. No byte-match or
original-game execution is claimed. A focused VC5 compile of the real new
translation unit succeeded; parent integration/linked tests provide admission
evidence separately.
