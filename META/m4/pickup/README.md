# Pickup construction and scene scheduling checkpoint

Seventeen complete source owners and one partial native rectangle adapter are
recorded separately in owners.json and META/function-status.json. The actual
constructor, tick/query/cleanup, default draw and numeric labels compose with
source room/entity scheduling, portal clipping, texture masks and clip commands.
No original code is mapped or executed. Original assets are read-only data.

The 158-TU target passes 3,399,331 real-data / 2,906,922 clean source checks;
78 absent-module link controls fail as required. Five focused scenarios account
for 287 real-data checks. Their clean equivalents need no original assets.
The native fixture constructs a pickup in an explicit one-room binding, invokes
its actual +e8 draw callback from the scheduler and renders real PU_9A, flare_yl
and clipbrd images. Clearing the room draw flag yields a blank control.

Review fixed the first-label depth early return, stale portal-target traversal,
32-bit label-price overflow and missing selected draw callback. Unit fixtures
cover those cases alongside callback list mutation, timing/lifetime transitions,
six clip planes, linked inactive sentinel and strict ghost distance boundaries.
Raw constructor/tick constants are verified by the lifecycle constant checker.

Reproduce with build.cmd (clean source), python tools/m0.py build --archive
GAME_DATA/Bhunter.bin (real data), and python tools/check_pickup.py (saved
provenance/build audit). The previous static level image remains byte-identical;
its placement scheduling remains diagnostic. This checkpoint is not a playable
source game, does not establish x87 PC24 fidelity and launches no game.

00454fc0 is explicitly WIP: native geometry/clip/submit works, while software
rasterization remains unavailable through the existing 0046e390 adapter. Next
source frontier is outer scene owner 00409130 and actual world/resource binding,
followed by actor/startup construction; no guessed tables or vtables are supplied.
