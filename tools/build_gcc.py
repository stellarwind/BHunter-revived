"""Build the restored game for 32-bit x86 Windows or Linux."""

import argparse
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

from prepare_sdl3 import INI_FLAGS, INI_SOURCE, LICENCE_SOURCES, SHADER_PATH, STB_VORBIS_SOURCE, prepare, stage_licences

ROOT = Path(__file__).resolve().parents[1]


def stage_runtime(out):
    """Stage only game assets, leaving saves and player settings untouched."""
    staged = {}
    for folder, name in (("GAME_DATA", "Bhunter.bin"), ("MUSIC_DATA", "track02.ogg"), ("MUSIC_DATA", "track02.wav")):
        target = out / folder / name
        candidates = [ROOT / folder / name, ROOT / "build" / folder / name]
        source = next(
            (path for path in candidates if path.is_file() and path.stat().st_size),
            None,
        )
        if source:
            target.parent.mkdir(parents=True, exist_ok=True)
            if source.resolve() != target.resolve() and (
                not target.exists()
                or (source.stat().st_size, source.stat().st_mtime_ns)
                != (target.stat().st_size, target.stat().st_mtime_ns)
            ):
                shutil.copy2(source, target)
            staged[folder + "/" + name] = str(source)
        elif target.is_file() and target.stat().st_size:
            staged[folder + "/" + name] = str(target)
        elif folder == "GAME_DATA":
            locations = ", ".join(str(path) for path in candidates + [target])
            raise FileNotFoundError(
                "Required game archive Bhunter.bin is missing or empty. "
                "Copy your game's Bhunter.bin to one of these locations and rerun "
                f"the build: {locations}"
            )
    return staged


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    linux = sys.platform.startswith("linux")
    if not linux and os.name != "nt":
        parser.error("The game build supports 32-bit x86 Windows and Linux")
    parser.add_argument("--cc", default=os.environ.get("BH_GCC", "gcc"))
    parser.add_argument("--cxx", default=os.environ.get("BH_GXX", "g++"))
    parser.add_argument(
        "--output-dir", type=Path, default=Path("build/linux-x86" if linux else "build")
    )
    parser.add_argument("--incremental", action="store_true")
    parser.add_argument(
        "--symbols",
        action="store_true",
        help="Include debugger symbols independently of DEBUGGING",
    )
    parser.add_argument("--compile-one", help="Compile one source without linking")
    parser.add_argument(
        "--debugging",
        action="store_true",
        help="Enable DEBUGGING logs and the framerate profiler",
    )
    cd = parser.add_mutually_exclusive_group()
    cd.add_argument("--skip-cd-check", dest="skip_cd_check", action="store_true")
    cd.add_argument("--enable-cd-audio", dest="skip_cd_check", action="store_false")
    parser.set_defaults(skip_cd_check=True)
    args = parser.parse_args()
    executable = "BHunter-source" if linux else "BHunter-source.exe"
    architecture = ["-m32", "-march=i686", "-mfpmath=387"] if linux else []
    compiler_target = r"(?:i[3-6]86|x86_64)-.*linux-.*" if linux else r"i[3-6]86-w64-mingw32"
    for name in ("cc", "cxx"):
        compiler = shutil.which(getattr(args, name))
        if not compiler:
            parser.error(f"Compiler not found: {getattr(args, name)}")
        setattr(args, name, str(Path(compiler).resolve()))
        target = subprocess.check_output([compiler, "-dumpmachine"], text=True).strip()
        if not re.fullmatch(compiler_target, target):
            parser.error(f"An x86 {'Linux' if linux else '32-bit MinGW-w64'} compiler is required; got {target}")

    out = (ROOT / args.output_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)
    cache_dir = (
        ROOT / "build-cache/gcc" / hashlib.sha256(str(out).encode()).hexdigest()[:12]
    )
    objects_dir = cache_dir / "obj"
    objects_dir.mkdir(parents=True, exist_ok=True)
    record = {
        "passed": False,
        "commands": [],
        "target": str(out / executable),
        "toolchain": "gcc",
        "debugging": args.debugging,
        "symbols": args.symbols,
        "skip_cd_audio_check": args.skip_cd_check,
        "original_code_required": False,
        "compiler": {
            "cc": args.cc,
            "cxx": args.cxx,
            "cc_version": subprocess.check_output([args.cc, "--version"], text=True),
            "cxx_version": subprocess.check_output([args.cxx, "--version"], text=True),
        },
    }
    embedded_config = ROOT / "src/bh_config_defaults.bin"
    dependencies = [
        path
        for path in (ROOT / "src").rglob("*")
        if path.is_file() and path.suffix in {".c", ".cpp", ".h", ".inc", ".slang"}
    ]
    # ENet 1.3.18, upstream commit 2662c0de09e36f2a2030ccc2c528a3e4c9e8138a.
    enet = ROOT / "third_party/enet"
    dependencies += sorted(enet.rglob("*.h")) + sorted(enet.glob("*.c"))
    dependencies += [
        INI_SOURCE,
        INI_SOURCE.with_suffix(".h"),
        STB_VORBIS_SOURCE,
        *(path for _, path in LICENCE_SOURCES),
        embedded_config,
        ROOT / "tools/build_game.py",
        ROOT / "tools/build_gcc.py",
        ROOT / "tools/prepare_sdl3.py",
        ROOT / "tools/game_compile_context.json",
    ]
    record["sources"] = {
        path.relative_to(ROOT).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(dependencies)
    }
    if not args.compile_one:
        try:
            record["assets"] = stage_runtime(out)
        except (OSError, shutil.Error) as error:
            record["error"] = str(error)
            (cache_dir / "build.json").write_text(json.dumps(record, indent=2) + "\n")
            parser.error(str(error))
    cache_path = cache_dir / "objects.json"
    cache = json.loads(cache_path.read_text()) if cache_path.exists() else {}

    def run(argv):
        command = [str(item) for item in argv]
        result = subprocess.run(
            command, cwd=ROOT, text=True, capture_output=True, check=False
        )
        record["commands"].append(
            {
                "argv": command,
                "exit_code": result.returncode,
                "output": result.stdout + result.stderr,
            }
        )
        (cache_dir / "build.json").write_text(json.dumps(record, indent=2) + "\n")
        if result.returncode:
            print(result.stdout + result.stderr)
            raise SystemExit(result.returncode)

    contexts = json.loads((ROOT / "tools/game_compile_context.json").read_text())[
        "sources"
    ]
    sources = (
        [ROOT / args.compile_one]
        if args.compile_one
        else [
            ROOT / "src" / name
            for name in sorted(
                set(contexts) | {"bh_game_app.c", "bh_game_main.c"} | ({"bh_random.c"} if linux else set())
            )
        ]
    )
    if not args.compile_one:
        sources.extend(
            enet / (name + ".c")
            for name in (
                "callbacks",
                "compress",
                "host",
                "list",
                "packet",
                "peer",
                "protocol",
                "unix" if linux else "win32",
            )
        )
    headers = hashlib.sha256(
        b"".join(
            path.read_bytes()
            for path in sorted([*(ROOT / "src").rglob("*"), *enet.rglob("*.h")])
            if path.suffix in {".h", ".inc"}
        ) + embedded_config.read_bytes()
    ).hexdigest()
    compiler_version = subprocess.check_output([args.cc, "--version"], text=True)
    objects = []
    for source in sources:
        obj = objects_dir / (source.stem + ".o")
        objects.append(obj)
        flags = []
        for flag in contexts.get(source.name, []):
            if flag.startswith(("/DDIRECT3D_VERSION=", "/DDIRECTDRAW_VERSION=")):
                continue
            if flag.startswith("/D"):
                flags.append("-D" + flag[2:])
            elif flag.replace("\\", "/") != "/I{DXSDK}/include":
                parser.error(f"Unsupported compilation context: {flag}")
        if re.search(
            r"IDirect(?:Draw|3D)\w*_\w+\(|DirectDraw(?:Create|EnumerateA)\(",
            source.read_text(),
        ):
            flags += [
                "-include",
                str(ROOT / "src/bh_sdl_adapter.h"),
            ]
        if args.debugging:
            flags.append("-DDEBUGGING")
        if args.skip_cd_check and source.name == "bh_menu_transition.c":
            flags.append("-DBH_SKIP_CD_AUDIO_CHECK")
        argv = [
            args.cc,
            "-std=gnu23",
            "-O2",
            *architecture,
            *(["-include", ROOT / "src/bh_linux_compat.h"] if linux else []),
            *(["-g"] if args.symbols else []),
            "-fno-strict-aliasing",
            "-fwrapv",
            "-ffunction-sections",
            "-fdata-sections",
            "-DBH_SDL_BACKEND",
            "-I" + str(enet / "include"),
            *flags,
            "-c",
            source,
            "-o",
            obj,
        ]
        signature = hashlib.sha256(
            (headers + compiler_version + repr([str(item) for item in argv])).encode()
            + source.read_bytes()
        ).hexdigest()
        entry = cache.get(source.name, {})
        if not (
            args.incremental
            and obj.exists()
            and entry.get("signature") == signature
            and entry.get("sha256") == hashlib.sha256(obj.read_bytes()).hexdigest()
        ):
            run(argv)
            cache[source.name] = {
                "signature": signature,
                "sha256": hashlib.sha256(obj.read_bytes()).hexdigest(),
            }
            cache_path.write_text(json.dumps(cache, indent=2) + "\n")
    if args.compile_one:
        return
    response = cache_dir / "objects.rsp"
    response.write_text(
        "\n".join('"' + str(obj).replace("\\", "/") + '"' for obj in objects) + "\n"
    )
    runtime_files = ["LICENCES"]
    if linux:
        stage_licences(out)
        sdl_flags = shlex.split(
            subprocess.check_output(["pkg-config", "--cflags", "--libs", "sdl3"], text=True)
        )
        run(
            [
                args.cxx,
                "-std=c++17",
                "-O2",
                *INI_FLAGS,
                *architecture,
                *(["-g"] if args.symbols else []),
                "-D__cdecl=",
                ROOT / "src/bh_sdl_gpu.cpp",
                ROOT / "src/bh_sdl_audio.cpp",
                ROOT / "src/bh_game_settings.cpp",
                INI_SOURCE,
                "@" + str(response),
                "-Wl,--gc-sections",
                *sdl_flags,
                "-o",
                out / executable,
            ]
        )
    else:
        gpu_import = prepare(out, gnu=True, compiler=args.cxx, symbols=args.symbols)
        run(
            [
                args.cc,
                "-mwindows",
                "-static",
                "-static-libgcc",
                "-Wl,--gc-sections",
                "@" + str(response),
                gpu_import,
                "-lwinmm",
                "-lws2_32",
                "-o",
                out / executable,
            ]
        )
        runtime_files += ["BHunterGPU.dll", "SDL3.dll", SHADER_PATH.as_posix()]
    objdump = subprocess.check_output(
        [args.cc, "-print-prog-name=objdump"], text=True
    ).strip()
    record["imports"] = {}
    for name in ([executable] if linux else [executable, "BHunterGPU.dll", "SDL3.dll"]):
        imports = subprocess.check_output([objdump, "-p", str(out / name)], text=True)
        libraries = re.findall(r"NEEDED\s+(\S+)" if linux else r"DLL Name:\s*(\S+)", imports)
        record["imports"][name] = libraries
        if any(
            re.match(r"lib(?:gcc|stdc\+\+|winpthread).*\.dll$", library, re.IGNORECASE)
            for library in libraries
        ):
            raise RuntimeError(
                f"Non-embedded GCC runtime dependency in {name}: {libraries}"
            )
    for name, digest in record["sources"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError(f"Source changed during build; rerun the build: {name}")
    record["executable_sha256"] = hashlib.sha256(
        (out / executable).read_bytes()
    ).hexdigest()
    record["runtime_sha256"] = {
        name: hashlib.sha256((out / name).read_bytes()).hexdigest()
        for name in runtime_files
    }
    record["passed"] = True
    record["translation_units"] = len(sources) + 3
    (cache_dir / "build.json").write_text(json.dumps(record, indent=2) + "\n")
    print(
        f"Built {out / executable} from {len(sources)} C translation units and the SDL adapters"
    )


if __name__ == "__main__":
    main()
