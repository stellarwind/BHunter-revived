# Input message owner static review

2026-10-25. No game or original-code execution. Before implementation, retained
call-edge evidence identifies00473c40 as an input message processor; role is
engine_candidate, attribution unresolved. It is neither a verified CRT/library
routine, import thunk, compiler helper nor shared tail. Its absence from the
current Ghidra function list was missing analysis metadata, not absence of code.

## Boundary, dispatch data and ABI

`verify_static.py` verifies every instruction byte against immutable
GAME_DATA/BHUNTER.EXE with pinned SHA256. The complete contiguous body is
00473c40..00473d24,229bytes/60instructions. `original/00473c40.tsv` contains
raw instruction evidence and `owner.json` its SHA256 and all return sites.
The three-byte00473d25..27 LEA ECX,[ECX] alignment padding is excluded.
Six-entry keyboard jump table00473d28..3f and seven-entry mouse jump table
00473d40..5b are data, separately verified and recorded in owner.json.
Keyboard indexes0..5 mean messages100..105, with102/103 forwarded. Mouse
indexes0..6 mean202..208, with203/206 forwarded.

**Original ABI is cdecl, not CALLBACK/stdcall.** All nine original returns are
single-byte C3, including default-forward return00473d24; none is C2 1000.
Four original arguments reside at stack+4..10. Startup00479200 pushes its
address at004798bf as a callback argument to00471c00, then clears five caller
arguments at004798e4. This establishes an engine callback registration rather
than proving direct Windows WNDPROC registration. A real Win32 CALLBACK wrapper
is required if this source helper is registered with Windows in a later owner.
Current source explicitly adds snapshot as first argument and uses cdecl;
that explicit binding adapter is consistent with the recovered helper behavior.

## Source contract check

src/bh_input_messages.c covers every original dispatch case. WM_KEYDOWN and
WM_SYSKEYDOWN append low-byte wParam to32-byte history only when lParam bit30
is zero, increment/wrap the byte head with&31, then set key[wParam&255].
Repeated down messages are swallowed. WM_KEYUP and WM_SYSKEYUP clear state
only when bit30 is set; other up messages are swallowed. Left/right/middle
down/up set/clear key slots1/2/4. Unrecognized messages call real
DefWindowProcA and return its value. Original default path preserves message
in EAX through unsigned range dispatch and passes all four arguments unchanged.

BHInputSnapshot current/previous/history layout preserves original offsets:
current004ef0bc, previous+255, history+510, head+542. Low-byte key255
deliberately writes previous[0]; using an unsigned-char view of the whole
struct preserves that relationship without indexing outside current[255].
History head must already be0..31, as in initialized original globals; the
implementation does not claim original malformed-state safety. Source NULL
snapshot guard delegates to DefWindowProcA, an explicit binding extension.

No behavioral mismatch found in valid snapshot/message domain. This static
review does not establish device ownership, application event-loop integration,
input delivery or interactive gameplay. Real-source linked test/build evidence
is separate and remains parent's integration responsibility.
