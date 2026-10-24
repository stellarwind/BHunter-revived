# M4 entity scheduling review

Source variants: `src/bh_entity_schedule.c/.h`. Focused checks: `tests/entity_schedule.c`.

## Boundaries and classification

| Original | Bytes / instructions | Role | Evidence |
|---|---:|---|---|
| 00409f20–00409f3d | 30 / 7 | game scene callback | Decrements room +104 and restores the verified RET callback 0047d680 at zero. Referenced by 00409f40. Not a shared epilogue or compiler helper. |
| 00409f40–0040a87e | 2367 / 663 | game scene scheduler | Called by 00409130, recurses into itself, reads game room/resource/portal fields and invokes game entity draw methods. No library signature or CRT helper pattern. |
| 00409530–00409f15 | 2534 / 845 | proprietary engine candidate, portal-specific convex clipper | Called by 00409f40 for transformed portal quads. Six clipping planes, projected x/y and scratch vertex links. No verified reusable external library provenance. |

00478990 and 00478a70 are concrete native draw command queue producers, assigned to `bh_draw_controls.c` by the integration owner. They are not callbacks. 00478170 is already recovered in `bh_draw_submit.c`; 004012e0 is already recovered in `bh_math.c`. Entity +e8 draw is the actual indirect game method supplied through the pickup/actor bindings. 0047d680 is a verified RET callback and is represented by inactive source room frame state.

## Recovered control flow and source bindings

00409f40 first submits its own room resource when bit 4 is set. Water/resource flag 0x40 with ordinary bucket selection surrounds bucket 0 and 3 resource submissions with four state commands; the root skips a trailing default submission. Each call installs a 30-frame callback. Once per frame, it draws the +128 actor chain by +f4 and the gated +12c pickup chain by +ec, reading each next link *after* its +e8 method returns. It then visits linked rooms from +124, drawing each resource, distance based overlay and gated pickup list. The linked water case has a trailing default submission after the two surrounded draws, confirmed from 0040a192–0040a1e7 disassembly; the decompiler's shape at that point is misleading.

The linked chain checks each candidate's +1c8 before rendering. A zero member is a terminal sentinel and is not rendered. Next, each portal with a neighbor checks camera side against integer plane and axis direction 0/1/2/4/5/6. It constructs a four-corner aperture from the overlap extents, transforms through the projected camera matrix, clips to near/far and current screen rectangle, queues a new rectangle for buckets 0 and 3, recurses, and restores the previous rectangle. The outer 00409130 owner queues the final full-screen rectangle after scheduling. Thus no local restore command is emitted by 00409f40.

`BHEntityScheduleRoom` binds one borrowed `BHPickupRoom` and explicit source resource handle/flags, pose, linked-next, linked-active and portal-target values. This deliberately avoids casting `BHWorldGeometry` or `BHEntity` to the original room layout. `BHPickupRoom` supplies real actor and pickup heads. `BHWorldGeometry` supplies recovered position, bounds, flags and portal navigation. The caller must preserve these bindings and the draw callbacks for the synchronous schedule call. The source uses a bounded depth and list length to reject malformed cyclic bindings; original code lacks those protections.

`bh_entity_schedule_clip_portal` uses the fully recovered six-plane `bh_clip_polygon` owner with an untextured four-vertex ring. Its scratch storage is per recursion, preserving the original temporary lifetime. This is a functional source composition of the 00409530 portal contract; it does not claim byte identity or all non-portal caller uses of 00409530. There are no other mapped callers.

## Validation and remaining integration

Focused tests target root and linked resource ordering, two child lists, frame suppression/expiry, overlay submission, portal camera-side/depth rejection, recursive clip command order, clip rectangle restoration, and near/side ring clipping. Link and run results will be recorded after draw-control producers and pickup lifecycle owner land. Frame/overlay floating-point precision and real-level portal recursion pixels remain unvalidated. There is no game execution, original code execution or gameplay automation in this block.
