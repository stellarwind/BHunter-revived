# Global camera source restoration

Four independent ordinary owners are implemented by `src/bh_main_camera.c`:
00451720 (settings), 00451750 (global projection), 00452070 (pose), and
004018a0 (world-Z pre-rotation). Complete instruction boundaries, body hashes,
provenance, source binding and limits are in `review.json`. Attribution stays
`engine_candidate`; shared camera/matrix technique does not establish authorship
or an available source library. No CRT or compiler scaffolding is counted.

The global owner is distinct from per-model 00451b80. In particular it writes
**five**, not six, planes. Original slots at 004e97b8, 004e97d8, 004e97e8,
004e97f8 and 004e9808 receive forward/right/left/top/bottom planes; the slot
004e97c8 stays untouched. All offsets are dot(normal,eye). Forward is positive
camera Z through the eye; side planes have outward normals and their interior
satisfies dot(normal,point)<=offset. There is no invented near/far-depth plane.
Near/far settings are retained in the config but ignored by this projection.
The global owner also resets original 004edea4 to zero.

BHMainCamera is an explicit source context adapter binding the projection,
inverse, pose matrix, angles, eye, planes, focal and projection-state word.
It does not assert a contiguous original ABI layout. Its owning caller must
initialize its bytes; the build retains plane slot1, angles and view storage.
No pointers are retained. The pose accepts six doubles (position, yaw, pitch,
roll), stores binary32 angles/position and builds a matrix from the verified
original identity constant, translation(-eye), world-Y(-yaw), world-X(-pitch),
world-Z(-roll), then global build. The build recomputes eye from rigid inverse,
retaining its float rounding rather than forcing a copy of input coordinates.

Original input/output partial overlaps are unsupported except the exercised
own-view binding and own-projected self-alias. The latter inverses the newly
projected matrix, matching actual instruction order; input depth row stays
unmodified. This does not promise a general inverse for nonorthonormal input.

Explicit binary32 stores preserve half-FOV, projection centers, trigonometric
coefficients, vertical aspect coefficient, matrix outputs and planes. Global
vertical coefficient uses stored cosine; this differs from the retained cosine
in the existing per-model source owner. Plane dot grouping is (x+y)+z. Current
scope is caller PC53/nearest/masked profile with moderate finite angles; CRT
sin/cos/tan replace x87 transcendental instructions. No byte equivalence or
original differential execution is claimed. Invalid/nonfinite/overflow guards
are explicit adapter behavior and preserve output on rejection.

`python META/m4/main-camera/build_focused.py` compiles the seven real TUs and
links `bhunter-main-camera.exe` with real math/rotation/FP/round/view config
definitions. All 2025 source checks pass: independent pinhole coefficients,
translated/rotated inverse, screen coordinates, original plane signs/slots,
depth independence, pose multiplication order and world-Z translation effects.
Removing the owner TU produces four actual LNK2001 unresolved symbols.
`python META/m4/main-camera/audit.py` verifies every exported instruction against
the immutable EXE, constants, source hashes and all four immutable references.
No original code or game launches are used. Parent integration and ledger
admission are separate responsibilities and must name their actual result.

An initial fixture run failed because the test compared compiler-retained
extended cast values with binary32 fields and transformed a point in place
despite the known sequential-write math contract. Those two fixture mistakes
were corrected using real binary32 store owner and a distinct output; no camera
source change was needed to make the final focused run pass.
