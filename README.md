# BHunter Revived

Restoring BHunter as a faithfully playable, source-built game on modern Windows.
**Source preview available:** run `build-source.cmd`, then `play-source.cmd` for a
manual playtest. Startup/static rendering passes; human playtesting is in progress.
M3 was skipped by user direction; M4 source restoration remains incomplete.
Current preview evidence: [validation](META/m4/playable/light-sign/validation.json).
The inventory checkpoint below predates the preview ownership corrections.

<!-- BHUNTER_STATUS_START -->

**Source recovery:** `[########################]` **100.0% (679/679)**

| Restored & linked | In progress | Not started |
|---:|---:|---:|
| 679 | 0 | 0 |

891 mapped functions − 212 verified runtime/helpers = 679 potential game/engine functions. Classification remains provisional.

**Playable game target:** 0/679 functions admitted. Function count is not effort or gameplay completion.

**Latest validated block:** 23 complete owners. [Block evidence](META/m4/finish/README.md).

**Last validated builds:** 601 translation units · 5,529,939 real-asset checks · 5,023,991 clean checks · 295 missing-module controls.

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
