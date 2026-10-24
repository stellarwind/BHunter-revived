# M2 config/profile container and path work block

Work block completed 2026-10-23. Review and dependencies are recorded in `config/review.json`, with complete original bodies/listings in `config/original/`. Four functional variants and five source support adapters compile/link/run in the named integration target. This is harness admission; full M2 and source-game admission remain open.

The chosen container is the config-owned circular doubly linked profile list, not a generic guessed engine collection. Header is 416 bytes, profile record 76 bytes: flag word at 0, next/previous pointers at 4/8, opaque name/binding bytes at 12..71 and baseline pointer at 72. Three immutable default profiles and header are captured from reference data. Custom flag-zero records own a separately allocated baseline with links/baseline cleared. Real CRT is reused; no allocation/runtime scaffolding is a recovery assignment.

Original config entries are zero-argument global-state operations. Source uses explicit owning context, path and enumerated-device data. Native path discovery uses real Win32 APIs, but tests write only isolated local fixtures. Renderer enumeration and registry/CD source validation are outside this family; no INI parser is invented because the inspected path helper only constructs the INI name. Math mapper remains unchanged.

## Reviewed functions and interfaces

| Original entry / full body | Role | Source behavior |
|---|---|---|
| `00401fd0..004022ca` | game config | `bh_config_load`: settings and profile ring, missing-file/header-only defaults |
| `004022d0..00402366` | game config | `bh_config_bootstrap`: load, reconcile device name, save temporary zero hardware word, restore runtime word |
| `00402370..00402460` | game config | `bh_config_save`: settings then 76-byte records in forward ring order |
| `00473910..00473a36` | engine candidate, authorship unknown | `bh_paths_init`: Windows INI path and current-directory decoded archive name, success return 2 |

All four have contiguous reviewed boundaries, plain RET and no shared tails assigned independently. Because they have zero original parameters, RET alone does not distinguish cdecl from stdcall. Source exports explicitly cdecl APIs with context/path/directory/device inputs. These are functional reconstructions with interface adaptation, not drop-in binary ABI wrappers. `config/review.json` supplements the historical M1 catalog without rewriting its initial archive assignment. Unknown engine attribution remains unknown; game-specific CFG operations are separate from the reused CRT.

Direct dependencies are classified in `config/dependency-validation.json`: real CRT allocation, stdio and case-insensitive comparison; Win32 directory imports; and these family members. `python tools/check_config_evidence.py` verifies that closure, complete boundaries, pinned EXE hash and four reference-data captures. It reconstructs the expected default-data header for comparison without modifying it. No original code executes in that checker or the integration target.

## Settings, circular list and ownership contract

`BHConfig.settings` retains all 416 original bytes. The hardware word at offset 0 and device-name buffer at 12..139 have direct consumer evidence; other fields remain opaque. The next named settings area starts at original `00496ac4` (offset 140). Each profile has a 16-byte name at record offset 12 followed by 44 opaque binding bytes. The x86 source profile layout is compile checked: 76-byte size, next/previous at 4/8 and baseline at 72. The wrapper's allocation chain and owned-baseline pointer are source support fields outside that recovered record.

A missing file supplies the exact default settings and three ordered profiles: `<Standard>`, `<MouseView>`, `<Joy 1 View>`. Header-only files retain their settings and supply the same three profiles. Defaults reference immutable source template records. Loaded flag-zero profiles own independent 76-byte baseline snapshots with next/previous/baseline fields cleared. Other loaded flags retain their word/payload but discard the stale serialized baseline pointer. Every disk next/previous pointer is overwritten with newly built links. A singleton points both ways to itself; multi-node lists close in file order with reciprocal links.

Initialization accepts a paired allocator/release contract; production defaults call real malloc/free. Tests use a counting allocator that still allocates real memory and can fail a chosen allocation. Reload constructs an owned candidate before replacing the previous state; all failures preserve prior settings/list and release the candidate. Disposal follows a separate allocation chain, independent of mutable profile flags or ring links, and releases custom snapshots exactly once. Context private ownership fields and allocator lifetime must remain intact. Arbitrary foreign contexts/private-chain corruption are outside the API contract.

Saving validates owned membership, count, circular reciprocity and bounded names before opening the file. It retains the original layout: 416 settings bytes followed by complete 76-byte live profile records in forward order. As in the original, those records contain current 32-bit process pointer words. Their values are not portable identifiers and are not dereferenced on reload. No new versioned format is substituted. Saving uses the original direct-write style and reports write/close failures; it does not promise atomic replacement after a partial I/O failure.

Bootstrap consumes actual enumerated device names as explicit bounded data; it does not fabricate renderer globals or enumeration. Case-insensitive comparison keeps the last match. No match selects the first device and clears the hardware word. It copies the chosen spelling, writes with the hardware word temporarily zero, then restores the runtime value even if saving fails. An empty device list is diagnosed instead of indexing invalid renderer state. Full settings meaning, input binding consumption and renderer enumeration still need their owning families.

## Path contract

`BHPaths` is initialized by the caller. The Windows INI filename is `BHUNTER.INI`; the archive name is recovered by decrementing the bytes of original `CIVOUFS/CJO` to obtain `BHUNTER.BIN`. A trailing backslash suppresses the extra separator; a slash does not. Paths retain case, spaces, relative components and the observed separator rule. A failed Windows-directory query leaves an existing INI path unchanged. The config helper constructs `BHUNTER.CFG` under a supplied Windows directory, or the literal relative filename if discovery failed.

Native `bh_paths_discover` calls real GetWindowsDirectoryA/GetCurrentDirectoryA without changing cwd. Source bounds both output fields to 256 bytes, rejects empty/oversize inputs and treats a failed current-directory query as failure with unchanged output. Config persistence accepts an explicit path, so tests and source integration write only private build fixtures; no Windows-directory or registry settings are changed. Constructing an INI name is not evidence of an INI parser: none is invented here. Registry SourceDir/copyright/CD verification in `00473a40` remains outside this family.

## Safety differences and evidence limits

Original paths use unchecked formatting and can continue with uninitialized output on API failure. Original config reads trust size arithmetic, partial reads and serialized addresses; failure/allocation/cleanup paths can crash or leak baseline storage. Source keeps valid layout/order/default/device behavior while rejecting truncated headers, trailing partial records, unterminated bounded names and counts above 4,096. Open errors other than missing-file are reported rather than silently defaulting. Pointer values from disk never become trusted references. Nonzero-flag baseline pointers are cleared. Rollback and ownership cleanup are explicit safety extensions.

The reviewed legacy format has no identified version marker. Structural validation is implemented; an unknown incompatible file that happens to satisfy this shape cannot be called version-verified. Tests use captured original default data and independently constructed legacy CFG files; no actual user-produced CFG was supplied and no original config/path code was run. There is no byte-match claim, original-backed config adapter, general binary loader or math-mapper extension. Modern 64-bit pointer layout and alternate serialization remain separate work.

## Executed checks and integration

- New `config-container-path` scenario: **2,508 checks, zero failures**. It covers original defaults, opaque byte preservation, disk pointer rebuilding, snapshot isolation, forward/backward/singleton/max-size rings, malformed names and every partial length through the three-record fixture, default/custom reloads, each node/snapshot allocation failure, late parse rollback, readonly save failure and mutable-link/flag disposal.
- Device tests check duplicate case-insensitive matches, first-device fallback, saved zero versus restored runtime hardware state, invalid enumeration and restoration after persistence failure. Path tests cover all input lengths 1..256, exact capacities, roots, trailing separators, spaces via clean checkout, relative/slash behavior, API-failure contracts and execution of native read-only discovery.
- A composed scenario constructs a private startup archive path, mounts a synthetic archive through the recovered VFILE module, reads `M2OK`, closes/shuts down, then writes/reloads the source config at the constructed CFG path. These fixtures remain under the build directory and are cleaned after execution.
- `build.cmd --archive GAME_DATA/Bhunter.bin` compiles all eleven real translation units and links `build/m0/bhunter-integration.exe`. **51,285 checks pass**, including the unchanged numerical suite and all 1,229 real archive payload comparisons. Separate missing math, VFILE, config and paths modules each fail linking with LNK2001. `config/integration-real.json` and `config/link-map.txt` preserve the source hashes, commands, resolved ownership and results.
- `config-clean/clean-checkout.json` records the same source build from an isolated checkout with spaces and a different launch directory, without original assets or compiler binaries inside the checkout. **31,636 combined checks pass**; `config/validation.json` checks identical source hashes between that build and the real-data run.

## Next gate

The profile container/path/config family is integrated. General engine/entity pools are not thereby recovered. The next useful M2 block is a reviewed bounded real-asset reader family needed by the `test.lev` / `test.pth` scene dependency path, with malformed-input checks and owning definitions. First review its complete boundaries, roles and dependencies rather than assigning all unknown loader code. Original runtime scenarios/precision transitions, renderer/input consumers, CD audio and DirectPlay remain tracked. Full M2 stays open until the required foundation readers compose; no playable game or source-game admission is claimed.
