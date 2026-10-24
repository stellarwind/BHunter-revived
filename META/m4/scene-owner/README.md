# Outer scene owner checkpoint

00409130 is classified as a complete game scene owner (1,012 bytes / 296
original instructions), restored in src/bh_scene_owner.c. It gates disabled
scenes, builds the portal projection, selects cached/exact/margin/fallback rooms,
advances the frame, invokes the real entity scheduler and queues both full-target
clip resets. Source return/error reporting extends the original void API.

BHSceneBindings is an explicit source adapter. Constructed world ring successors
(+124) and containment owners (+1c8) define linked-room traversal and its root
sentinels. Matched portal neighbors define a separate graph. Resource handles,
flags, poses and entity list heads are borrowed, validated bindings; the adapter
does not invent original constructors or claim block-script recovery.

The scene's camera dimensions and draw target dimensions remain distinct. The
scheduler receives the render eye for portal facing and a separately bound player
position for ghost overlays. Prior scheduler source variants and tests are frozen
in the pickup evidence; no byte-match evidence is transferred.

The 163-TU source target passes 3,399,450 real-data / 2,906,962 clean checks,
185 focused real-data checks and 80 absent-module link controls. A native fixture
constructs actual TEST.LEV placements 0/5797, matches portals, resolves the real
BSP model and textures, and renders through the owner, scheduler and consumers.
Cache and gate reacquisition images match; changing rooms changes pixels;
disabling world drawing yields a blank control. A separate unit fixture verifies
full-target clip restoration with an 800x600 target and 641x481 camera.

Run python tools/m0.py build --archive GAME_DATA/Bhunter.bin for integration,
python tools/check_m0_clean.py --evidence-dir META/m4/scene-owner/clean for a
source-only checkout, and python tools/check_scene_owner.py for the saved audit.
Original assets remain immutable. No original code was loaded or executed and
no game was launched. Native fixtures do not establish full-level gameplay,
block-script settings, x87 PC24 fidelity, or software rasterizer support.

Next source frontier: callers 0041c860/00428f50 and actual player/camera/resource
initialization. Full world/startup and non-model registries remain incomplete.
