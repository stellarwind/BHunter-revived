# BHunter restoration rules

Read `RESTORATION_PLAN.md` and `META/PROGRESS.md` before implementation. Keep per-implementation evidence in `META/function-status.json`. Original EXE/BIN/MDF/MDS are immutable references; never silently patch or replace them.

- Default objective: faithful source-built gameplay, not 100% instruction matching.
- Before decompiling, classify the boundary and role. Exclude verified CRT/libraries, compiler EH/unwind/stack helpers and imports from gameplay recovery assignments. Shared tails must be recovered as part of their owning behavior, not assigned/count as independent gameplay functions. Unknown engine provenance is not proof it is game-owned.
- Every admitted implementation must compile in its real translation unit, link into a named target with real definitions, and execute relevant checks. Objects/listings alone are candidates. No empty functions, constant-success stubs, guessed globals/vtables, or inline asm to silence missing definitions.
- Record provenance, implementation (`not_started/wip/func/bm/reused/adapter`), integration and validation separately. Never transfer byte-match evidence from one source variant to another variant's linked tests.
- Work one function or small dependency family at a time toward the next observable milestone. Run the current integration build and affected scenario before marking a family admitted.
- Matching investigations need a question, positive control and stopping condition; default cap is one 30-minute block unless the user explicitly requests more. Repeated identical outputs mean stop/reassess. Functional equivalents may be accepted with appropriate evidence.
- Preserve the pinned VC5 comparison setup/header context. Toolchain certainty is block-specific; no global flag changes for one near-match. Padscan runs only in snapshots and is optional diagnostic work.
- Do not count disassembly coverage as source completion. No shipping target may load/map/execute original game code; original-backed test adapters must be explicitly recorded and isolated.
- The current math image mapper is not a general game loader. Do not extend it implicitly to run unrecovered game subsystems.
- Retain numerical/timing/lifetime contracts. Track unresolved 24-bit x87 behavior and CD audio/DirectPlay requirements; no silent feature deletions or broad backend rewrite before a faithful slice.
- Update durable evidence/blockers when a block ends. Do not fabricate successful tests or mark milestones complete from source counts. Do not spawn agents unless the user authorizes delegation.
