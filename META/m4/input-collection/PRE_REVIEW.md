# Collector boundary and constants pre-review

2026-10-25. Static support only; no OS input calls, game execution or original
code execution. Before decompilation, retained function map/call edges classify
004744f0 as complete engine input collection owner, attribution unresolved.
It polls two WinMM joystick IDs and User32 cursor, updates key/axis state,
dispatches real input-list evaluation00474020 and snapshots previous keys.
Ordinary bounded entry004744f0 and RET00475634 enclose4421bytes and
1107instructions. No CRT/library signature establishes reusable provenance;
it is not an import, compiler helper or shared tail. Its imported joyGetPosEx,
GetCursorPos and SetCursorPos are platform dependencies, not recovery targets.
The actual default loop00479140 calls it at0047918c; the collector calls
00474020 at004755f8. No new gameplay or loop recovery is claimed.

`verify_static.py` verifies every contiguous instruction against the pinned
immutable EXE, writes raw TSV/JSON and owner hash. Body SHA256:
93380cfa1ea024de9192d9fe8cd9c0350d6c5699402f0e1141e80bf60f4f7f0a.

## Widths and values

`constants.json` records each literal's actual instruction operand width, raw
bytes, decoded value and every referencing instruction. All referenced
0048a000..0048a050 literals are eight-byte doubles:32768,0,5000,-5000,
0.00004,-1,+1,1/30000,0.5,-0.002,0.99, at eight-byte increments in that
order. The nearby00489ff0/ff4/ff8 literals are four-byte float0,+1,-1.
Decompiler casts in the mouse filter must not narrow -0.002 or0.99: actual
instructions00475515/1d and0047556b/73 consume doubles. Lower clamp compares
retained result after FST (not FSTP); upper clamp reloads stored binary32.

## Snapshot pair ownership

Original00474020 instructions establish distinct packet cases:

| Source field | Original addresses | Evidence |
|---|---|---|
| pair3[0/1] | ushort004eed88/004eed8a |004742ca/004742d4 |
| pair4[0/1] | low16 of long004eed80/004eed84 |004742f7/00474301 |
| mouse_axes[0/1] | float004eed8c/004eed90 | evaluator/collector float loads |

pair3 is accumulated16-bit position. Collector ADD AX operations at004754c3
and004754f0 intentionally truncate each signed cursor delta to low16 and add
with16-bit wrap, then compare unsigned against0x3ff. pair4 is per-frame raw
delta low words. Its fields are four bytes apart in original memory; copying
the contiguous four bytes at004eed80 would be wrong. Source snapshot's packed
pair4 must be filled separately from each32-bit delta. Mouse filter uses the
full signed32-bit deltas through FILD dword0047550f/00475565.

This pre-review does not admit a collector implementation or establish platform
callback wiring, OS success behavior, interactive input or gameplay fidelity.
