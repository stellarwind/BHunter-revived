# CD-audio and DirectPlay host availability — 2026-10-24

The isolated x86 `m3_host_probe.c` compiles with the pinned VC5 toolchain, links with real Windows import libraries, and executes system APIs. It runs no original game code, opens no network session and requests no playback or volume changes. The DirectX 7 dplay.h dependency is explicit and hash-pinned in its report; its local include path affects this diagnostic only. Original GUID bytes at 0048a088/0048a098 match IID_IDirectPlay3A and CLSID_DirectPlay used by this probe. This is host capability evidence, not original-game behavior validation.

## CD audio: actual host blocker

GetLogicalDrives/GetDriveType finds zero CD-ROM drives. The same MCI_OPEN_TYPE/cdaudio request used by the original returns 266, defined by the pinned MMSYSTEM.H as MCIERR_CANNOT_LOAD_DRIVER. No MCI device opens and no status/play/close request follows. auxGetNumDevs returns 0; waveOutGetNumDevs returns 2. A working wave output device does not supply the missing CD-audio device.

The immutable MDF still contains audio track 2, and its original selected range/loop contract remains in cd-contract.json. Data availability is established; runtime MCI playback and completion-notify looping are unavailable in this host environment as currently configured. This does not justify removing CD audio or substituting an unreviewed backend. A usable CD-audio device/media route is required to capture the original MCI scenario; the probe installs or mounts nothing.

## DirectPlay: API and providers available

CoInitialize and CoCreateInstance for the original DirectPlay3A interface both return S_OK. EnumConnections with DPCONNECTION_DIRECTPLAY returns S_OK and three providers:

- Serial Connection For DirectPlay — 0f1d6860-88d9-11cf-9c4e-00a0c905425e
- Modem Connection For DirectPlay — 44eaa760-cb68-11cf-9c4e-00a0c905425e
- Internet TCP/IP Connection For DirectPlay — 36e95ee0-8577-11cf-960c-0080c7534e82

The COM interface is released and COM uninitialized. No connection initialization, discovery, hosting, joining or packet exchange occurs. An absent legacy DLL/API is therefore not the current networking blocker. Actual original host/join/mode and peer observations remain required. Serial/modem enumeration alone establishes no attached serial/modem hardware. A same-host session test may answer some lifecycle questions later, but cannot prove independent-machine interoperability.

## Reproducibility and limits

`META/m3/host/662c8a7b39f4.json` retains compile/link/query commands, outputs, source snapshots/hashes and executable hash. `python tools/check_m3_host_evidence.py` reads the evidence without repeating queries, verifies original GUIDs, successful real linking/execution, archived sources and actual return values. Original EXE/BIN/MDF/MDS hashes and original SourceDir registry views remain unchanged. Diagnostic process success means its queries completed; MCI failure is preserved, not relabeled successful playback. No feature is silently deleted and M3 remains active.
