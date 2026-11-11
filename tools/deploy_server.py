#!/usr/bin/env python3
"""Install a BHunter Linux server release and restart its systemd service."""

import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import tempfile
import time
import urllib.request

ROOT = Path("/opt/bhunter")
RELEASES = ROOT / "releases"
CURRENT = ROOT / "current"
SERVICE = "bhunter.service"
API = "https://api.github.com/repos/stellarwind/BHunter-revived/releases"
ARCHIVE = re.compile(r"BHunter-\d+\.\d+\.\d+-linux-server-x86\.tar\.gz")
TAG = re.compile(r"v\d+\.\d+\.\d+(?:-rc\.\d+)?")


def download(url):
    request = urllib.request.Request(url, headers={"User-Agent": "BHunter-deploy"})
    return urllib.request.urlopen(request, timeout=30)


def release_assets(tag):
    endpoint = f"tags/{tag}" if tag else "latest"
    with download(f"{API}/{endpoint}") as response:
        release = json.load(response)
    tag = release["tag_name"]
    if not TAG.fullmatch(tag):
        raise ValueError(f"Unexpected release tag: {tag}")
    assets = {asset["name"]: asset["browser_download_url"] for asset in release["assets"]}
    matches = [name for name in assets if ARCHIVE.fullmatch(name)]
    if len(matches) != 1 or matches[0] + ".sha256" not in assets:
        raise ValueError(f"{tag} must contain one Linux server archive and its checksum")
    name = matches[0]
    with download(assets[name + ".sha256"]) as response:
        checksum = response.read().decode("ascii").split()
    if len(checksum) != 2 or checksum[1] != name or not re.fullmatch(r"[a-f0-9]{64}", checksum[0]):
        raise ValueError(f"Invalid checksum file for {name}")
    return tag, name, assets[name], checksum[0]


def install_release(tag, name, url, digest):
    destination = RELEASES / f"{tag}-{digest[:12]}"
    if destination.is_dir():
        return destination
    with tempfile.TemporaryDirectory(dir=RELEASES, prefix=".download-") as temporary:
        archive = Path(temporary) / name
        with download(url) as response, archive.open("wb") as stream:
            shutil.copyfileobj(response, stream)
        with archive.open("rb") as stream:
            if hashlib.file_digest(stream, "sha256").hexdigest() != digest:
                raise ValueError(f"Checksum mismatch for {name}")
        with tarfile.open(archive, "r:gz") as bundle:
            bundle.extractall(temporary, filter="data")
        package = Path(temporary) / name.removesuffix(".tar.gz")
        server = package / "BHunter-server"
        levels = package / "GAME_DATA"
        if not server.is_file() or not any(levels.glob("*.bhlevel")):
            raise ValueError(f"Missing server binary or levels in {name}")
        package.chmod(0o755)
        levels.chmod(0o755)
        server.chmod(0o755)
        package.replace(destination)
    return destination


def activate(path):
    with tempfile.TemporaryDirectory(dir=ROOT, prefix=".current-") as temporary:
        link = Path(temporary) / "current"
        link.symlink_to(path)
        link.replace(CURRENT)


def restart():
    subprocess.run(["systemctl", "restart", SERVICE], check=True)
    deadline = time.monotonic() + 12
    while time.monotonic() < deadline:
        pid = subprocess.check_output(
            ["systemctl", "show", SERVICE, "--property=MainPID", "--value"], text=True,
        ).strip()
        if pid.isdigit() and int(pid) > 1:
            sockets = subprocess.check_output(["ss", "-H", "-lunp"], text=True)
            if f"pid={pid}," in sockets:
                return
        time.sleep(.25)
    raise RuntimeError(f"{SERVICE} did not open a UDP socket")


def deploy(tag):
    if not CURRENT.is_symlink():
        raise ValueError(f"Initialize {CURRENT} with the currently installed release first")
    RELEASES.mkdir(exist_ok=True)
    tag, name, url, digest = release_assets(tag)
    destination = install_release(tag, name, url, digest)
    previous = CURRENT.resolve(strict=True)
    if previous == destination:
        print(f"{tag} is already installed")
        return
    activate(destination)
    try:
        restart()
    except (OSError, subprocess.SubprocessError, RuntimeError) as error:
        activate(previous)
        try:
            restart()
        except (OSError, subprocess.SubprocessError, RuntimeError) as rollback:
            raise RuntimeError(f"{tag} failed: {error}. Rollback restart also failed: {rollback}") from rollback
        raise RuntimeError(f"{tag} failed: {error}. Restored {previous.name}") from error
    print(f"Deployed {tag} and restarted {SERVICE}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", nargs="?", help="Release tag. Defaults to GitHub's latest stable release.")
    args = parser.parse_args()
    if args.tag and not TAG.fullmatch(args.tag):
        parser.error("Expected a release tag such as v0.5.3 or v0.5.3-rc.1")
    if os.geteuid() != 0:
        parser.error("Run with sudo")
    try:
        with open("/run/lock/bhunter-deploy.lock", "w") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            deploy(args.tag)
    except (OSError, KeyError, ValueError, tarfile.TarError, RuntimeError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
