# Triple frame-buffer owner pre-review

2026-10-25. Before decompilation, retained call graph/global use classify
00471c00/00471cc0/00471d70 as engine_candidate ordinary queue-buffer lifetime
and producer handoff owners, authorship unknown. They are not verified CRT,
compiler EH/stack support, imports, or shared tails. Verified CRT allocation
and Win32 synchronization are dependencies rather than game recovery targets.

export_static.py pins complete contiguous RET-ended instruction bodies against
the immutable original PE:00471c00..00471cb6=183bytes/36instructions,
00471cc0..00471d67=168/50,00471d70..00471de6=119/32. owner.json records
SHA256; original/ preserves raw TSV/JSON/decompiler. No original execution.

## Initialization and actual caller

Initializer stores frame callback00491884 and message callback00491888,
computes double1000/rate-1.5, stores binary64 interval00491890, compares retained
result with double1.0 and clamps lower values to1.0. All three constants
00489fc8/00489fd0/00489fd8 are actual eight-byte doubles, respectively1000,
1.5,1.0; constants.json includes verified raw bytes and referencing addresses.
Three12-byte descriptors004ee308/314/320 each receive allocation(size),
busy0 and signed age3. Allocation failure is unchecked original behavior.

Actual startup call004798bf..004798e4 (also byte-verified) passes arguments:
frame callback00479140, double30.0 (words0 and403e0000), size0x47c70,
message callback00473c40, then caller cleans20bytes. Size0x47c70=294000=
3500*84bytes, consistent with draw command stride0x54. These addresses identify
real callbacks but do not authorize replacing unrecovered callback bodies with
fake game logic or claiming complete startup from buffer tests.

## Acquire and publish

Acquire waits on original mutex004ee730 with5ms timeout, retrying only
WAIT_TIMEOUT0x102. It chooses free slot with signed age strictly greater than
current best, initially-1. Ties favor earliest slot. It publishes active
descriptor004eed5c, marks busy1/age-1, releases mutex, returns buffer pointer.
Original non-timeout wait failures still proceed; source may guard explicitly.

The no-available branch is undefined/unestablished fallback behavior, not a
gameplay argument contract.00471cc0 PUSH ECX saves incoming register; if slot0
does not qualify,00471d0c reloads that saved value into ESI. A later qualifying
slot can replace it; otherwise it is dereferenced. No read/caller evidence
establishes a semantic fastcall parameter. Ghidra's provisional param_1 does
not justify adding one. Source should reject no-slot or otherwise document a
deliberate synchronization adapter, never fabricate a buffer from ECX.

Publish uses the same wait loop, increments all signed age words by one with
32-bit wrap, clears active busy, then copies active descriptor004eed5c into
published004eed58 before ReleaseMutex. It does not clear active pointer.
Source real three-slot memory and native Win32 mutex must own this lifetime;
focused fixtures need no original loader, original callbacks or game execution.
