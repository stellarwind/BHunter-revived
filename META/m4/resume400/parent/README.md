# Editor input setup and packet dispatch

Complete00412190 and004100b0 implement editor control setup and its installed
per-step packet consumer. Classification and closure were retained in the prior
`loading-display/next-review/WORLD_INPUT_CONTRACT.md` before implementation.
These are editor application functions, not player controls, CRT or compiler
support. Source startup004474c0 references this editor setup; full world startup
and editor update/display owners remain unrecovered. No player/gameplay admission
is inferred. `evidence.py` checks both complete instruction streams against the
immutable PE and original inventory, excluding alignment padding.

Real dependencies are restored input-list allocation00475640, key writer00475770,
kind4 writer004757e0, entity acquisition00476f20 and packet application00475870.
No callbacks or vtables are fabricated. Real source entity dispatch invokes the
installed actual callback. Original editor globals are explicit borrowed source
context/list/state/name. The private entity payload+420 context pointer is an
adapter and adds no count. Valid contexts/list/state/pool outlive callback use;
caller releases dispatcher before list/context teardown. The tick reloads the
current context list, preserving original live-global behavior.

All29 slots, exact28 virtual key values, Ctrl-C/V modifiers, untouched binding
bytes, mode1 flag2 and kind4 atslot8 are retained. The original shared name buffer
is empty at initial memory; the context exposes its real live contents rather
than inventing descriptions. NULL name defaults to empty. The source initializer
rejects repeated setup and invalid owners. List allocation failure returns0;
entity allocation failure retains the already constructed list and returns1,
with no dispatcher. Original failure assumptions are documented as source safety
differences, not silent successful placeholders.

Fixtures exercise real allocation, every binding key/tag/modifier and selected
preserved bytes, synthetic snapshot
evaluation, actual installed entity callback and packet application, modifier
gating, key release, raw kind4 payload and live list replacement. Actual allocator
failures distinguish list failure and pool growth failure; owning teardown leaves
no allocations. No polling, physical input, cursor APIs, game/original code,
UI capture, audio or network execution is used. Full integration/clean-build
evidence is required at the parent checkpoint before admission.
