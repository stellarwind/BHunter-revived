# Complete wait-for query pair: read-only source plan

Whole00443270(221bytes/80instructions) and00442f40(202/63) are ordinary script
event-query application owners. All423 executable bytes/143instructions are
contiguousRET-terminated and PE-verified in `raw-review.json` and the retained
loading-display/next-review evidence. They are not CRT, imports, compiler tails
or a partial VM body. Ghidra presently has no function metadata for either;
honest fresh decompile failures are retained, with full raw still available.
No implementation or analysis-database mutation occurred in this scout.

##State and exact event contracts

Both use a48-byte state at entity+e8. Proposed `BHScriptWaitState` contains
fired/next/actor pointers(offset0/4/8), script_loaded(+c), remaining(+10),
two retained DWORDs(+14/+18), expected event identifier(+1c), signed variable
selectors for remaining/expected(+20/+24) and two retained tail DWORDs(+28/+2c).
All48 bytes copy, including unused/reserved fields; do not zero only a guessed
subset or retype ignored payload as another undocumented feature.

Template00443270:

- Event20 immediately releases only when state.script_loaded==0.
- Event4 schedules actual admitted delete_tick as the entity step.
- Event8 resolves each nonnegative selector using signed-short variables of
  the packet actor, writes resolved template remaining/expected BEFORE trying
  allocation, then acquires a real child. Failure leaves those template updates
  and does not forward activation.
- A successful child copies packet actor position and all48 state bytes,
  replaces its script_loaded from current context script_loading, replaces its
  actor from the activation packet, and installs actual child query00442f40.
- After registration, if template.next exists, its actual query receives the
  ORIGINAL activation packet. Original NULL-query hazard needs an explicit
  source guard, not a dummy successful callback.

Child00442f40:

- Event20 releases only a child not marked script_loaded.
- Event14 compares packet DWORD+8 to saved expected; mismatch returns. On match,
  nonnull fired entity with nonnull query receives a40-byte stack packet whose
  defined fields are event8 and saved actor. Remaining stack payload is original
  unspecified data, not a source-default zero message contract.
- Afterwards a nonzero remaining count decrements with32-bit wrap. Reaching0
  unregisters the actual query (verified RET sentinel becomes NULL under the
  existing entity contract), then schedules admitted delete_tick. A starting0
  is unlimited, not immediate deletion.
- Event4 schedules delete_tick. Event10 does so only if packet.actor equals
  saved actor. These branches retain query registration until deletion; only
  matching completion at count0 explicitly unregisters first.

##Actual source bindings and one concrete linkage blocker

Entity acquire/release/set-query owners are admitted in `bh_entities.c`. The
required delete owner00442eb0 is admitted **static** `delete_tick` in
`bh_script_timer.c:100`; its persistent owner is a real BHScriptTimerContext*
at `entity->payload+52`. Current public header exposes no way to obtain that
actual callback. Duplicating its body or installing a made-up callback would
avoid the existing real definition and is not an acceptable closure.

Root selected the smallest coordinated fix AFTER preceding families: add a
count-zero public getter to `bh_script_timer.c/.h` returning the actual existing
delete_tick function pointer, leaving its body/identity unchanged. For example
`BHEntityStep bh_script_timer_delete_step(void)`. This is exposure of a real
owner, not a new recovered game function or constant-success placeholder.
No getter or admitted file has been edited yet.

All six active records on that TU need current-source variant refresh and new
whole integration evidence; original historical hashes/evidence remain retained:

|ID|Owner|Implementation/type|
|---|---|---|
|00443010-functional-c|template_timer|func / linked_harness|
|00443140-functional-c|template_timer2|func / linked_harness|
|00442cf0-functional-c|active_tick|func / linked_harness|
|00442de0-functional-c|always_tick|func / linked_harness|
|00442eb0-functional-c|delete_tick|func / linked_harness|
|00442ef0-functional-c|running_query|func / linked_harness|

Exact current source/header hashes and record metadata are preserved in
`wait-timer-affected-records.json`. No byte-match evidence may be transferred to
the getter variant. Root owns ledger, source-variant refresh, full real/clean
build and count decisions.

##Proposed new module and fixtures

After the real getter is available, new `bh_script_wait.c/.h` can borrow the
actual BHScriptTimerContext directly: it already supplies pool, persistent
actor-variable table and current script_loading. Its binding writes the actual
48-byte wait state plus that actual timer context pointer at payload+52, with
size/offset assertions; delete_tick therefore resolves its existing expected
owner exactly. A source binding adapter installs real wait-template query and
no fabricated timer/VM callback. Separate wait-state getter is needed because
the admitted timer-state accessor intentionally accepts only its own query
identities. No changes to its existing validation policy are needed.

Proposed APIs `bh_script_wait_bind(BHScriptTimerContext*,BHEntity*,state)`,
`bh_script_wait_template(BHEntity*,void*)`,
`bh_script_wait_child(BHEntity*,void*)`, plus validated state access. Only the
last two map to original owners. Scenario script-wait, checkpoint
`bh_script_wait_checkpoint(unsigned long*)` (no archive required).

Real dynamic callback closure is viable: bind actual admitted timer/timer2
entities with `bh_script_timer_bind`, and use those real template queries as
wait.fired and wait.next. Event8 forwarding then creates an observable real
timer child, not a fake observer callback. Use actual pool, actor table and
signed-short variables for selectors; verify literal and variable resolution,
all48 copied bytes, successful child position/actor/script flag, mutation-before-
allocation-failure, actual next forwarding, expected-ID match/mismatch, unlimited
and finite counts, query-unregister ordering, event10 actor match/mismatch,
event20 script flag and actual shared delete callback on a later pool step.

Keep pool/context/actors and referenced entities alive until all queries and
deletion steps finish. A fired callback can release/recycle the current entity;
match existing timer policy with generation and real active-pool membership
guards before subsequent writes. Source selector bounds may reuse the actual
22-short BHScriptTimerActor table; do not guess raw entity-variable pointers.
No physical input, native audio Play, VM implementation, game/editor execution
or UI capture is needed. The callback exposure is the sole current linkage
blocker; no missing game behavior lies behind the actual dynamic timer queries.
