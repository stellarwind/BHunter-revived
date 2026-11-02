"""Run SDL virtual-gamepad and original keyboard packet regressions."""

import os
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / "toolchain/SDL3/SDL3-3.4.18"
CACHE = ROOT / "build-cache/gamepad-tests"


def run(*args):
    subprocess.run([str(arg) for arg in args], cwd=CACHE, check=True)


def main():
    CACHE.mkdir(parents=True, exist_ok=True)
    cxx = Path(os.environ.get("BH_GPU_CXX", shutil.which("g++") or "g++"))
    cc = cxx.with_name("gcc.exe")
    sources = [
        ROOT / "src" / name
        for name in (
            "bh_player_input.c",
            "bh_input_evaluate.c",
            "bh_input_collection.c",
            "bh_input_devices.c",
            "bh_round.c",
        )
    ]
    run(cc, "-std=c99", "-O2", "-DBH_SDL_BACKEND", "-c", *sources)
    target = CACHE / "gamepad-tests.exe"
    run(
        cxx,
        "-std=c++17",
        "-O2",
        "-static",
        "-DDIRECT3D_VERSION=0x0500",
        "-DDIRECTDRAW_VERSION=0x0500",
        "-I" + str(SDK / "include"),
        ROOT / "tools/gamepad_test.cpp",
        *(CACHE / (source.stem + ".o") for source in sources),
        SDK / "lib/x86/SDL3.lib",
        "-ld3dcompiler_47",
        "-luser32",
        "-lgdi32",
        "-lwinmm",
        "-o",
        target,
    )
    shutil.copy2(SDK / "lib/x86/SDL3.dll", CACHE / "SDL3.dll")
    run(target)
    # The original evaluator still passes with gamepad fields left at zero.
    # The restoration test corpus is local and intentionally excluded from Git.
    if not (ROOT / "tests/input_evaluate.c").exists():
        return
    keyboard = CACHE / "keyboard-tests.exe"
    run(
        cc,
        "-std=c99",
        "-O2",
        "-DBH_INPUT_EVALUATE_STANDALONE",
        ROOT / "tests/input_evaluate.c",
        ROOT / "src/bh_player_input.c",
        ROOT / "src/bh_input_evaluate.c",
        "-o",
        keyboard,
    )
    run(keyboard)


if __name__ == "__main__":
    main()
