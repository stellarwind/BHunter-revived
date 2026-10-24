# M2 entity pool and intrusive list work block

Started 2026-10-24. Complete reviewed bodies and classification are in `entities/review.json` and `entities/original/`. `00476d50` is a DirectPlay COM wrapper and is excluded. Seven pool/dispatch functions are the implementation family; CRT allocation and the verified RET sentinel are not gameplay recovery assignments.

Original slots are 864 bytes. Real initial arena is 9,600 slots. Growth, generation, selective initialization, intrusive active/free/query membership and dispatch order are observable contracts. Source must own all chunks and call real supplied callbacks; no fake game callback bodies or guessed globals/vtables are permitted. Shared RET is represented as an explicit inactive callback state, not a separately recovered function.

## Complete recovered family

| Original entry | Source interface | Behavior |
|---|---|---|
| `00476da0` | `bh_entities_init` | allocate/zero initial contiguous arena; build free chain |
| `00476f20` | `bh_entity_acquire` | pop or grow, link active head, initialize only observed fields |
| `00477070` | `bh_entity_release` | unlink active/query membership and recycle |
| `00476e90` | `bh_entity_set_query` | registration insertion, replacement and removal |
| `004770f0` | `bh_entities_query` | mask filtering, optional spatial filter and event-3 position update |
| `004772f0` | `bh_entities_step` | active traversal or focused single-slot step |
| `00477330` | `bh_entities_refresh` | active traversal with zero second argument |

All seven complete bodies are implemented as scoped functional variants in the real `src/bh_entities.c` translation unit. Source introduces explicit pool context/allocator/origin/focus instead of guessing original globals. Slot size and relevant offsets compile-check against the original x86 layout. No binary drop-in entry ABI or historical engine authorship is claimed. `00476d50` calls a DirectPlay COM slot and is excluded from this family; networking requirements are not removed.

The original shared RET at `0047d680` means an inactive callback. Source represents that state as NULL and conditionally suppresses the call. There is no empty gameplay callback body, original code pointer or fake vtable. Actual callbacks are supplied by callers. The test TU's tracing, packet mutation and self-release callbacks are isolated contract fixtures, not recovered gameplay or shipping definitions.

## Allocation and lifetime

Initialization clears all initial 864-byte slots and links them in ascending address order. Original bootstrap requests 9,600 slots. The free chain returns slots in that order, incrementing generation at offset 4. Exhaustion allocates `floor(capacity/8)+8` slots, queues all but the last in reverse address order and immediately returns the last with generation zero. Growth slots receive selective initialization; their flags/opaque payload remain unspecified until the caller initializes game-owned fields. Recycled payload and flags are retained. Release increments generation again and makes that slot the next reusable head. Generation is unsigned 32-bit and wraps as the original INC does.

Each acquisition resets refresh/step/query slots to inactive, query links to zero, lifetime/scale to -1/+1, kind to zero and origin to the current explicit owner vector. These are observed fields only; the module does not blanket-clear recycled/growth gameplay state. Source owns every initial/growth allocation in a separate chunk chain with a live bitmap; source bookkeeping is not inserted into the recovered slot layout. Default allocation uses the real CRT. Paired custom allocators support deterministic memory-failure tests.

Active/free and query lists are distinct. Query callback replacement retains registration position. Removal/re-enabling uses consistent backlinks; the original leaves some stale free/query backlinks that can corrupt later reuse. Repairing those unused/stale-link cases is an explicit safety extension. Foreign/interior/double releases, zero/oversize arenas and growth beyond 65,535 slots are rejected. Allocation failure leaves existing slots and counters intact. Disposal frees arena chunks only, invokes no guessed game destructors, and rejects active dispatch. Callers must release any external resources they attached before disposing their pool.

## Dispatch and mutation contracts

Active step/refresh iteration is newest-first and caches the active successor before invoking the callback. Self-release is supported; newly acquired heads wait until a later pass. Step focus is explicit; releasing the focused slot clears focus safely. Refresh passes zero as the second argument. Changing a cached successor's lifetime during a callback stops traversal with `MUTATION` rather than following a recycled/new lifetime. This is a safety extension over the original raw-pointer walk. Nested queries are allowed to depth 16; nested step/refresh and callback-time disposal are rejected. Pool/list/allocator fields are engine-owned and must not be edited by game callbacks.

The query function's surprising order is preserved: it starts at the query-registration head but follows the active chain at offset `0xe0`, not the query-registration next link at `0xd4`. Inactive callbacks encountered on that walk correspond to original RET calls and are skipped explicitly. Registration order can therefore limit which active slots are visited; tests distinguish this from a conventional query-list traversal.

Radius <= 0 or unordered uses the original nonspatial branch. Positive radius requires a center and the verified PC53 profile. Six axis bounds are stored as float32. A flag-0x100 entity first receives `{event=3, position}`; the modified packet position is written back before filtering. AABB and spherical boundaries are inclusive. Distance uses the inspected mixed expression `(dx*binary32(dx)+dy*dy)+dz*binary32(dz)` followed by SQRT at PC53, with no intervening squared-radius float store. This differs from the BSP bounds contract. Real separate-TU round stores preserve observed widths. Spatial callbacks must preserve that numerical profile; driver/thread transitions remain unverified.

`tests/entity_query_cases.h` has 768 independently generated Python controls at the preceding/nearest/following float radius around 256 varied distances. The checker reproduces the exact data and reviewed boundaries. Analytical 3-4-5, event-3 writeback, NaN-radius branch and precision rejection cases supplement them. No original entity function is executed and no byte match or universal numerical proof is claimed.

## Executed integration

- Source-only entity scenario: **819 checks**, covering initial/growth order, generation and selective payload retention, intrusive membership, callback replacement/removal, actual traversal order, focused step, self/future mutation, new-head timing, bounded nesting, foreign/double/interior release, allocation failure and maximum 65,535 occupancy.
- The real-data scenario decodes TEST.LEV with the source reader, acquires/releases **5,815 slots** in the observed **9,600-slot arena**, and verifies complete active/free ownership. It copies placement positions/flags for this ownership exercise; it does not install gameplay callbacks, instantiate the original full level loader or execute scripts.
- All **19 real translation units** compile/link in `build/m0/bhunter-integration.exe`. The combined real-data build passes **349,767 checks**; the entity scenario contributes **12,454**. All previous foundation/asset tests and nine omitted-module LNK2001 controls pass.
- `entities-clean/clean-checkout.json` records **33,807 checks** from a clean source checkout with spaces and a separate launch cwd, without original game files/compiler binaries in the checkout. Source hashes are compared with the real-data build in `entities/validation.json`. `entities/integration-real.json` and `entities/link-map.txt` preserve commands, owning definitions and results.
- `python tools/check_entities_evidence.py` verifies the independent controls and complete boundary catalog. All four original reference hashes remain unchanged. The math comparison setup and original-code mapper are unchanged.

## M2 exit audit

The pool/list allocation contract is now admitted to the harness with seven full functional variants and three separately counted lifecycle adapters. Game callbacks and full level/navigation/script owners remain unadmitted. `EXIT_AUDIT.md` evaluates the full M2 requirements rather than treating source counts as completion. Bounded PCX image and WAV sample readers required by the first slice remain the next foundation gap. The M2 goal stays active; CD music, DirectPlay and original runtime scenarios remain tracked for later gates.
