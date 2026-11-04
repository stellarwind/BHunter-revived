"""Fetch pinned official SDL3 and build the x86 SDL GPU adapter with MinGW-w64."""

import hashlib
import os
import shutil
import subprocess
import urllib.request
import zipfile
from pathlib import Path

from bake_comic_font import prepare as prepare_comic_font

ROOT = Path(__file__).resolve().parents[1]
VERSION = "3.4.18"
# Official release archive; digest is verified before extracting downloaded code.
SHA256 = "78d84602ae616cfe26b33a73b7d3b9b6b56e8a6650e53d92c9c6ca938f747acd"


def prepare(out, *, gnu=True, compiler=None, symbols=False):
    out = Path(out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    prepare_comic_font(out)
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
    compiler = compiler or os.environ.get("BH_GPU_CXX", shutil.which("g++") or "g++")
    cache = ROOT / "build-cache/sdl3"
    if gnu:
        cache = cache / "gcc"
    cache.mkdir(parents=True, exist_ok=True)
    import_lib = cache / ("libBHunterGPU.dll.a" if gnu else "BHunterGPU.lib")
    inputs = [
        ROOT / "src/bh_sdl_gpu.cpp",
        ROOT / "src/bh_sdl_adapter.h",
        ROOT / "src/shaders/legacy.hlsl",
        Path(__file__),
        ROOT / "src/bh_video_layout.h",
        ROOT / "src/bh_gamepad.h",
        ROOT / "src/bh_platform.h",
        ROOT / "src/bh_overlay_font.h",
        ROOT / "src/bh_overlay_text.h",
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
                    inputs[0].read_text(),
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
            *(["-g"] if symbols else []),
            "-shared",
            "-static",
            "-static-libgcc",
            "-static-libstdc++",
            "-DDIRECT3D_VERSION=0x0500",
            "-DDIRECTDRAW_VERSION=0x0500",
            "-I" + str(sdk / "include"),
            str(inputs[0]),
            str(sdk / "lib/x86/SDL3.lib"),
            "-ld3dcompiler_47",
            "-luser32",
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
    shutil.copy2(sdk / "lib/x86/SDL3.dll", out / "SDL3.dll")
    shutil.copy2(sdk / "LICENSE.txt", out / "SDL3-LICENSE.txt")
    (out / "shaders").mkdir(exist_ok=True)
    shutil.copy2(ROOT / "src/shaders/legacy.hlsl", out / "shaders/legacy.hlsl")
    # Remove only the obsolete staged runtime files, never game assets or saves.
    for name in ("ddraw.dll", "DDrawCompat-LICENSE.txt"):
        (out / name).unlink(missing_ok=True)
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

    print(prepare(Path(sys.argv[1] if len(sys.argv) > 1 else ROOT / "build")))
