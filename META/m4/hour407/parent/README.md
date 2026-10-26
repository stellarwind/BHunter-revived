# Whole native DirectSound startup

0047b500 is a complete228-byte/75-instruction sound initialization owner on the
actual00479200 startup path. Its cached pre-assignment raw review and current
immutable-PE verification distinguish this ordinary engine candidate from the
DirectSoundCreate import, reused CRT, compiler scaffolding and already admitted
0047b5f0 disposal. No third-party engine provenance is established.

Real closure is only DirectSoundCreate, Sleep and native COM methods, followed
by the existing actual BHSoundQueue disposal lifetime. The source accepts an
explicit freshly initialized queue and HWND instead of original globals. It
clears the single listener-active word, attempts default-device creation10
times with100ms sleeps only between failures, invokes cooperative level1,
requests the original20-byte primary descriptor (flags1, other fields0), and
actually calls primary Play(0,0,1). Device failure returns0. Once the device
exists, cooperative/primary/Play errors retain original success return1;
primary creation failure leaves primaryNULL. Actual HRESULTs/call counts are
recorded separately rather than replaced with success constants.

Returned COM references are owned directly by the actual queue, with no extra
AddRef hidden in an initialization adapter. The existing real disposal releases
them. Fresh-owner/NULL guards reject unsafe reuse; a defensive release covers
nonconforming native failure output. Those source guards and diagnostic fields
are count-zero adaptations. No fake COM interface or function hook is used.

`build_check.py` compiles the real TU, links the actual queue/fp/round definitions
and native libraries, and runs32 checks plus missing-module link control.
Two native initialization cycles each create the real default device/primary,
query capabilities/status, observe actual looping, immediately Stop without
uploading any samples, and prove final COM reference release through the actual
queue disposal. Each scenario therefore makes2 required primary Play calls.
No secondary sound/WAVs are played. No window, input, game/original code or
UI capture is used; GetDesktopWindow supplies the live normal-level HWND.

Ten-failure retry exhaustion and actual native cooperative/primary/Play errors
are retained source behavior but are not forced on this positive host. No claim
is made for physical CD audio, audible secondary playback, full startup or a
playable game. Full parent integration and clean-build evidence are required
before this implementation is admitted.
