"""Fetch and stage pinned official SDL3 for 32-bit Windows builds."""

import hashlib
import os
import shutil
import subprocess
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = "3.4.18"
# Official release archive; digest is verified before extracting downloaded code.
SHA256 = "78d84602ae616cfe26b33a73b7d3b9b6b56e8a6650e53d92c9c6ca938f747acd"
WINDOWS_RUNTIME_FILES = ("SDL3.dll", "LICENCES")
# inih r62, upstream commit 26254ee9de7681f8825433415443e7116ff24b98.
INI_SOURCE = ROOT / "third_party/inih/ini.c"
# stb_vorbis 1.23, upstream commit d85119c15235cb3caa65fb357e694cbc17238952.
STB_VORBIS_SOURCE = ROOT / "third_party/stb/stb_vorbis.c"
# Indented keys are ordinary assignments in our settings files.
INI_FLAGS = ["-DINI_ALLOW_MULTILINE=0"]
LICENCE_SOURCES = (
    ("BHunter Revived", ROOT / "LICENSE"),
    ("SDL3", ROOT / "LICENSES/SDL3.txt"),
    ("ENet", ROOT / "third_party/enet/LICENSE"),
    ("inih", INI_SOURCE.with_name("LICENSE.txt")),
    ("stb_vorbis (MIT)", STB_VORBIS_SOURCE.with_name("LICENSE")),
)


def stage_licences(out):
    sections = [
        f"{title}\n{'=' * len(title)}\n".encode("ascii") + path.read_bytes()
        for title, path in LICENCE_SOURCES
    ]
    (out / "LICENCES").write_bytes(b"\n\n".join(sections))
    for name in ("SDL3-LICENSE.txt", "ENet-LICENSE.txt", "inih-LICENSE.txt", "ComicNeue-LICENSE.txt"):
        (out / name).unlink(missing_ok=True)


def windows_sdk():
    deps = ROOT / "toolchain/SDL3"
    deps.mkdir(parents=True, exist_ok=True)
    archive = deps / f"SDL3-devel-{VERSION}-VC.zip"
    if not archive.exists():
        urllib.request.urlretrieve(
            f"https://github.com/libsdl-org/SDL/releases/download/release-{VERSION}/"
            + archive.name,
            archive,
        )
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise RuntimeError("SDL3 archive SHA256 mismatch")
    sdk = deps / f"SDL3-{VERSION}"
    if not (sdk / "include/SDL3/SDL.h").exists():
        with zipfile.ZipFile(archive) as z:
            z.extractall(deps)
    return sdk


def stage_windows_runtime(out, sdk):
    shutil.copy2(sdk / "lib/x86/SDL3.dll", out / "SDL3.dll")
    stage_licences(out)
    for name in ("ddraw.dll", "DDrawCompat-LICENSE.txt", "shaders/legacy.hlsl"):
        (out / name).unlink(missing_ok=True)
    staged_shader = out / "shaders/legacy.slang"
    if staged_shader.parent.resolve() != (ROOT / "src/shaders").resolve():
        staged_shader.unlink(missing_ok=True)


def prepare(out, *, gnu=True, compiler=None, symbols=False):
    """Build and stage the separate adapter for legacy investigation tools."""
    out = Path(out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    sdk = windows_sdk()
    compiler = compiler or os.environ.get("BH_GPU_CXX", shutil.which("g++") or "g++")
    cache = ROOT / "build-cache/sdl3"
    if gnu:
        cache = cache / "gcc"
    cache.mkdir(parents=True, exist_ok=True)
    import_lib = cache / ("libBHunterGPU.dll.a" if gnu else "BHunterGPU.lib")
    sources = [
        ROOT / "src/bh_sdl_gpu.cpp",
        ROOT / "src/bh_sdl_audio.cpp",
        ROOT / "src/bh_game_settings.cpp",
    ]
    inputs = [
        *sources,
        ROOT / "src/bh_sdl_adapter.h",
        ROOT / "src/bh_render_types.h",
        ROOT / "src/bh_sdl_audio.h",
        ROOT / "src/bh_game_settings.h",
        ROOT / "src/shaders/bh_gpu_shaders.h",
        Path(__file__),
        ROOT / "src/bh_video_layout.h",
        ROOT / "src/bh_gamepad.h",
        ROOT / "src/bh_platform.h",
        ROOT / "src/bh_overlay_atlas.inc",
        ROOT / "src/bh_overlay_font.h",
        ROOT / "src/bh_overlay_text.h",
        INI_SOURCE,
        INI_SOURCE.with_suffix(".h"),
        STB_VORBIS_SOURCE,
    ]
    compiler_version = subprocess.check_output([compiler, "--version"])
    signature = hashlib.sha256(
        b"".join(p.read_bytes() for p in inputs)
        + VERSION.encode()
        + compiler.encode()
        + compiler_version
        + str(symbols).encode()
    ).hexdigest()
    stamp = cache / "build.sha256"
    cached_dll = cache / "BHunterGPU.dll"
    if (
        not stamp.exists()
        or stamp.read_text() != signature
        or not cached_dll.exists()
        or not import_lib.exists()
    ):
        definition = cache / "BHunterGPU.def"
        import re

        # Export definitions, not every function called by the adapter. Inline
        # portable helpers belong to the DLL implementation, not its public ABI.
        export_symbols = sorted(
            set(
                re.findall(
                    r"^    (?:\w+[ \t*]+)+(bh_\w+)\([^;{}]*\)\s*\{",
                    "\n".join(source.read_text() for source in sources),
                    re.MULTILINE,
                )
            )
        )
        definition.write_text(
            "LIBRARY BHunterGPU.dll\nEXPORTS\n" + "\n".join(export_symbols) + "\n"
        )
        cmd = [
            compiler,
            "-std=c++17",
            "-O2",
            *INI_FLAGS,
            *(["-g"] if symbols else []),
            "-shared",
            "-static",
            "-static-libgcc",
            "-static-libstdc++",
            "-I" + str(sdk / "include"),
            *map(str, sources),
            str(INI_SOURCE),
            str(sdk / "lib/x86/SDL3.lib"),
            "-o",
            str(cached_dll),
            str(definition),
        ]
        if gnu:
            cmd.append("-Wl,--out-implib," + str(import_lib))
        subprocess.run(cmd, check=True, cwd=ROOT)
        if not gnu:
            create_vc_import_library(cache, definition, import_lib)
        stamp.write_text(signature)
    shutil.copy2(cached_dll, out / "BHunterGPU.dll")
    stage_windows_runtime(out, sdk)
    return import_lib


def create_vc_import_library(cache, definition, import_lib):
    lib = (
        Path(os.environ.get("BH_VC_ROOT", "D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC"))
        / "bin/LIB.EXE"
    )
    env = dict(os.environ)
    env["PATH"] = os.pathsep.join(
        [
            env.get("BH_COMPILER_DIR", str(ROOT / "toolchain/vc5/bin")),
            env.get("BH_SHARED_DIR", str(lib.parent.parent.parent / "SHAREDIDE/BIN")),
            env.get("PATH", ""),
        ]
    )
    subprocess.run(
        [
            str(lib),
            "/nologo",
            "/machine:ix86",
            "/def:" + str(definition),
            "/out:" + str(import_lib),
        ],
        check=True,
        cwd=cache,
        env=env,
    )


if __name__ == "__main__":
    import sys

    out = Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "build").resolve()
    out.mkdir(parents=True, exist_ok=True)
    sdk = windows_sdk()
    stage_windows_runtime(out, sdk)
    print(sdk)
