"""Package only the game runtime, launchers, readme, screenshot and licenses."""
import argparse
import hashlib
import json
import re
import zipfile
from pathlib import Path
from prepare_ddrawcompat import prepare

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default="0.1")
    args = parser.parse_args()
    if not re.fullmatch(r"\d+(?:\.\d+){1,2}", args.version):
        parser.error("Version must contain two or three numeric components")
    runtime = ROOT / "build/game"
    executable = runtime / "BHunter-source.exe"
    record = json.loads((runtime / "build.json").read_text())
    if not record.get("passed") or record.get("debugging") or record.get("mission_profile"):
        parser.error("Build a successful release configuration with build-source.cmd first")
    if record.get("executable_sha256") != hashlib.sha256(executable.read_bytes()).hexdigest():
        parser.error("Executable differs from the recorded build")
    for name, digest in record["sources"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != digest:
            parser.error(f"Source changed since build: {name}")
    prepare()
    output = ROOT / f"dist/BHunter-{args.version}-windows-x86.zip"
    output.parent.mkdir(parents=True, exist_ok=True)
    prefix = f"BHunter-{args.version}/"
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as bundle:
        for name in ("BHunter-source.exe", "ddraw.dll", "DDrawCompat-LICENSE.txt"):
            bundle.write(runtime / name, prefix + "build/game/" + name)
        for name in ("README.md", "LICENSE", "play-source.cmd", "play-hardware.cmd", "play-software.cmd", "tools/unlock_levels.py"):
            bundle.write(ROOT / name, prefix + name)
        for screenshot in sorted((ROOT / "screenshots").rglob("*.jpg")):
            bundle.write(screenshot, prefix + screenshot.relative_to(ROOT).as_posix())
        bundle.writestr(prefix + "GAME_DATA/", b"")
        bundle.writestr(prefix + "THIRD-PARTY.txt",
                        "DDrawCompat 0.7.2 (unmodified official binary)\n"
                        "License: build/game/DDrawCompat-LICENSE.txt\n"
                        "Corresponding source: https://github.com/narzoul/DDrawCompat/tree/v0.7.2\n"
                        "Source archive: https://github.com/narzoul/DDrawCompat/archive/refs/tags/v0.7.2.zip\n")
    digest = hashlib.sha256(output.read_bytes()).hexdigest()
    checksum = output.with_suffix(output.suffix + ".sha256")
    checksum.write_text(f"{digest}  {output.name}\n", encoding="ascii")
    print(output)
    print(digest)


if __name__ == "__main__":
    main()
