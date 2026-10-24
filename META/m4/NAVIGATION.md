# Navigation graph and routes

Restored full `00407f30` and corridor helper `00407d90` in `src/bh_navigation.c`.
The source-built scene derives 1,454 nodes / 283 junctions. All 80,089 recomputed
route cells exactly match TEST.PTH, including its asymmetry/nonzero diagonals.
The 32-TU checkpoint passed 570,506 real-data / 93,903 clean checks; 79 focused
checks cover topology, cache, allocation and lifecycle behavior.

Contracts/limits are in `src/bh_navigation.h`; exact evidence is in `navigation/`.
The prior reader-only variant stays historical WIP. No gameplay or game admission.
