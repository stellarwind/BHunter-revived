# CD stop/close boundary

00449b40 is an ordinary complete media-lifetime owner: 77 bytes, 21 instructions, terminal RET. It tests device004dfb6c, clears playing004dfb70 before MCI_STOP, reloads the current device for MCI_CLOSE, then clears device and sets last-track004912d0 to -1. An absent device changes nothing. The owner is neither an import wrapper alone nor verified CRT/compiler support; historical engine provenance remains unresolved.

The explicit BHCdClose context borrows the live media state. Real mciSendCommandA calls retain their order; reported errors do not suppress the original subsequent state changes. No playback, CD opening, drive enumeration or replacement backend is implemented here.

The source-only focused target compiles the real translation unit with the pinned VC5 setup and links winmm.lib. It creates a private two-sample PCM WAV and opens it as a stopped waveaudio MCI surrogate, verifies native STOP/CLOSE and loss of the retired device ID, verifies both-error cleanup using that same retired private ID, and removes the fixture. Eleven checks pass. This does not validate a physical CD device, playback timing, track selection or the rest of the CD subsystem. The test never sends MCI_PLAY. Its first invalid-ID fixture used the MCI all-devices value and was discarded; the final fixture exclusively addresses its own retired device ID.

Original instruction bytes are in 00449b40.disasm.json and are checked against the immutable PE by the family recorder. cd-focused.json records focused commands/results; whole integration and clean evidence remain separate admission gates. No game or original code executes.
