"""Compare production Comic Sans output with original GDI settings offscreen."""

import argparse
import hashlib
import json
import math
import shutil
import subprocess
import sys
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from bake_comic_font import close, offscreen

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--runtime", type=Path, default=ROOT / "build")
args = parser.parse_args()
runtime = args.runtime.resolve()
out = ROOT / "harness_tools/output/portability-step2/comic-comparison"
out.mkdir(parents=True, exist_ok=True)
(out / "fonts").mkdir(exist_ok=True)
for name in ("BHunterGPU.dll", "SDL3.dll"):
    shutil.copy2(runtime / name, out / name)
shutil.copy2(runtime / "fonts/comic-sans-ms-24.bhf", out / "fonts/comic-sans-ms-24.bhf")
cache = (
    ROOT / "build-cache/gcc" / hashlib.sha256(str(runtime).encode()).hexdigest()[:12]
)
record = json.loads((cache / "build.json").read_text())
objects = [
    line.strip('"')
    for line in (cache / "objects.rsp").read_text().splitlines()
    if Path(line.strip('"')).stem in {"bh_gdi_text", "bh_draw_queue"}
]
command = [
    record["compiler"]["cc"],
    "-std=gnu99",
    "-O2",
    "-static",
    "-static-libgcc",
    "-ffunction-sections",
    "-fdata-sections",
    "-DBH_SDL_BACKEND",
    "-DDIRECT3D_VERSION=0x0500",
    "-DDIRECTDRAW_VERSION=0x0500",
    "-I" + str(ROOT / "src"),
    ROOT / "harness_tools/comic_text_probe.c",
    *objects,
    ROOT / "build-cache/sdl3/gcc/libBHunterGPU.dll.a",
    "-Wl,--gc-sections",
    "-o",
    out / "comic-text-probe.exe",
]
subprocess.run([str(arg) for arg in command], check=True, cwd=ROOT)
cases = [
    ("manual-cue-cycle-start", 8, 8, 0x80FFFF),
    ("PORTABILITY REGRESSION: INPUT / AUDIO / TEXT", 8, 8, 0x80FFFF),
    ("No objects, press 'N' to create new", 0, 0, 0xFF0000),
    ("TARGET 5000   SUSPECT -5000", 20, 20, 0xFFFFFF),
    ("AVATAR To ffi ill 0123456789", 0, 0, 0xFFFFFF),
    ("left/right positioned sound", -7, 3, 0x123456),
    ("clipped text", 610, 3, 0x00FF00),
    ("top clipping", 20, -9, 0xFF8000),
    ("lower clipping", 0, 85, 0x808080),
    ("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 4, 40, 0xFFFFFF),
    ("abcdefghijklmnopqrstuvwxyz", 4, 40, 0xFFFFFF),
    ("!@#$%^&*()[]{};:,.?+-=_", 4, 40, 0xFFFFFF),
]
results = []
for index, (text, x, y, rgb) in enumerate(cases):
    native = offscreen(640, 96)
    try:
        gdi, dc, *_, memory = native
        colorref = (rgb & 255) << 16 | (rgb & 0xFF00) | (rgb >> 16 & 255)
        gdi.SetTextColor(dc, colorref)
        raw = text.encode("cp1252")
        if not gdi.TextOutA(dc, x, y, raw, len(raw)):
            raise OSError("Reference TextOutA failed")
        gdi.GdiFlush()
        expected = Image.frombytes("RGB", (640, 96), bytes(memory), "raw", "BGRX")
    finally:
        close(native)
    raw_path = out / f"case-{index}.raw"
    subprocess.run(
        [
            str(out / "comic-text-probe.exe"),
            str(raw_path),
            str(x),
            str(y),
            f"{rgb:x}",
            raw.hex(),
        ],
        cwd=out,
        check=True,
    )
    actual = Image.frombytes("RGB", (640, 96), raw_path.read_bytes(), "raw", "BGRX")
    differences = [a - b for a, b in zip(actual.tobytes(), expected.tobytes())]
    differing = sum(any(differences[i : i + 3]) for i in range(0, len(differences), 3))
    rmse = math.sqrt(sum(value * value for value in differences) / len(differences))
    result = {
        "text": text,
        "position": [x, y],
        "differing_pixels": differing,
        "rmse": rmse,
        "passed": differing == 0,
    }
    results.append(result)
    expected.save(out / f"case-{index}-original.png")
    actual.save(out / f"case-{index}-portable.png")
(out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(json.dumps(results, indent=2))
raise SystemExit(not all(result["passed"] for result in results))
