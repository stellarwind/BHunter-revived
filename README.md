# BHunter

<p align="center">
  <img src="assets/cover.png" alt="BHunter cover art">
</p>

<table>
  <tr>
    <td align="center"><a href="screenshots/BHunter-20261029-230509-088-1.jpg"><img src="screenshots/thumbs/BHunter-20261029-230509-088-1.jpg" alt="BHunter gameplay screenshot 1" width="280"></a></td>
    <td align="center"><a href="screenshots/BHunter-20261029-230621-253-2.jpg"><img src="screenshots/thumbs/BHunter-20261029-230621-253-2.jpg" alt="BHunter gameplay screenshot 2" width="280"></a></td>
    <td align="center"><a href="screenshots/BHunter-20261029-230700-576-3.jpg"><img src="screenshots/thumbs/BHunter-20261029-230700-576-3.jpg" alt="BHunter gameplay screenshot 3" width="280"></a></td>
  </tr>
  <tr>
    <td align="center"><a href="screenshots/BHunter-20261029-230715-224-4.jpg"><img src="screenshots/thumbs/BHunter-20261029-230715-224-4.jpg" alt="BHunter gameplay screenshot 4" width="280"></a></td>
    <td align="center"><a href="screenshots/BHunter-20261029-230812-238-5.jpg"><img src="screenshots/thumbs/BHunter-20261029-230812-238-5.jpg" alt="BHunter gameplay screenshot 5" width="280"></a></td>
    <td align="center"><a href="screenshots/BHunter-20261029-230838-789-6.jpg"><img src="screenshots/thumbs/BHunter-20261029-230838-789-6.jpg" alt="BHunter gameplay screenshot 6" width="280"></a></td>
  </tr>
  <tr>
    <td align="center"><a href="screenshots/BHunter-20261029-230940-826-7.jpg"><img src="screenshots/thumbs/BHunter-20261029-230940-826-7.jpg" alt="BHunter gameplay screenshot 7" width="280"></a></td>
    <td align="center"><a href="screenshots/BHunter-20261029-231029-578-8.jpg"><img src="screenshots/thumbs/BHunter-20261029-231029-578-8.jpg" alt="BHunter gameplay screenshot 8" width="280"></a></td>
    <td align="center"><a href="screenshots/BHunter-20261029-231036-260-9.jpg"><img src="screenshots/thumbs/BHunter-20261029-231036-260-9.jpg" alt="BHunter gameplay screenshot 9" width="280"></a></td>
  </tr>
</table>

A source restoration of **BHunter**, the 1999 hovercar combat game by **InterActive Vision**, published by **Midas Interactive Entertainment**. Hunt criminals through a futuristic city and collect their bounties. [Original game details](https://gamefaqs.gamespot.com/pc/250530-bhunter/data).

## Install

1. Download and extract [release 0.1](https://github.com/stellarwind/BHunter-revived/releases/tag/v0.1) on Windows.
2. Copy `Bhunter.bin` from your own original game into the `GAME_DATA` folder.
3. Run `BHunter-source.exe`. Video settings apply immediately; choose Windowed or Borderless and any enumerated display resolution up to 3840×2160. Alt+Enter switches window mode. Window position, normal size and maximized state are saved in `video-sdl3.ini`.

Music plays from a mounted BHunter CD, or from `MUSIC_DATA/track02.wav` beside `GAME_DATA`. 

Supported original game: **BHunter, English retail PC edition (1999)**. The tested files are:

| File | SHA-256 |
|---|---|
| `Bhunter.bin` | `2393a661fd647ed6de3eee34c35c30812c44f3ee15b9d44244a63720804df581` |
| `BHUNTER.EXE` (reference only) | `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1` |

See [How to play](HOWTOPLAY.md) for controls.

Original game files are not included. Rendering and window management use SDL3 GPU (Direct3D 12 on Windows); `ddraw.dll` is no longer required. This is an early playable release; full original-game fidelity is still being reviewed.

## Build

Requires Python 3.10+, Visual C++ 5.0 with Visual Studio 97 SP3, the DirectX 7 SDK headers/DirectInput library, and a 32-bit MinGW-w64 C++17 compiler (`g++` on PATH, or `BH_GPU_CXX`). Proprietary build dependencies are not included.

Set these paths in a Command Prompt:

```bat
set BH_VC_ROOT=C:\VC5\VC
set BH_SP3_ROOT=C:\VS97SP3\all\vc
set BH_SHARED_DIR=C:\VC5\SHAREDIDE\BIN
set BH_DXSDK_ROOT=C:\DX7SDK
set BH_COMPILER_DIR=C:\BHunter-compiler
```

Prepare `BH_COMPILER_DIR` with stock VC5 `CL.EXE`, SP3 `c1.dll`, `c1xx.dll`, `c2.exe` (from `enu\vc\bin`), and stock `MSDIS100.DLL`, `MSPDB50.DLL`, `MSVCP50.DLL` (from `SHAREDIDE\BIN`). The build uses the stock VC5 linker and SP3 headers/libraries.

```bat
build-source.cmd
build\BHunter-source.exe
```

The build stages the executable and your local `GAME_DATA` and `MUSIC_DATA` into `build/`. The CFG lives beside the executable. Run from that folder, exactly like the release. To create a release ZIP:

```bat
python tools\package_release.py --version 0.1
```

The build downloads and checksum-verifies official SDL 3.4.18. The adapter is built as `BHunterGPU.dll`; its shaders are in `shaders/legacy.hlsl`. The restored game remains x86. The DirectX SDK is used for legacy data types and input; rendering creates no DirectDraw/Direct3D COM objects.

The game uses a 640×480 logical layout in a centered 4:3 viewport, with geometry rasterized at the output resolution. This preserves HUD and 3D menu proportions. Simulation and sound updates remain at 30 Hz; rendering interpolates completed command snapshots and is capped at 60 Hz.

For menu/VFX screenshots, scaling comparisons and timing checks:

```bat
python tools\build_game.py --debugging --incremental
python tools\check_sdl3.py
```

Diagnostics write to `build-cache/sdl-captures/`. They exercise main/options/video navigation, capture menu rows and VFX at 640×480, 1080p and 4K, check player/camera/cockpit interpolation while visibility and HUD contents change, compare moving target brackets pixel-for-pixel against render-pose references, reproduce and verify portal seam coverage with a moving render camera, check that interpolated frames do not resurrect consumed bullet flares, and run a bounded live menu/music check. They do not automate the desktop. Full mission/NPC visual parity remains a separate validation step.

## Unlock all levels

Quit the game, then run with Python 3.10+:

```bat
python tools\unlock_levels.py
```

This unlocks all nine mission entries in `source-preview.cfg` beside the executable (`build/source-preview.cfg` for local builds), backing up the save before changing it. Other settings stay intact. For another save, add `--save "path\to\source-preview.cfg"`.

## AI disclaimer

This source restoration was decompiled, reconstructed and built with the assistance of OpenAI Codex. It is an unofficial restoration; exact original-game behavior is still being reviewed.

## License

Restoration code: [MIT](LICENSE). Original game assets remain the property of their respective owners. Bundled [SDL3](https://github.com/libsdl-org/SDL/tree/release-3.4.18) uses the zlib license, included in the release.
