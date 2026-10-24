# BHunter Revived

Restoring BHunter as a faithfully playable, source-built game on modern Windows.
**Not playable yet.** M4 source restoration is active; M3 was skipped by user direction.

<!-- BHUNTER_STATUS_START -->

**Source recovery:** `[######------------------]` **25.0% (182/727)**

| Restored & linked | In progress | Not started |
|---:|---:|---:|
| 182 | 7 | 538 |

887 mapped functions − 160 verified runtime/helpers = 727 potential game/engine functions. Classification remains provisional.

**Playable game target:** 0/727 functions admitted. Function count is not effort or gameplay completion.

<!-- BHUNTER_STATUS_END -->

**Latest iteration · 2026-10-24:** outer scene orchestration and world/resource
bindings now drive real two-room native rendering, including portal traversal,
cached-room changes and clip resets. Pickup construction, lifetime and drawing
are linked with the scene scheduler.

**Validation:** 163 translation units (production and test source files) ·
3,399,450 real-asset checks · 2,906,962 clean source checks ·
80 missing-module link controls. No game launches.

**Next:** scene callers and player/camera/resource initialization toward a playable
slice. Full startup, block-script bindings and software rendering remain open.

[Progress & blockers](META/PROGRESS.md) · [Build](BUILDING.md) ·
[Restoration plan](RESTORATION_PLAN.md) · [Function ledger](META/function-status.json)

Refresh statistics with `mkstatus.cmd`.
