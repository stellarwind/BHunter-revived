# Actual DirectPlay provider enumeration callback

Complete00475d60 (398 bytes/139 instructions) is the six-argument WINAPI
EnumConnections callback installed by startup00475ef0. The complete PE-verified
body ends RET24. Ordinary provider/profile behavior has unresolved engine
authorship; CRT allocation/copy and system COM roles are reused dependencies,
not additional recovered owners. Analysis metadata changes no reference bytes.

The source retains eight188-byte slots, exact GUID copies, owned connection
payload copies, zero word+20, provider type+24,20-byte ANSI short-name copy at
156 and NUL at176. Exact original MODEM/TCPIP/IPX/SERIAL GUIDs map to kinds
2/1/0/3 and their original labels; unknown providers retain kind4 and truncated
short name. Other bytes are retained, including overlapping profile storage.
The source names only verified fields and does not invent a complete profile
layout. Slot8 cancels actual enumeration. The explicit source catalog/allocator
owns connection copies until disposal and must outlive callback use.

Invalid input/full catalog/allocation failure receive transactional source
guards; original callback assumes valid inputs/allocation and increments its
global count before unchecked copies. These guards and catalog init/dispose
are count-zero adapters. The ordinary valid-input copy/label/type/count contract
is retained. Whole provider initialization00475ef0 remains a separate owner;
this callback neither initializes a connection nor opens any session.

`build_check.py` compiles the actual source/test/driver with pinned VC5 and DX7
headers, links real COM/CRT and native definitions, executes109 focused checks
and requires missing-module link failure. Synthetic cases cover all four GUIDs,
unknown providers, non-word blob lengths, exact eighth-slot cancellation, full
record-byte preservation and allocation failure. Native CoCreateInstance of
actual DirectPlay3A registers the recovered callback with EnumConnections,
copies actual provider connection data, disposes every allocation and releases
the final COM reference. No connection initialization/session, original/game
code, physical input, UI capture, audio or playback occurs.

Whole-parent integration and independent clean-build admission are required.
