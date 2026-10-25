# HUD, room attachments and weapon overlays — 2026-10-25

The user authorized another 90 minutes of source restoration with Sol medium implementation and Sol low independent review, then directed stopping at exactly 300 admitted functions. The block started 09:26:47 UTC and its deadline is 10:56:47 UTC. No game/startup/original-code execution, automated gameplay or native input capture/recenter was performed.

## Admitted source

The current frozen checkpoint adds 33 complete original owners, taking the harness to 300/727 active original functions in 288 real translation units. These are functional source implementations, not byte-match claims. Every listed owner compiles in its real source unit, links with real definitions into `build/m0/bhunter-integration.exe` and passes affected source-built checks.

| Family | Complete original owners | Source |
|---|---|---|
| Room/portal radar recursion and reset |00417f30,00417f70|`src/bh_attachment.c`|
| HUD setup and actor markers |0041b2d0,0041bbf0|`src/bh_hud.c`, `src/bh_hud_markers.c`|
| World and flat status |00418f00,0041a3d0|`src/bh_hud_world.c`, `src/bh_hud_flat.c`|
| World-plane full-image quads |00456d30,00477710|`src/bh_plane_quad.c`|
| Scoreboard text and placement |004275c0,00427710|`src/bh_hud_scores.c`|
| Arc producer and weapon HUD |00432050,00432ff0|`src/bh_weapon_hud_arc.c`|
| Target reticle weapon HUD |004314f0|`src/bh_weapon_hud_target.c`|
| Adaptive path append/select/consume |00455ee0,00477880,004779e0|`src/bh_path_queue.c`|
| Two-point segment append/consume |00455e40,004777c0|`src/bh_segment_queue.c`|
| Queued fade, setter and complete display fade |00478930,00477f20,0045cf30,0045cf50|`src/bh_fade.c`|
| End-frame presentation |0045d690|`src/bh_present.c`|
| Shared weapon delay and ready/reset |004309b0,00431050|`src/bh_weapon_common.c`|
| Target weapon ready/cancel/tracking |004310c0,00431130,00431180|`src/bh_weapon_target_lifecycle.c`|
| Arc weapon activate/deactivate/tracking |004324a0,00432510,00432c60|`src/bh_weapon_arc_lifecycle.c`|
| Weapon target-removal query |00433e50|`src/bh_weapon_removal.c`|
| Weapon-index3 float timers |00433170|`src/bh_weapon_latch.c`|

“Attachment” is the existing world-room/six-portal graph, not an invented model hierarchy. The source borrows actual world/navigation definitions. Compact HUD/player/weapon views bind observed fields without claiming a recovered complete entity ABI or constructors. Actual ten-weapon dispatch has only two nonempty HUD methods: arc index 4 and target index 5. The remaining eight use the verified RET sentinel and add no recovery count. Verified CRT rand, ftol, pow, sprintf and allocation remain library reuse.

The two shared weapon callbacks use actual entity-owned state: embedded live target/charge at +f8/+fc, ammo at +10c and delay at +114. They preserve signed delay wrapping, reset-before-random order and exactly one real CRT rand call before requesting queued sound. An allocator mutation test verifies original live player position/velocity pointers. Actual PCM upload and queued voices execute without Play. The field accessor and fade-table disposal are source adapters with no original-function count. Constructor, method identity registry and gameplay writer lifetimes remain open.

The existing 00455f70/004569b0 ribbon source has a separately recorded corrected variant: duplicate points preserve ignored zero-normalization and the original unordered miter clamp. The actual masked x87 invalid/divide sticky effects remain observable. Prior source/header and ledger records are preserved in `prior-ribbon/`; their old tests and byte evidence are not transferred. These two variants add no new original-function count.

## Validation

The real-archive integration passes 3,844,710 checks; the clean asset-free checkout passes 3,351,626. Both use identical source hashes and 138 missing-module controls reject undefined owners with LNK2001. The clean checkout excludes the original assets and compiler; the pinned local compiler/SDK dependencies remain external. Windows command-length overflow in the larger clean link was corrected using VC5 LINK response files with the expanded argv and response-file hash recorded. No compiler flags were changed to obtain a match.

`hud-native`, `weapon-hud-native`, `fade-native` and `present-native` use actual offscreen DirectDraw/Direct3D2 surfaces and recovered rendering owners. Real radar, status, icons, font, target and arc assets are decoded through the source archive/image path. Known procedural colors, blank controls, seeded repeat/change controls and readback assertions accompany actual assets. HUD fixture residency required preallocation after loading the later 128px sheets; only the fixture binding changed. The real HUD 102 and weapon 181 checks include actual assets; the clean checks use independent procedural resources.

Presentation adds 49 actual surface checks: ordered fade gating including masked unordered comparison, native EndScene, software Unlock order, optional BltFast and error continuation. Lost-surface Restore is implemented and statically checked; actual lost surfaces were not induced.

Fade endpoints are exact full-target blue/red controls. Half-fade readback is 2048 blue/2048 purple pixels on this legacy host; its center blend assertion does not establish uniform fractional blending or original-runtime visual equivalence. This observation is recorded in `fade/NOTES.md` and `review/FINAL_NATIVE_REVIEW.json`. A bounded offscreen diagnostic rejected STIPPLEDALPHA and DITHERENABLE as causes of the checkerboard; production state remains unchanged.

`owners.json` pins complete reviewed original bodies and classification; `original/*.tsv` is checked byte-for-byte against immutable PE data. `validation.json`, `integration-real.json`, `link-map.txt`, `clean/clean-checkout.json`, `clean/link-map.txt` and exact `sources/` snapshots bind implementation variants to linked checks. Global per-implementation evidence is in `META/function-status.json`. Reproduce the final audit with `python tools/check_hud_attachment.py`; build with `python tools/m0.py build --archive GAME_DATA/Bhunter.bin` and `python tools/check_m0_clean.py --evidence-dir META/m4/hud-attachment/clean`. Source checks do not execute original code.

## Remaining boundary

`NEXT.md` identifies complete player scene callback 0041c860 (3934 bytes/1162 instructions), its real direct/indirect dependencies, mutation order, 32-particle and four-segment RNG contracts, resources and borrowed object lifetimes. Neither it nor flyby callback 00428f50 is admitted here. Player/camera/weapon construction, startup bindings, software raster dispatch, render worker orchestration and live mission ownership remain. PC24 numerical behavior, CD audio and DirectPlay requirements remain unresolved. M3 is skipped by user direction, not passed. M4 is incomplete and no playable source game exists.

## Stop at 300

The final eight distinct owners extend actual weapon lifecycle and shared-state behavior. Query/list/allocator mutation, real BSP occlusion with disabled-wall control, retention skipping occlusion, raw mixed-delta thresholds, zero-distance unordered cone acceptance, sound handle/latch ordering, charge wrap and timer negative/zero/NaN/subnormal behavior are checked. Actual sound buffers queue voices without Play. The target-aim accessor is an explicit source field adapter, not another original function.

A candidate at 00433f70 was rejected by the count audit because bh_portal_center was already admitted in src/bh_route_query.c. Its duplicate source/tests were removed from integration; candidate evidence is preserved under portal-center/ and adds no function count. The final replacement is the distinct 00433170 timer owner. The exporter now rejects conflicting previously admitted assignments. Exactly 300 unique active linked original functions pass the final audit. Work stops here at the user's request; no further owners are assigned.
