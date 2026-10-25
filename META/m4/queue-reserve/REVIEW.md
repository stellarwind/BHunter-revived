# Queue reservation and queued render-mode callback

2026-10-25. Complete source owners00477530 and00477360 are in new
`src/bh_queue_reserve.c/.h`. Both compiled in the actual TU, linked against real
definitions and passed65 focused source/native checks. No existing source,
tests, checkpoint or shared build files were edited for this family. No game
launch or original-code execution occurred.

`PRE_REVIEW.md` records classification and contracts before implementation.
Both owners are engine candidates, authorship unknown, with no verified CRT,
compiler/EH/import or independent shared-tail classification. Complete pinned
bodies are00477530–004775ca (155 bytes/50 instructions) and
00477360–00477375 (22/9). The36-byte jump table at004775cc is separate data and
is not counted as code or another function. Every body/table byte is checked
against the immutable PE by `verify_evidence.py`.

The actual jump-table entries give header mapping1,2,3,4,5,0,6,7,8. An unsigned
JA excludes values above8, including negative signed representations; these
all use default header0. The source API takes a signed32 selector but compares
its unsigned representation, preserving that behavior. Reservation links the
new command after the existing tail, retains its prior successor and publishes
the new header tail. It writes **only** the new next word and existing linkage;
kind, resource and all18 payload words remain untouched. Source guards prevent
malformed count/binding/topology writes. Full queues preserve count and every
command byte, reporting BH_DRAW_FULL separately. The existing source queue is
bounded to65535 slots, so its count/capacity representation retains the original
signed comparison within that valid domain.

The complete consumer loads original command+08 (`words[0]`), invokes actual
admitted `bh_render_mode`0045b500, then reads the command's current next word.
It publishes next even when the source owner reports failure. It accepts the
same callback payload directly without inventing another executable function
pointer or vtable. Context/command/next may not overlap. The source convenience
wrapper `bh_queue_mode` populates existing BH_DRAW_STATE and that mode word;
it is an adapter, not a third original function. No enum change was needed.

`check_standalone.py` compiles the real source/test TUs, links named
`build/queue-reserve/queue_reserve.exe` against real source objects and normal
CRT/system/native libraries, and executes65 checks. Fixtures cover all nine
selectors plus negative/out-of-range inputs, untouched80-byte payload prefix,
FIFO linkage, full no-mutation, malformed source topology/count, wrapper's two
stores, all five actual software-mode combinations, unknown mode, missing
bindings and failure/link forwarding. Native composition stages a real white
texture and pending red triangle fan; the queued consumer executes the actual
residency/flush/EndScene/BeginScene/mode switch. Native epoch/run counts, actual
device blend state, red interior pixel and blank exterior pixel are verified.

Parent integration requires `src/bh_queue_reserve.c`, `tests/queue_reserve.c`,
SDK flags, checkpoint `bh_queue_reserve_checkpoint`, scenario `queue-reserve`,
and map symbols `_bh_queue_reserve`, `_bh_queue_mode_execute`. Parent integration
and clean-build evidence remain separate gates. No full game/worker admission
or instruction matching is claimed.
