# BHunter

<p align="center">
  <img src="assets/cover.png" alt="BHunter cover art">
</p>

<table>
  <tr>
    <td align="center"><a href="screenshots/1.png"><img src="screenshots/1.png" alt="BHunter screenshot 1" width="280"></a></td>
    <td align="center"><a href="screenshots/2.png"><img src="screenshots/2.png" alt="BHunter screenshot 2" width="280"></a></td>
    <td align="center"><a href="screenshots/3.png"><img src="screenshots/3.png" alt="BHunter screenshot 3" width="280"></a></td>
  </tr>
  <tr>
    <td align="center"><a href="screenshots/4.png"><img src="screenshots/4.png" alt="BHunter screenshot 4" width="280"></a></td>
    <td align="center"><a href="screenshots/5.png"><img src="screenshots/5.png" alt="BHunter screenshot 5" width="280"></a></td>
    <td align="center"><a href="screenshots/6.png"><img src="screenshots/6.png" alt="BHunter screenshot 6" width="280"></a></td>
  </tr>
  <tr>
    <td align="center"><a href="screenshots/7.png"><img src="screenshots/7.png" alt="BHunter screenshot 7" width="280"></a></td>
    <td align="center"><a href="screenshots/8.png"><img src="screenshots/8.png" alt="BHunter screenshot 8" width="280"></a></td>
    <td align="center"><a href="screenshots/9.png"><img src="screenshots/9.png" alt="BHunter screenshot 9" width="280"></a></td>
  </tr>
  <tr>
    <td align="center"><a href="screenshots/10.png"><img src="screenshots/10.png" alt="BHunter screenshot 10" width="280"></a></td>
    <td align="center"><a href="screenshots/11.png"><img src="screenshots/11.png" alt="BHunter screenshot 11" width="280"></a></td>
    <td align="center"><a href="screenshots/12.png"><img src="screenshots/12.png" alt="BHunter screenshot 12" width="280"></a></td>
  </tr>
  <tr>
    <td align="center"><a href="screenshots/13.png"><img src="screenshots/13.png" alt="BHunter screenshot 13" width="280"></a></td>
    <td align="center"><a href="screenshots/14.png"><img src="screenshots/14.png" alt="BHunter screenshot 14" width="280"></a></td>
    <td align="center"><a href="screenshots/15.png"><img src="screenshots/15.png" alt="BHunter screenshot 15" width="280"></a></td>
  </tr>
</table>

A source restoration of **BHunter**, the 1999 hovercar combat game by **InterActive Vision**, published by **Midas Interactive Entertainment**. Hunt criminals through a futuristic city and collect their bounties. [Original game details](https://gamefaqs.gamespot.com/pc/250530-bhunter/data).

## Features

- SDL powered window management, hardware rendering, input and audio.
- 60fps, interpolated from 30.
- Up to 4k resolution with 4:3, 16:9, 21:9, and 16:10 aspect ratios.
- Gamepad support

## Install

1. Download and extract the [latest BHunter release](https://github.com/stellarwind/BHunter-revived/releases/latest) on Windows.
2. Create a `GAME_DATA` folder beside the executable and copy `Bhunter.bin` from your own original game into it.
3. Run `BHunter-source.exe`.

Music plays from `MUSIC_DATA/track02.wav` beside `GAME_DATA` (the extracted PCM WAV of CD track two, without the pregap). Extract the track from your own disc; mounted-CD playback is no longer used. Game volume controls affect only BHunter.

Supported original game: **BHunter, English retail PC edition (1999)**. The tested files are:

| File | SHA-256 |
|---|---|
| `Bhunter.bin` | `2393a661fd647ed6de3eee34c35c30812c44f3ee15b9d44244a63720804df581` |
| `BHUNTER.EXE` (reference only) | `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1` |

See [How to play](HOWTOPLAY.md) for controls.

## Build

Requires Python 3.10+ and a **32-bit MinGW-w64 GCC toolchain** with `gcc` and `g++` on PATH. SDL3 is downloaded automatically on the first build.

Place your original `Bhunter.bin` in `GAME_DATA` or `build/GAME_DATA`, then run:

```bat
build.cmd
build\BHunter-source.exe
```

The game and required DLLs are written directly to `build/`. Keep them together when moving the game.

### Native Linux build

The game builds for 32-bit x86. Install GCC and G++ with 32-bit C and C++ libraries, 32-bit SDL3 development files, Python 3.10+, and `pkg-config` configured to find the 32-bit SDL3 package. Running the game requires a 32-bit Vulkan loader and driver.

Place `Bhunter.bin` in `GAME_DATA` as above, then run:

```sh
python3 tools/build_gcc.py --incremental
./build/linux-x86/BHunter-source
```

## AI disclaimer

This source restoration was decompiled, reconstructed and built with the assistance of OpenAI Codex. It is an unofficial restoration; exact original-game behavior is still being reviewed.

## License

Restoration code: [MIT](LICENSE). Original game assets remain the property of their respective owners. Bundled [SDL3](https://github.com/libsdl-org/SDL/tree/release-3.4.18) uses the zlib license, included in the release.
