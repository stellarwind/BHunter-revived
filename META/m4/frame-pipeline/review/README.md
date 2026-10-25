# Independent frame boundary review

Static audit only; no original code or game execution. `BOUNDARY_REVIEW.json` records complete boundaries, roles, registrations, queue lifetime and unresolved contracts. Raw instruction arrays retain exactly155 and47 instructions with their terminal RETs.

00471e00 is the full renderer/platform worker. The three-slot selection, sentinel bypass, busy release before display wait/Flip, conditional restoration block and bottom-only shutdown check are material lifetime contracts. The no-valid-slot case exposes an original invariant/hazard, not permission to invent another selection policy.

00479140 is a fixed-step callback with an acknowledged shutdown path. Calling it only a render producer loses the complete cleanup branch and simulation dependency. Its15 direct calls are not15 missing source functions: classify existing admitted owners, CRT and actual missing owners separately. Historical provenance remains unknown where indicated by the role catalog.

Neither target was in the active implementation ledger when reviewed. This audit claims no new admission, compilation or execution. Source variants require separate linked checks; a safe bounded worker API may explicitly differ from the original unbounded thread entry, but must not silently transfer equivalence claims.

Raw reference check: python META/m4/frame-pipeline/review/check_boundaries.py passes immutable EXE hash, all202 instructions/665 body bytes, filename format and rb mode. The source-added arbitrary dump format issue was corrected to bounded dump_prefix plus fixed format by its owning agent.
