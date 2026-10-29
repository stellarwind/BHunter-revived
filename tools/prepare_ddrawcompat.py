"""Stage the pinned official DDrawCompat runtime for the game and release ZIP."""
import hashlib
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.7.2"
SHA256 = "a932a4dcb2100552b6918fd4a3cc7de138bb97448bbca2a7051954993a397b39"
URL = f"https://github.com/narzoul/DDrawCompat/releases/download/v{VERSION}/DDrawCompat-v{VERSION}.zip"


def prepare():
    archive = ROOT / f"build/dependencies/DDrawCompat-v{VERSION}.zip"
    archive.parent.mkdir(parents=True, exist_ok=True)
    if not archive.exists():
        request = urllib.request.Request(URL, headers={"User-Agent": "BHunter-build"})
        with urllib.request.urlopen(request, timeout=60) as response:
            data = response.read()
        if hashlib.sha256(data).hexdigest() != SHA256:
            raise RuntimeError("DDrawCompat download checksum mismatch")
        archive.write_bytes(data)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SHA256:
        raise RuntimeError("DDrawCompat archive checksum mismatch")
    destination = ROOT / "build/game"
    destination.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(archive) as bundle:
        dll = next(n for n in bundle.namelist() if Path(n).name.lower() == "ddraw.dll")
        (destination / "ddraw.dll").write_bytes(bundle.read(dll))
    (destination / "DDrawCompat-LICENSE.txt").write_bytes((ROOT / "LICENSES/DDrawCompat.txt").read_bytes())
    print(f"Staged DDrawCompat {VERSION}")
    return destination


if __name__ == "__main__":
    prepare()
