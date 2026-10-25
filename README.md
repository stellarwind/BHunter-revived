# BHunter Revived

Restoring BHunter as a faithfully playable, source-built game on modern Windows.
**Not playable yet.** M3 was skipped by user direction; M4 source restoration remains incomplete.

<!-- BHUNTER_STATUS_START -->

**Source recovery:** `[############------------]` **50.2% (365/727)**

| Restored & linked | In progress | Not started |
|---:|---:|---:|
| 365 | 6 | 354 |

887 mapped functions − 160 verified runtime/helpers = 727 potential game/engine functions. Classification remains provisional.

**Playable game target:** 0/727 functions admitted. Function count is not effort or gameplay completion.

**Latest validated block:** 1 complete owner. [Block evidence](META/m4/pickup-basic/README.md).

**Last validated builds:** 358 translation units · 3,871,035 real-asset checks · 3,365,207 clean checks · 172 missing-module controls.

<!-- BHUNTER_STATUS_END -->

[Progress & blockers](META/PROGRESS.md) · [Build](BUILDING.md) ·
[Restoration plan](RESTORATION_PLAN.md) · [Function ledger](META/function-status.json)

Run `MKSTATUS.cmd` (or `push.cmd`) to refresh this status, commit **all non-ignored changes** (including staged files, new files and deletions), and push the current branch to its configured remote. An optional commit message can be supplied:

```bat
MKSTATUS.cmd "Restore HUD and weapon lifecycle"
MKSTATUS.cmd --dry-run
```

The dry run changes nothing. A normal run pushes existing local commits even when there is nothing new to commit. Errors stop the command and keep local changes/commits available for retry; it never force-pushes or automatically merges/rebases. Status includes the last recorded successful checkpoint; this command does not rerun the game or builds.

Use `MKSTATUS.cmd --check` to check the README without committing or pushing. To refresh it locally only, run `python tools/mkstatus.py`.
