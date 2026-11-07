"""Build BHunter release archives and back up superseded downloads."""

import argparse
from datetime import datetime, timezone
import hashlib
import io
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys
import tarfile
import tempfile
import zipfile

from prepare_sdl3_linux import prepare as prepare_linux_sdl, validate_runtime

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / "dist"
VERSION = r"[0-9]+(?:\.[0-9]+){1,2}"
RELEASE = re.compile(
    rf"BHunter-(?P<version>{VERSION})-(?:windows-x86\.zip|linux-x86\.tar\.gz)(?:\.sha256)?"
)

LINUX_LAUNCHER = b"""#!/bin/sh
set -eu
cd -- "$(dirname -- "$0")"
mkdir -p -- settings
export LD_LIBRARY_PATH="$PWD/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_VIDEODRIVER=x11
exec ./BHunter-source --asset-dir "$PWD" --settings-dir "$PWD/settings" "$@" > "$PWD/last-run.log" 2>&1
"""

WSL_BUILD = r"""
set -euo pipefail
root=$(realpath -e -- "$1")
test "$root" != /
test -x "$root/usr/bin/python3"
workspace="$root/workspace-release"
mkdir -p -- "$workspace"
test "$(realpath -e -- "$workspace")" = "$workspace"
exec 9<"$workspace"
flock -x 9
for directory in src tools third_party; do
    mkdir -p -- "$workspace/$directory"
    test "$(realpath -e -- "$workspace/$directory")" = "$workspace/$directory"
    rsync -rt --delete --exclude=__pycache__/ -- "$2/$directory/" "$workspace/$directory/"
done
mkdir -p -- "$workspace/GAME_DATA"
test "$(realpath -e -- "$workspace/GAME_DATA")" = "$workspace/GAME_DATA"
cp -- "$3" "$workspace/GAME_DATA/Bhunter.bin"
chroot "$root" /bin/sh -ec 'cd /workspace-release; python3 tools/prepare_sdl3_linux.py; export PKG_CONFIG_PATH="$PWD/build-cache/sdl3/linux/install/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"; exec python3 tools/build_gcc.py --incremental'
runtime="$workspace/build/linux-x86"
cp -- "$runtime/BHunter-source" "$runtime/ENet-LICENSE.txt" "$4/"
cp -- "$workspace/build-cache/sdl3/linux/install/lib/libSDL3.so.0" "$4/libSDL3.so.0"
"""


def build_native():
    subprocess.run(
        [sys.executable, str(ROOT / "tools/build_gcc.py"), "--incremental"],
        cwd=ROOT, check=True,
    )


def build_linux_wsl(chroot, distro, runtime):
    wsl = ["wsl.exe", *(["--distribution", distro] if distro else []), "--user", "root", "--exec"]

    def linux_path(path):
        return subprocess.check_output([*wsl, "wslpath", "-a", str(path)], text=True).strip()

    archive = next(
        (path for path in (ROOT / "GAME_DATA/Bhunter.bin", ROOT / "build/GAME_DATA/Bhunter.bin")
         if path.is_file() and path.stat().st_size),
        None,
    )
    if archive is None:
        raise FileNotFoundError("Copy Bhunter.bin to GAME_DATA or build/GAME_DATA first")
    runtime.mkdir()
    subprocess.run(
        [*wsl, "bash", "-s", "--", chroot, linux_path(ROOT), linux_path(archive), linux_path(runtime)],
        input=WSL_BUILD.encode(), check=True,
    )


def package_windows(version, stage):
    runtime = ROOT / "build"
    name = f"BHunter-{version}"
    archive = stage / f"{name}-windows-x86.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as bundle:
        for member in (
            "BHunter-source.exe", "BHunterGPU.dll", "SDL3.dll", "SDL3-LICENSE.txt",
            "ENet-LICENSE.txt", "shaders/legacy.slang",
        ):
            bundle.write(runtime / member, f"{name}/{member}")
    return archive


def package_linux(version, stage, runtime, sdl):
    validate_runtime(runtime / "BHunter-source")
    validate_runtime(sdl, sdl=True)
    name = f"BHunter-{version}-linux-x86"
    archive = stage / f"{name}.tar.gz"
    with tarfile.open(archive, "w:gz") as bundle:
        for member, data, mode in (
            ("BHunter-source", (runtime / "BHunter-source").read_bytes(), 0o755),
            ("lib/libSDL3.so.0", sdl.read_bytes(), 0o755),
            ("SDL3-LICENSE.txt", (ROOT / "LICENSES/SDL3.txt").read_bytes(), 0o644),
            ("ENet-LICENSE.txt", (runtime / "ENet-LICENSE.txt").read_bytes(), 0o644),
            ("run.sh", LINUX_LAUNCHER, 0o755),
        ):
            info = tarfile.TarInfo(f"{name}/{member}")
            info.mode, info.size = mode, len(data)
            bundle.addfile(info, io.BytesIO(data))
    return archive


def publish(version, files):
    names = {file.name for file in files}
    superseded = []
    for path in sorted(DIST.iterdir()):
        match = RELEASE.fullmatch(path.name)
        if match and (match["version"] != version or path.name in names):
            if not path.is_file() or path.is_symlink():
                raise OSError(f"Expected a regular release file: {path}")
            superseded.append(path)

    backup = None
    saved, installed = [], []
    try:
        if superseded:
            backup_root = DIST / "bkp"
            backup_root.mkdir(exist_ok=True)
            stamp = datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%SZ")
            backup = Path(tempfile.mkdtemp(prefix=f"{stamp}-", dir=backup_root))
            for path in superseded:
                path.replace(backup / path.name)
                saved.append(path)
        for file in files:
            file.replace(DIST / file.name)
            installed.append(file)
    except BaseException:
        for file in reversed(installed):
            (DIST / file.name).replace(file)
        for path in reversed(saved):
            (backup / path.name).replace(path)
        if backup is not None:
            backup.rmdir()
        raise
    if backup is not None:
        print(f"Moved {len(saved)} prior release files to {backup}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="Release version, for example 0.5 or 0.5.1")
    parser.add_argument(
        "--linux-chroot", default=os.environ.get("BH_RELEASE_LINUX_CHROOT"),
        help="On Windows, also build Linux in an existing x86 WSL chroot",
    )
    parser.add_argument("--wsl-distro", help="WSL distribution that contains the chroot")
    args = parser.parse_args()
    if not re.fullmatch(VERSION, args.version):
        parser.error("Supply a version with two or three numeric components")
    if os.name != "nt" and not sys.platform.startswith("linux"):
        parser.error("Release builds require Windows or Linux")
    if args.linux_chroot:
        chroot = PurePosixPath(args.linux_chroot)
        if os.name != "nt" or not chroot.is_absolute() or chroot == PurePosixPath("/"):
            parser.error("--linux-chroot requires Windows and an absolute WSL path other than /")
    if args.wsl_distro and not args.linux_chroot:
        parser.error("--wsl-distro requires --linux-chroot")

    DIST.mkdir(exist_ok=True)
    lock = DIST / ".release-lock"
    try:
        lock.mkdir()
    except FileExistsError:
        parser.error(f"A release build is already locked at {lock}")
    try:
        with tempfile.TemporaryDirectory(prefix=".release-", dir=DIST) as directory:
            stage = Path(directory)
            if sys.platform.startswith("linux"):
                sdl_prefix = prepare_linux_sdl()
                os.environ["PKG_CONFIG_PATH"] = str(sdl_prefix / "lib/pkgconfig") + (
                    os.pathsep + os.environ["PKG_CONFIG_PATH"] if os.environ.get("PKG_CONFIG_PATH") else ""
                )
            build_native()
            if os.name == "nt":
                packages = [package_windows(args.version, stage)]
                if args.linux_chroot:
                    runtime = stage / "linux-runtime"
                    build_linux_wsl(args.linux_chroot, args.wsl_distro, runtime)
                    packages.append(package_linux(args.version, stage, runtime, runtime / "libSDL3.so.0"))
            else:
                packages = [package_linux(
                    args.version, stage, ROOT / "build/linux-x86", sdl_prefix / "lib/libSDL3.so.0",
                )]
            checksums = []
            for archive in packages:
                checksum = archive.with_name(archive.name + ".sha256")
                digest = hashlib.sha256(archive.read_bytes()).hexdigest()
                checksum.write_bytes(f"{digest}  {archive.name}\n".encode("ascii"))
                checksums.append(checksum)
            publish(args.version, packages + checksums)
            for archive in packages:
                print(DIST / archive.name)
    finally:
        lock.rmdir()


if __name__ == "__main__":
    main()
