# World geometry construction

Restored `004084a0`, bound callbacks `004082d0`/`004083d0`, material classification
`00410880`, and Y rotation `004017e0` in the real linked source target.
All 5,815 real placements construct from 454 models. Saved 1,454 node bounds,
3,739 portal records, 3,120 links and four owner resolutions match exactly.
The 30-TU checkpoint passed 567,512 real-data / 93,824 clean checks.

Contracts are in `src/bh_world_geometry.h` and `src/bh_rotation.h`;
exact evidence is in `construction/world-*` and `construction/review.json`.
Grids remain explicit fixture inputs; full scripts/entity installation and general
rotation fidelity remain open. No source-game admission or game launch.
