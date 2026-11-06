"""Build BHunter in an existing WSL chroot and deploy it to a paired Steam Deck."""

import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
BUILD = r"""
set -euo pipefail
root=$(realpath -e -- "$1")
test "$root" != /
test -x "$root/usr/bin/python3"
workspace="$root/workspace-deploy"
mkdir -p -- "$workspace"
test "$(realpath -- "$workspace")" = "$workspace"
# Lock this workspace through the build and local binary copy.
exec 9<"$workspace"
flock -x 9
for directory in src tools; do
    mkdir -p -- "$workspace/$directory"
    test "$(realpath -- "$workspace/$directory")" = "$workspace/$directory"
    rsync -rt --delete --exclude=__pycache__/ -- "$2/$directory/" "$workspace/$directory/"
done
install -D -- "$3" "$workspace/GAME_DATA/Bhunter.bin"
install -D -- "$4" "$workspace/assets/fonts/comic-sans-ms-24.bhf"
chroot "$root" /bin/sh -ec 'cd /workspace-deploy; python3 tools/build_gcc.py --incremental'
cp -- "$workspace/build/linux-x86/BHunter-source" "$5"
"""

INSTALL = r"""
set -eu
cd -- "$1"
stage=$2
expected=$3
previous=$4
trap 'rm -f -- "$stage" "$stage.previous"' EXIT
exec 9<.
flock -x 9
test "$(sha256sum -- "$stage" | cut -d' ' -f1)" = "$expected"
if test "$(sha256sum BHunter-source | cut -d' ' -f1)" != "$previous"; then
    echo "The installed build changed during upload. Retry the deploy." >&2
    exit 1
fi
chmod --reference=BHunter-source -- "$stage"
dependencies=$(LD_LIBRARY_PATH="$PWD/lib" ldd -r "$stage" 2>&1) || {
    printf '%s\n' "$dependencies" >&2
    exit 1
}
if printf '%s\n' "$dependencies" | grep -Eq 'not found|undefined symbol'; then
    printf '%s\n' "$dependencies" >&2
    exit 1
fi
cp -p -- BHunter-source "$stage.previous"
mv -fT -- "$stage.previous" BHunter-source.previous
mv -fT -- "$stage" BHunter-source
test "$(sha256sum BHunter-source | cut -d' ' -f1)" = "$expected"
"""


def build(chroot, distro):
    wsl = ["wsl.exe", *(["--distribution", distro] if distro else []), "--user", "root", "--exec"]

    def linux_path(path):
        return subprocess.check_output([*wsl, "wslpath", "-a", str(path)], text=True).strip()

    def asset(*candidates):
        for name in candidates:
            path = ROOT / name
            if path.is_file() and path.stat().st_size:
                return linux_path(path)
        raise FileNotFoundError(f"Missing build asset. Checked {', '.join(candidates)}")

    output = ROOT / "build/deck-check/BHunter-source.deploy"
    output.parent.mkdir(parents=True, exist_ok=True)
    archive = asset("GAME_DATA/Bhunter.bin", "build/GAME_DATA/Bhunter.bin")
    font = asset("assets/fonts/comic-sans-ms-24.bhf", "build/fonts/comic-sans-ms-24.bhf")
    subprocess.run(
        [*wsl, "bash", "-s", "--", chroot, linux_path(ROOT), archive, font, linux_path(output)],
        input=BUILD.encode(), check=True,
    )
    return output


def deploy(host, key, directory, binary, expected):
    options = [
        "-i", str(key), "-o", "BatchMode=yes", "-o", "IdentitiesOnly=yes",
        "-o", "StrictHostKeyChecking=yes", "-o", "ConnectTimeout=5",
    ]
    ssh = ["ssh", *options, host]
    installed = f"{directory}/BHunter-source"
    previous = subprocess.check_output([*ssh, shlex.join(["sha256sum", "--", installed])], text=True).split()[0]
    if previous == expected:
        print(f"{host}:{installed} already matches {expected}")
        return
    template = f"{directory}/.BHunter-source.deploy.XXXXXXXX"
    stage = subprocess.check_output([*ssh, shlex.join(["mktemp", "--", template])], text=True).strip()
    if not re.fullmatch(re.escape(template[:-8]) + r"[a-zA-Z0-9]{8}", stage):
        raise ValueError("Unexpected temporary path returned by the Deck")
    try:
        subprocess.run(
            ["scp", *options, "--", f"./{binary.name}", f"{host}:{stage}"],
            cwd=binary.parent, check=True,
        )
        command = shlex.join(["bash", "-s", "--", directory, stage, expected, previous])
        subprocess.run([*ssh, command], input=INSTALL.encode(), check=True)
    finally:
        subprocess.run([*ssh, shlex.join(["rm", "-f", "--", stage])], check=False)
    print(f"Installed {host}:{installed} ({expected})")
    print(f"Previous build saved as {installed}.previous. Relaunch BHunterNative to use the new build.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", nargs="?", help="Paired Deck IP or SSH host, optionally prefixed by deck@")
    parser.add_argument("--chroot", help="Absolute WSL path to the prepared Debian i386 chroot")
    parser.add_argument("--distro", help="WSL distribution (defaults to the system WSL default)")
    parser.add_argument("--key", type=Path, help="SSH private key (defaults to the Steam Devkit pairing key)")
    parser.add_argument("--install-dir", default="/home/deck/devkit-game/BHunterNative")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--build-only", action="store_true", help="Build locally without connecting to the Deck")
    mode.add_argument("--binary", type=Path, help="Deploy an existing 32-bit Linux build")
    args = parser.parse_args()
    if not args.build_only and not args.host:
        parser.error("Supply the paired Deck host, or use --build-only")
    if args.host and not re.fullmatch(r"(?:[\w.-]+@)?[a-zA-Z0-9][\w.-]*", args.host):
        parser.error("Invalid SSH host")
    directory = PurePosixPath(args.install_dir)
    if not directory.is_absolute() or directory == PurePosixPath("/") or ".." in directory.parts:
        parser.error("--install-dir must be an absolute game directory")
    try:
        if not args.binary and not args.chroot:
            configured = ROOT / "build/deck-check/linux-root.txt"
            if configured.is_file():
                args.chroot = configured.read_text().strip()
            if not args.chroot:
                parser.error("Supply --chroot with the path to your existing WSL build environment")
        if not args.binary and not PurePosixPath(args.chroot).is_absolute():
            parser.error("--chroot must be an absolute WSL path")
        key = args.key
        if not args.build_only:
            if key is None:
                local_app_data = os.environ.get("LOCALAPPDATA")
                if not local_app_data:
                    parser.error("Supply --key with the paired Deck SSH key")
                key = Path(local_app_data) / "steamos-devkit/steamos-devkit/devkit_rsa"
            if not key.is_file():
                raise FileNotFoundError(f"SSH key not found at {key}")
            key = key.resolve()
        binary = (args.binary or build(args.chroot, args.distro)).resolve()
        data = binary.read_bytes()
        if data[:6] != b"\x7fELF\x01\x01" or data[18:20] != b"\x03\x00":
            raise ValueError(f"Expected a native 32-bit x86 Linux executable at {binary}")
        expected = hashlib.sha256(data).hexdigest()
        if args.build_only:
            print(f"Built {binary} ({expected})")
        else:
            host = args.host if "@" in args.host else f"deck@{args.host}"
            deploy(host, key, str(directory), binary, expected)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Deck deployment failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
