"""Package only the game runtime, launchers, readme, screenshot and licenses."""
import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path
from prepare_sdl3 import prepare

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default="0.2")
    args = parser.parse_args()
    if not re.fullmatch(r"\d+(?:\.\d+){1,2}", args.version):
        parser.error("Version must contain two or three numeric components")
    runtime = ROOT / "build"
    executable = runtime / "BHunter-source.exe"
    record = json.loads((ROOT / "build-cache/build.json").read_text())
    if not record.get("passed") or record.get("debugging") or record.get("mission_profile"):
        parser.error("Build a successful release configuration with build-source.cmd first")
    if record.get("executable_sha256") != hashlib.sha256(executable.read_bytes()).hexdigest():
        parser.error("Executable differs from the recorded build")
    for name, digest in record["sources"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != digest:
            parser.error(f"Source changed since build: {name}")
    prepare(runtime)
    for name, digest in record.get("runtime_sha256", {}).items():
        if hashlib.sha256((runtime / name).read_bytes()).hexdigest() != digest:
            parser.error(f"Runtime changed since build: {name}")
    output = ROOT / f"dist/BHunter-{args.version}-windows-x86.zip"
    output.parent.mkdir(parents=True, exist_ok=True)
    prefix = f"BHunter-{args.version}/"
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as bundle:
        for name in ("BHunter-source.exe", "BHunterGPU.dll", "SDL3.dll", "SDL3-LICENSE.txt", "shaders/legacy.hlsl"):
            bundle.write(runtime / name, prefix + name)
        for name in ("README.md", "HOWTOPLAY.md", "LICENSE", "tools/unlock_levels.py", "assets/cover.png"):
            bundle.write(ROOT / name, prefix + name)
        for screenshot in sorted((ROOT / "screenshots").rglob("*.jpg")):
            bundle.write(screenshot, prefix + screenshot.relative_to(ROOT).as_posix())
        bundle.writestr(prefix + "GAME_DATA/", b"")
        bundle.writestr(prefix + "MUSIC_DATA/", b"")
        bundle.writestr(prefix + "THIRD-PARTY.txt",
                        "SDL 3.4.18 (unmodified official x86 binary)\n"
                        "License: SDL3-LICENSE.txt\n"
                        "Source: https://github.com/libsdl-org/SDL/tree/release-3.4.18\n")
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    checksum = output.with_suffix(output.suffix + ".sha256")
    checksum.write_text(f"{digest}  {output.name}\n", encoding="ascii")
    print(output)
    print(digest)


if __name__ == "__main__":
    main()
