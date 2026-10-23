# BHunter restoration reconnaissance

Latest math pass: ten recovered routines and two typed layouts are in `../src/bh_math.c` and `../src/bh_math.h`. See `math/RECOVERY.md` for the native 60,000-case differential test, the 24-bit x87 rounding differences, Ghidra type updates, and the expanded 240-configuration compiler comparisons. Exact original compiler flags remain unresolved.

Inspected 2026-10-23 using ghidra-cli and local PE/header inspection. Original executable is unpatched.

## Binary and compiler

- English EXE: 609,792 bytes; SHA256 `c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1`.
- Native 32-bit x86 Windows PE, image base `0x00400000`.
- PE timestamp: 1999-09-11 10:57:53 UTC (metadata, not independent proof of build date).
- PE linker version **5.10**. Microsoft Visual C++ runtime string, Microsoft CRT startup/SEH code, and Ghidra's `visualstudio:unknown` classification support **Visual C++ 5.0-era tools** as the leading candidate. Exact compiler build/service pack is not established; linker version alone does not prove compiler version or exclude mixed object files.
- CRT is apparently statically linked: CRT startup and file routines are inside the EXE; no MSVCRT runtime DLL import. No MFC DLL dependency in the import table.
- Existing VC4/4.1/4.2 tools are useful for comparison, but VC5 is the better candidate for a matching rebuild. Do not select a compiler solely to make reconstructed source compile; first compare generated code and runtime signatures.
- Entry point `0x0047e730`; WinMain `0x004723c0`.

## VC5 CD comparison

Extracted compiler, headers, libraries, CRT sources and supporting DLLs from the supplied Enterprise CD1 into `D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO`. CL runs and reports **11.00.7022**; LINK runs and reports **5.00.7022**. Add both `VC/BIN` and `SHAREDIDE/BIN` to PATH when invoking these tools. Headers and libraries are in VC/INCLUDE and VC/LIB. No system installation was performed.

`compare_crt.py` compares library COFF .text sections against the original EXE, masking relocation operands and requiring all remaining section bytes to match. A section must contain a relocation-free run of at least 24 bytes to qualify. This is a conservative code comparison, not a complete compiler identification system; library members absent from the EXE naturally do not match.

| Library | Matched eligible sections | Matched section bytes |
|---|---:|---:|
| VC5 LIBC | 135 / 720 | 33,847 |
| VC5 LIBCMT | 98 / 749 | 24,097 |
| VC4.1 LIBC | 17 / 798 | 6,785 |
| VC4.1 LIBCMT | 13 / 817 | 5,851 |

Results saved in `crt-comparison.json`. This strongly supports VC5-family CRT code and favors the single-threaded LIBC variant. Shared routines across library variants mean these counts are not independent evidence for every member. The original EXE's linker 5.10 differs from the CD's 5.00.7022; an updated toolset or mixed compiler/linker combination remains possible. A particular service pack is not yet established.

## Compiler flags, library scout, and executable map (second pass)

The SP3 runtime is now the strongest match: **146 sections / 42,711 bytes**, versus the original VC5 LIBC's 135 / 33,847 bytes. Complete relocation-masked SP3 matches include `WinMainCRTStartup` at `0047e730`, formatted input/output, and locale routines. This identifies the runtime family strongly; it does not uniquely identify which compiler compiled every engine/game translation unit. The single-threaded CRT points to **/ML or equivalent explicit LIBC selection**, not /MT or /MD. The game can still create its own threads while linking a single-threaded CRT.

Matching does not require every library section to occur in the EXE. A static library contains many routines the game never uses. Our conservative matcher also skips short sections, requires a 24-byte unrelocated anchor, and compares whole remaining sections; a non-match is not evidence of a wrong compiler. Matched runtime bytes include alignment padding, so do not treat them as an exact original-source size.

COFF symbols recovered 174 runtime symbol addresses from the SP3/original VC5 matches. Ghidra has **147 verified CRT functions** tagged `library-crt-verified`; the other symbol addresses can be labels/entries not yet represented as independent functions. Short unmatched routines in the same runtime region must not be called game code.

Compiler option experiments are in `run_compiler_probes.py` and `compiler-probes-{sp3,stock,vc41}.json`. The script builds behavioral reconstructions of vector cross product and matrix translation under 80 flag combinations per toolset: /Od, /O1, /O2, /Ox; /G3, /G4, /G5, /G6, /GB; and default, /Op, /Oa, /Ow variants. It also varies commutative operand order in the reconstructed expressions. **None reproduced either original routine exactly.** These sources are reconstructions, so this does not exclude those compilers or options. Generated object files and source are retained for inspection. The SP3 driver still displays its unchanged 11.00.7022 banner while invoking patched 11.00.7303 backends.

Observations supporting an optimized build: most functions before the import-thunk area omit a standard EBP frame; optimized x87 scheduling and inline string operations occur. **Exact /O flags, /G CPU target, /Op setting, /Zp packing, /Ob inlining, and per-file overrides are not established.** Neither /O2 nor /G5 should be recorded as confirmed. Recovering the full historical command line may be impossible because different flag combinations can emit identical code and objects may come from different builds.

The Rich header exists, but its decoded records do not provide a useful compiler build fingerprint here. There is no recovered original compiler command line.

### Ownership and reusable-library scout

No byte matches were found against sampled VC5 LIBCP, LIBCPMT, LIBCI, GLAUX or LARGEINT static code sections. Many DirectX libraries tested are import/GUID libraries and contain no eligible code sections, so their zero match counts say nothing about API usage. Imports and COM references establish DirectX use independently.

No QvLib, RenderWare, BRender, Miles, Smacker, zlib, JPEG or PNG attribution strings were found in the initial ASCII string scout. Absence of strings does not exclude stripped, modified, or unattributed libraries. The math, image loader and VFILE layer remain **engine/reuse candidates**, not verified original game code. There is currently no identified open-source library to substitute wholesale.

DirectPlay3A and the DirectPlay COM CLSID have active references in `00475ef0` and `00476410`. CD music is confirmed separately by the MCI `cdaudio` initialization in `00449a50`.

### First executable map

- `function-map.tsv`: **887 functions**, names, observed bodies, calls, string references, frame usage and confidence tags.
- `call-edges.tsv`: direct call graph edges; indirect calls require separate analysis.
- `unassigned-code.tsv`: decoded instructions outside current function bodies.
- `subsystems.tsv`: manually selected subsystem anchors with evidence and ownership status.
- `binary-map.json`: section sizes and aggregate counts.

Initial Ghidra analysis defined only 644 functions. Decoded code pointers to aligned unowned instructions yielded **243 additional candidate callback functions**, bringing mapped instruction bytes to **530,895 of 537,914 (98.7%)**. This is instruction-byte coverage, not completeness of source recovery. Callback boundaries and signatures still require review; remaining gaps may include shared tails and incorrectly bounded functions. No EXE bytes were changed.

There are **677 functions before 0047d360**, the approximate game/engine region. This is not a count of verified original game functions. Around `0047d360..0047d65f` are import thunks, and from roughly `0047d660` onward is the Microsoft runtime region.

On disk the EXE contains 554,496 bytes of .text, 9,728 bytes of .rdata, 40,960 initialized .data bytes, 3,584 .rsrc bytes, and 1,024 header bytes. .data expands to 453,796 bytes in memory. Thus the 609,792-byte EXE is not all game instructions, and its runtime is statically linked.

Restoration workflow: identify reusable library/CRT code; repair function signatures and structures at subsystem boundaries; recover platform-independent math, assets, scripts and simulation incrementally with behavioral comparisons; modernize graphics/input/audio behind explicit interfaces. Matching the compiler helps reconstruction, but an exact historical flag set is not a prerequisite for a faithful playable port.

## DirectX

- The EXE explicitly reports: `This application requires DirectX5.0 or newer`.
- DirectDrawCreate and DirectDrawEnumerateA imports; startup queries **IDirectDraw2**, GUID `b3a6f3e0-2b43-11cf-a2de-00aa00b93356` at VA `0x0048a430`.
- Renderer initialization `0x0045bff0` references **IDirect3D2** (`6aae1ec1-662a-11d0-889d-00aa00bbb76a`, VA `0x0048a400`) and **IDirect3DHALDevice** (`84e63de0-46aa-11cf-816f-0000c020156e`, VA `0x0048a3f0`). Texture setup references **IDirect3DTexture2** at `0x0048a3e0`.
- DirectInputCreateA in `0x00471260` receives version **0x0500**. IDirectInputDevice2A and constant-force GUIDs occur in the binary; initialization includes force feedback setup.
- DirectSoundCreate is imported for sound. WinMM MCI, auxiliary mixer, wave volume, and joystick APIs are also imported. `cd music` strings and MCI imports warrant tracing CD audio separately before calling the restoration complete.
- Hardware and software rendering choices are present in startup. Renderer code handles pixel masks and 8/16/24/32-bit formats; the shown hardware initialization rejects fewer than two bytes per pixel. Exact supported modes need a separate pass.
- REDIST's DDRAW.DLL and D3DIM.DLL report **4.06.02.0436**, consistent with DirectX 6.1 distribution. Shipped runtime version is distinct from the DirectX 5-era interfaces the game requests.
- DirectPlay/DirectPlayLobby GUID blocks are present, but GUID presence alone does not prove active multiplayer support; trace references before implementing it.
- GUID names were matched against local `D:/Decomp/dx7sdk/include` headers. Older interface definitions in those headers are useful even though this is not a DirectX 7 application.

## Why the CD prompt appears

WinMain first calls `0x00473a40`, now named `BH_CheckSourceFiles` in Ghidra. Failure triggers the retry/cancel "Please insert BHunter CD!" dialog.

That function opens **HKLM\SOFTWARE\InterActive Vision\BHunter\Paths**, reads **SourceDir**, builds paths for `copyright.txt`, `BIP_B1.TXT`, and `BIP_B2.TXT`, and opens `copyright.txt`. It XORs 32-bit words of that file and compares the result with **0x6b405d08** (VA `0x0049189c`). On 64-bit Windows the original 32-bit program's registry view matters.

Thus missing installation registry data and the source file are sufficient to produce this CD prompt. This routine does not establish a physical-drive requirement. BIP paths are constructed here, but this function's decompilation shows only copyright.txt being read. Other checks and CD audio dependencies still require investigation.

`BH_InitPaths` at `0x00473910` constructs a Windows-directory BHUNTER.INI path and a current-directory archive path. Its obfuscated string `CIVOUFS/CJO` decodes by subtracting one from each character to `BHUNTER.BIN`; it returns 2 in this executable. Working directory therefore matters.

Next practical step: make source-path discovery portable, preserving checksum behavior initially, then trace graphics initialization and CD music. No EXE patch or registry change has been made in this pass.

## Asset archive

`Bhunter.bin` is 13,154,037 bytes. All entries validated as contiguous, in bounds, ending exactly at EOF.

- Little-endian u32 file count: **1229**.
- 1229 directory records of **256 bytes**, beginning at offset 4.
- Each record: filename field of 248 bytes (NUL-terminated ASCII), u32 start offset, u32 exclusive end offset.
- First payload starts at **314628 / 0x4cd04**.
- Extensions: **505 BSP, 459 PCX, 203 TXT, 60 WAV, 1 LEV, 1 PTH**.
- A sampled TXT contains readable commands such as `trigger`, `waitfor`, `render off`, `collision off`, `detach`, `explode`, and `timer2`: potentially valuable behavior specifications.
- Full validated directory is saved in `assets-index.json`. Asset payloads have not been expanded yet.

## Saved analysis

Ghidra project: `../ghidra-projects/Bhunter.gpr`, program `BHUNTER.EXE`. Startup functions were renamed for navigation. Decompiled C files here retain the initial inferred types; Ghidra currently misidentifies some signatures/stack variables, so consult disassembly before patching or implementing calling conventions.

- `0047e730.c`: CRT entry.
- `004723c0.c`: WinMain/startup.
- `00473a40.c`: source-path/checksum check.
- `00473910.c`: path initialization.
- `00471260.c`: DirectInput setup.
- `0045bff0.c`: renderer initialization.
- `imports.json`: imported DLLs/functions.

GAME_DATA now contains only REDIST, English BHUNTER.EXE, and Bhunter.bin. Installer removal was blocked by automatic approval review; files were moved out to `%TEMP%/Bhunter-installer-fc35952481c845808a76ae444149428a` instead. Original MDF/MDS remain available to recover source markers or audio tracks.

Reference: Microsoft's MIDL documentation identifies compiler version 1100 with Visual C++ 5.0: https://learn.microsoft.com/en-us/windows/win32/midl/-msc-ver . This documents version nomenclature, not proof of this binary's exact compiler.
