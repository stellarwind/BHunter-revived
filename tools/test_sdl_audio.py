"""Run deterministic SDL audio and scheduler regressions with the bundled GCC."""
import os
import shutil
from prepare_sdl3 import VERSION
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "toolchain/w64devkit/bin"
SDK = ROOT / "toolchain/SDL3" / ("SDL3-" + VERSION)
OUT = ROOT / "TESTS/build/sdl-audio"
OUT.mkdir(parents=True, exist_ok=True)
env = dict(os.environ)
env["PATH"] = str(BIN) + os.pathsep + env["PATH"]
CC = os.environ.get("BH_GCC") or shutil.which("gcc", path=env["PATH"]) or "gcc"
CXX = os.environ.get("BH_GXX") or shutil.which("g++", path=env["PATH"]) or "g++"
def run(args):
    subprocess.run([str(x) for x in args], cwd=OUT, env=env, check=True)
objects = []
for name in ["bh_sound_queue", "bh_sound_tick", "bh_fp", "bh_round", "bh_cd_audio", "bh_cd_notify", "bh_cd_close", "bh_media_control"]:
    obj = OUT / (name + ".o")
    run([CC, "-std=gnu99", "-O2", "-ffunction-sections", "-fdata-sections", "-c", ROOT / "src" / (name + ".c"), "-o", obj])
    objects.append(obj)
run([CXX, "-std=c++17", "-O2", "-static", "-I" + str(SDK / "include"), ROOT / "tools/audio_tests/sdl_audio.cpp", *objects, SDK / "lib/x86/SDL3.lib", "-Wl,--gc-sections", "-o", OUT / "sdl-audio.exe"])
shutil.copy2(SDK / "lib/x86/SDL3.dll", OUT / "SDL3.dll")
run([OUT / "sdl-audio.exe"])
