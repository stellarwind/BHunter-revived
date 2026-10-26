# Complete native provider initialization

Whole00475ef0 is317 bytes/111 instructions on actual startup00479200's direct
dependency path. Its ordinary native provider/modem initialization role is an
engine candidate, not a COM import, compiler helper or independent shared tail.
Full immutable-PE instruction bytes and exact GUID data establish its boundary
and original interfaces. Decompilation obscures arguments, so raw stack/vtable
instructions and pinned SDK layout assertions own the contract.

The complete source resets provider/modem counts, calls actual CoInitialize,
creates actual DirectPlay3A and Lobby2A, invokes real EnumConnections through
the recovered six-argument callback with original application GUID
FE8D1C00-ABFA-11D2-9635-0020182A833A, loops over original kind2 modem providers
and calls real InitializeConnection, queries system player0 address, allocates
and retries on BUFFERTOOSMALL, then uses the actual admitted modem address
callback when address data and Lobby exist. Temporary address data is freed,
Lobby is released/cleared, then peer is released/cleared. No session or player
is opened/created. Successful COM initialization remains owned until same-thread
actual source network shutdown, as original.

Explicit catalog/modem/startup context replaces original globals; provider
connection copies remain owned by the catalog after native interfaces leave.
Fresh-owner and failed allocation/lobby guards are source safety adaptations.
First size output starts0 instead of unspecified original stack contents.
HRESULTs, call counters and successful-initialization counters are diagnostics;
return1 means valid invocation, not successful networking. They cannot convert
failed native operations into success. Context/buffer/schema helpers add no count.

Independent review caught NULL application GUID in the first source candidate;
the actual original GUID is now supplied, and focused/current full validation
belongs only to corrected source bytes. No prior candidate was admitted.
Raw interface GUIDs and all native method slots, kind loop, query/callback
arguments, buffer free and release order were independently reviewed.

`build_check.py` compiles real source TUs and real native/COM dependencies with
the pinned compiler/SDK and links a named console target.18 focused checks pass
with a missing-module link control. The actual host creates both interfaces,
enumerates providers, attempts1 modem InitializeConnection, records failure
HRESULT-2005466886, then records GetPlayerAddress failure-2005466816. Successful
connections0, address enumerations0, sessions0. All copied/temporary ownership
and actual source COM shutdown are checked. Positive connection/address/EnumAddress
branches are implemented but remain unforced host limitations, not success claims.
No game/original code, physical input, audio, UI capture or gameplay ran.

Full integration/independent clean builds are required before admission. Full
DirectPlay gameplay, actual peers, physical modem support and M4 remain open.
