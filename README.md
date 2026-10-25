# BHunter Revived

Restoring BHunter as a faithfully playable, source-built game on modern Windows.
**Not playable yet.** M4 source restoration is active; M3 was skipped by user direction.

<!-- BHUNTER_STATUS_START -->

**Source recovery:** `[#######-----------------]` **32.2% (234/727)**

| Restored & linked | In progress | Not started |
|---:|---:|---:|
| 234 | 7 | 486 |

887 mapped functions − 160 verified runtime/helpers = 727 potential game/engine functions. Classification remains provisional.

**Playable game target:** 0/727 functions admitted. Function count is not effort or gameplay completion.

<!-- BHUNTER_STATUS_END -->

**Latest iteration · 2026-10-25:** 52 complete scene-caller dependencies now include player/input initialization, all camera modes, world collision correction, flyby/route updates, real background resources, atlas text, horizon color tables and collision cache initialization. Both full scene callbacks remain open.

**Validation:** 203 translation units · 3,536,976 real-asset checks · 3,044,022 clean checks · 99 missing-module controls. No game launches.

**Next:** horizon/software rendering and actual caller/constructor dependencies toward a playable slice.

[Progress & blockers](META/PROGRESS.md) · [Build](BUILDING.md) ·
[Restoration plan](RESTORATION_PLAN.md) · [Function ledger](META/function-status.json)

Refresh statistics with `mkstatus.cmd`.
