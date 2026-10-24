# Geometry face access

Restored `0044e060`, `0044e210`, `0044e230`, `0044e240`, and `0044e2a0`
in `src/bh_geometry.c`, with real face-flag ownership over decoded BSP data.
48,609 family checks cover all 505 real BSPs. The 27-TU checkpoint passed 482,798 checks.

API/lifetime contracts are in `src/bh_geometry.h`; boundary, fixture, build and
validation evidence is in `construction/geometry-*` and `construction/review.json`.
Full rendering/collision objects remain unfinished. No game launched.
