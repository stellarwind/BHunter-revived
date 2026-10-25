# Concrete next owner after window/worker/service

**Still no playable source game.** This is a read-only frontier audit, not new
implementation/admission. No original/game execution or native input occurred.
`next-review/startup-direct-dependencies.json` records every distinct direct
callee of complete `00479200` and all matching ledger variants, including WIP
duplicates. Pending window/FF-start/CD-notify owners are not reassigned here.

## Recommend complete sound resource loader `0047b950`

Recover the whole781-byte/261-instruction `0047b950..0047bc5c` next. It directly
fills the resources consumed by the now restored player/flyby sounds and frame
service's deferred sound tick. Startup invokes it repeatedly for actual WAVs;
it does not require the large mission/resource constructor. The current ledger
contains only `0047b950-wave-data-subset-c` WIP in `bh_wave.c`, and
`bh_sound_register_wave` explicitly remains an adapter rather than this owner.
Do not promote either subset or count the scanner again.

The reviewed boundary is an ordinary engine resource owner, with unknown
historical provenance, not CRT/import/compiler support or a shared tail.
`next-review/0047b950.disasm.json` contains every instruction verified against
immutable PE raw bytes. Fresh decompiler output is saved but loses several
stack/register identities; recover the native buffer descriptor, upload spans
and metadata from raw instructions rather than copying its `unaff_*` artifacts.

**Closure is bounded: there are no missing direct source callees.**
`0047b330` resource allocation, `0047b830`=`bh_riff_scan`, and VFILE
open/close/seek/read/tell (`0047cbd0/0047ce50/0047cec0/0047cf60/0047d230`) are
already admitted. The ledger has an older WIP scanner record alongside the
complete `bh_riff_scan`; use its complete implementation. malloc/free/rand
are reused CRT. No indirect game callback is installed or invoked here.

Real remaining work is owning the file buffer, scanning original RIFF request
semantics, creating an actual DirectSound buffer (flags0xe0), copying both
Lock spans, Unlock/Release failure lifetimes, allocating/publishing the real
resource, filename storage, GetFrequency fallback44100, voice-limit state,
cache initialization, RNG phase and original x87-derived resource metadata.
Retain existing sound owner definitions; do not guess metadata values or claim
the PCM registration adapter supplies them. Source guards must bound malformed
RIFF/name lengths without silently substituting another format contract.

Use the existing borrowed real DirectSound device binding and archived real
WAV fixtures for linked checks. Native buffer readback can prove exact upload
without Play; verify missing/truncated files and allocation/backend failures.
No audible playback, physical input, CD device or full startup is needed for
this loader. The numerical metadata requires explicit PC53/rounding and RNG
controls; byte matching is optional and separately bounded.

## Exact missing direct startup owners

`00479200` itself is1,800 bytes/428 instructions. Seven direct original owners
remain incomplete; their presence is not a claim that all startup dependencies
are restored:

| Address | Bytes / instructions | Current blocker |
|---|---|---|
|`004474c0`|9,584/2,572|large world/resource bootstrap; defer whole closure |
|`0044a160`|178/42|loading-screen setup also needs missing769-byte `00449e50` image owner |
|`0044aac0`|1,487/444|loading renderer and several separate draw owners; defer whole closure |
|`00471260`|350/113|force-feedback constructor needs actual104-byte enum callback `004711b0`, data-format/effect/GUID bindings and real hardware-positive validation |
|`00475ef0`|317/111|DirectPlay provider initialization also installs missing131-byte callback `00475cd0`; not the admitted packet transport |
|`0047b500`|228/75|DirectSound startup/primary-loop owner, separate from existing binding adapter |
|`0047b950`|781/261|recommended whole resource loader above; existing wave subset is WIP |

Other distinct direct callees are already admitted: texture/model path and
policy/image load (`0044e880/0044e910/0044f4f0/004507c0`), camera settings
(`00451720`), render material (`0045b1d0`), frame buffers (`00471c00`), input
names/devices (`00473d60`), player-input create/bind-key/bind-string
(`00475640/00475770/00475810`), entities (`00476da0`), voice-limit policy
(`0047b920`) and archive mount (`0047d270`). `0047de50` is verified reused
VC5 `_printf`, not another game assignment.

The sound initializer `0047b500` is a small following owner: actual
DirectSoundCreate with10 attempts/100ms intervals, cooperative level1, primary
buffer creation and Play looping. A primary-creation failure still returns1
with primaryNULL. Its complete raw228-byte body is retained. Do not add that
Play to loader-only validation or label it already recovered by the current
device-binding adapter.

The alternative force-feedback constructor closure was also checked before
selecting scope: `004711b0` is a real stdcall enumeration callback, creates a
device, QueryInterfaces Device2, releases the temporary interface and stops
enumeration only on success. It is not CRT/import or shared code. The
constructor's six-byte `0047d658` target is the DirectInputCreateA import thunk,
excluded from recovery. Its Acquire/EffectStart paths require a legitimate
owned physical device; the current no-input-execution constraint makes it a
less useful immediate admission than sound buffer loading.

Full WinMain and full `00479200` remain separate owners. No prefix extraction,
callback registration or single WAV loader completes either bootstrap.
