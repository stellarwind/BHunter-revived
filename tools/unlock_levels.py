"""Unlock all nine mission-menu entries in a BHunter source-build save."""
import argparse
import csv
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--save", type=Path, default=ROOT / "build/game/source-preview.cfg",
                        help="Save/configuration file (default: build/game/source-preview.cfg)")
    args = parser.parse_args()
    if os.name == "nt":
        processes = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq BHunter-source.exe", "/FO", "CSV", "/NH"],
            capture_output=True, text=True, check=True)
        if any(row and row[0].lower() == "bhunter-source.exe" for row in csv.reader(processes.stdout.splitlines())):
            parser.error("Quit BHunter before editing its save.")
    path = args.save.resolve()
    data = path.read_bytes()
    count, remainder = divmod(len(data) - 416, 76)
    if remainder or not 1 <= count <= 4096 or b"\0" not in data[12:140]:
        parser.error("Not a supported BHunter save (416-byte settings plus 76-byte input profiles).")
    if any(b"\0" not in data[416 + i * 76 + 12:416 + i * 76 + 28] for i in range(count)):
        parser.error("Invalid input-profile name in save.")
    # bh_mission_progress writes this signed little-endian word; the mission
    # menu enables rows 0..8 when their index is <= this value.
    unlocked = struct.unpack_from("<h", data, 2)[0]
    if unlocked >= 8:
        print(f"All nine mission entries are already unlocked: {path}")
        return
    if unlocked < 0:
        parser.error("Unexpected negative mission progression; save left unchanged.")
    patched = bytearray(data)
    struct.pack_into("<h", patched, 2, 8)
    backup = path.with_name(path.name + f".backup-{time.time_ns()}")
    with backup.open("xb") as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=path.parent, prefix=path.name + ".", suffix=".tmp", delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(patched)
            stream.flush()
            os.fsync(stream.fileno())
        if path.read_bytes() != data:
            raise RuntimeError("Save changed while preparing the update; refusing to replace it.")
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    print(f"Unlocked all nine mission entries: {path}")
    print(f"Backup: {backup}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        raise SystemExit(str(error))
