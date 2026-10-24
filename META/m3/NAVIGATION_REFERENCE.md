# Completed original portal graph and candidate training route

Run `startup/2e901f78ca70.json` compiles, links and executes a read-only navigation snapshot at the original bootstrap return. It sends no input and stops before the first simulation callback. **No menu/prologue/flight sequence was replayed.** Original references, protected Windows files and original registry views remain unchanged; breakpoint restoration and private-key cleanup verify.

## Graph ownership

Complete reviewed owners `004084a0` and `00408940` build each scene placement's six possible portal records and match neighbouring apertures. Their role remains engine-candidate; verified math/CRT dependencies are not new gameplay assignments. `navigation-review/assignment.json` records classification before their exports. This is reference analysis, not source graph-construction admission.

An actual portal is a 60-byte record at scene entity `+1cc + slot*3c`. It contains direction, plane, original face extents, clipped overlap extents, scratch overlap extents and neighbour pointer at record `+38` (entity `+204 + slot*3c`). Adjacent portals use opposite direction code `direction XOR 4`, the same plane and overlapping projected extents. Their recorded overlap is the intersection of both original apertures. Scene entities with no own portal model may resolve to an owning geometry entity through `+1c8`.

`00407f30` puts entities with a connected degree other than two first in `004df268`, assigning their indices at entity `+108`, then appends degree-two corridor entities. In this actual graph there are **1,454 nodes**: 283 junction/end nodes and 1,171 corridors. The older bootstrap field named `navigation_count_raw` reads only `004df264 = 283`; it was never the total graph-node count (`004df26c = 1454`). This distinction matters when interpreting the path matrix.

The snapshot records 3,739 model portals, 3,120 of them connected. All connected records have an actual reciprocal opposite portal and correct clipped overlap. The live 283-by-283 route matrix is exactly equal to the archived `TEST.PTH` payload after its count word. Its exit values are **local portal slots at junctions**, not neighbouring global node IDs. Degree-two corridors are followed through their actual pointers.

## Candidate route

The final introductory sensor, placement 5064, resolves to graph node 1296 / geometry placement 5063. Target launcher 4827 resolves to node 220. A search over the captured connected portals yields 15 edges, with a bounds-centre distance of approximately 1216.977 units. All four junction decisions on that path agree with the original matrix, including its remaining distances within the stated numerical tolerance.

The route first follows the corridor along approximately X=-55 through Z=-120, -150 and -210. It then bends toward X=80 along Z=-245, continues through the next openings near X=95/Z=-260..-380, and crosses successive rooms toward the launch area. Exact source/destination placement IDs, local exit slots, aperture centres and opening dimensions are in `navigation-route.json`.

The failed direct approach's sampled positions were near the first corridor. That observation and the connected detour justify changing the next route; they do **not** prove a collision occurred, identify a specific obstructing polygon, or establish a bug in the original controls. Collision/contact was not captured in that variant.

`python tools/check_m3_navigation.py` verifies the executed source snapshots, real graph indices and neighbour pointers, reciprocal portals, original aperture clipping, matrix equality and route decisions. Every proposed portal centre lies in both adjoining captured node bounds. The narrow initial openings include 15-unit vertical spans. A centre point and graph connection do not prove swept ship clearance, line of sight, navigability by this control policy or a successful combat scenario.

## Next runtime scope

Use the player's actual current world owner and the moving target's current navigation owner, checking them against the new process's actual node array/placement identities. The candidate's destination is the **initial launch area**, not a guarantee that the moving target will still be there after the flight. Follow verified portals and advance from actual owner transitions rather than blindly chasing the distant target through intermediate geometry. Retain the corrected positive-forward convention and capture contact/clearance and original shot/hit/death outcomes together.

No position, graph, physics, health or mission-state injection is introduced. No computer-use skills, visual identification, original-code mapper expansion, source-game admission or M3 completion is claimed. Kills, mission completion, failure/retry and the other open M3 scenarios remain unvalidated.
