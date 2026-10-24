# BHunter Revived

Restoring BHunter as a faithfully playable, source-built game on modern Windows.
**Not playable yet.** M4 source restoration is active; M3 was skipped by user direction.

<!-- BHUNTER_STATUS_START -->

**Source recovery:** `[#####-------------------]` **21.3% (155/727)**

| Restored & linked | In progress | Not started |
|---:|---:|---:|
| 155 | 5 | 567 |

887 mapped functions − 160 verified runtime/helpers = 727 potential game/engine functions. Classification remains provisional.

**Playable game target:** 0/727 functions admitted. Function count is not effort or gameplay completion.

<!-- BHUNTER_STATUS_END -->

**Latest iteration · 2026-10-24:** +31 recovered functions. Original camera,
lighting, clipping and lit/unlit BSP traversal now render a textured static city
scene through native Direct3D2: **146 textures · 1,798 polygons · 3,112 triangles**.

**Validation:** 132 translation units · 3,398,563 real-asset checks ·
2,906,329 clean source checks · 66 missing-module link controls. No game launches.

**Next:** world/entity/script orchestration and player controls toward a playable slice.

[Progress & blockers](META/PROGRESS.md) · [Build](BUILDING.md) ·
[Restoration plan](RESTORATION_PLAN.md) · [Function ledger](META/function-status.json)

Refresh statistics with `mkstatus.cmd`.
