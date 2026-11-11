"""Build the SDL-free BHunter dedicated server for 32-bit Windows or Linux."""

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=os.environ.get("BH_GCC", "gcc"))
    parser.add_argument("--output-dir", type=Path, default=ROOT / "build_server")
    args = parser.parse_args()

    windows = os.name == "nt"
    if not windows and not sys.platform.startswith("linux"):
        parser.error("The server build supports Windows and Linux.")
    cc = shutil.which(args.cc)
    if not cc:
        parser.error(f"Compiler not found: {args.cc}")
    target = subprocess.check_output([cc, "-dumpmachine"], text=True).strip()
    expected = r"i[3-6]86-w64-mingw32" if windows else r"(?:i[3-6]86|x86_64)-.*linux-.*"
    if not re.fullmatch(expected, target):
        parser.error(f"An x86 {'32-bit MinGW-w64' if windows else 'Linux'} compiler is required. Found {target}.")

    enet = ROOT / "third_party/enet"
    platform = "win32" if windows else "unix"
    architecture = [] if windows else [
        "-m32", "-march=i686", "-mfpmath=387", "-include", "src/bh_linux_compat.h",
    ]
    libraries = ["-lwinmm", "-lws2_32"] if windows else []
    out = (ROOT / args.output_dir).resolve()
    out.mkdir(parents=True, exist_ok=True)
    executable = out / ("BHunter-server.exe" if windows else "BHunter-server")
    with TemporaryDirectory(dir=out) as scratch:
        candidate = Path(scratch) / executable.name
        subprocess.run(
            [
                cc, "-std=gnu99", "-O2", "-flto", "-fno-strict-aliasing", *architecture,
                "-I", str(enet / "include"),
                "src/server/bh_server_main.c", "src/server/bh_server_session.c",
                "src/bh_network_game.c", "src/bh_network_codec.c", "src/bh_vfile.c",
                *(str(enet / f"{name}.c") for name in (
                    "callbacks", "compress", "host", "list", "packet", "peer", "protocol", platform,
                )),
                *libraries, "-o", str(candidate),
            ],
            cwd=ROOT, check=True,
        )
        candidate.replace(executable)
    print(f"Built {executable}")


if __name__ == "__main__":
    main()
