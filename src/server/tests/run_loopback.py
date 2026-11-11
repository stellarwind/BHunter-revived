"""Build and run the SDL-free dedicated session check using 32-bit GCC."""

import os
import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


def main():
    root = Path(__file__).resolve().parents[3]
    windows = os.name == "nt"
    if not windows and not sys.platform.startswith("linux"):
        raise SystemExit("The loopback check supports Windows and Linux.")

    enet = root / "third_party/enet"
    platform = "win32" if windows else "unix"
    flags = [] if windows else [
        "-m32", "-march=i686", "-mfpmath=387", "-include", "src/bh_linux_compat.h",
    ]
    libraries = ["-lwinmm", "-lws2_32"] if windows else []
    with TemporaryDirectory() as output:
        executable = Path(output) / ("session-loopback.exe" if windows else "session-loopback")
        subprocess.run(
            [
                os.environ.get("BH_GCC", "gcc"), "-std=gnu99", "-O2", "-flto",
                "-fno-strict-aliasing", *flags, "-I", str(enet / "include"),
                "src/server/tests/session_loopback.c", "src/server/bh_server_session.c",
                "src/bh_network_game.c", "src/bh_network_codec.c",
                *(str(source) for source in sorted(enet.glob("*.c"))
                  if source.stem not in {"win32", "unix"} or source.stem == platform),
                *libraries, "-o", str(executable),
            ],
            cwd=root, check=True,
        )
        subprocess.run([str(executable)], check=True, timeout=30)


if __name__ == "__main__":
    main()
