"""Build the release SDL with optional PipeWire, PulseAudio and ALSA backends."""

import hashlib
import os
from pathlib import Path
import re
import subprocess
import tarfile
import urllib.request

from prepare_sdl3 import VERSION

ROOT = Path(__file__).resolve().parents[1]
SOURCE_SHA256 = "9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3"


def validate_runtime(path, *, sdl=False):
    """Reject accidental linkage to build-machine audio/window-system libraries."""
    path = Path(path)
    header = path.read_bytes()[:20]
    if header[:6] != b"\x7fELF\x01\x01" or header[18:20] != b"\x03\x00":
        raise ValueError(f"Expected a 32-bit x86 Linux library/executable: {path}")
    imports = subprocess.check_output(["objdump", "-p", str(path)], text=True)
    needed = set(re.findall(r"NEEDED\s+(\S+)", imports))
    allowed = {"libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "librt.so.1"}
    if not sdl:
        allowed |= {"libSDL3.so.0", "libstdc++.so.6", "libgcc_s.so.1"}
    unexpected = needed - allowed
    if unexpected:
        raise ValueError(
            f"{path} requires unbundled libraries: {', '.join(sorted(unexpected))}. "
            "Build release SDL with dynamically loaded system backends."
        )


def prepare():
    cache = ROOT / "build-cache/sdl3/linux"
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / f"SDL3-{VERSION}.tar.gz"
    if not archive.exists():
        urllib.request.urlretrieve(
            f"https://github.com/libsdl-org/SDL/releases/download/release-{VERSION}/{archive.name}",
            archive,
        )
    if hashlib.sha256(archive.read_bytes()).hexdigest() != SOURCE_SHA256:
        raise RuntimeError("SDL3 source archive SHA256 mismatch")
    source = cache / f"SDL3-{VERSION}"
    if not source.exists():
        with tarfile.open(archive) as bundle:
            bundle.extractall(cache, filter="data")
    build = cache / "build-ninja"
    prefix = cache / "install"
    subprocess.run([
        "cmake", "-G", "Ninja", "-S", str(source), "-B", str(build),
        "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_INSTALL_PREFIX={prefix}",
        "-DCMAKE_INSTALL_LIBDIR=lib", "-DCMAKE_C_FLAGS=-m32", "-DCMAKE_CXX_FLAGS=-m32",
        "-DSDL_SHARED=ON", "-DSDL_STATIC=OFF", "-DSDL_TEST_LIBRARY=OFF",
        "-DSDL_TESTS=OFF", "-DSDL_EXAMPLES=OFF", "-DSDL_DEPS_SHARED=ON",
        "-DSDL_PIPEWIRE=ON", "-DSDL_PIPEWIRE_SHARED=ON",
        "-DSDL_PULSEAUDIO=ON", "-DSDL_PULSEAUDIO_SHARED=ON",
        "-DSDL_ALSA=ON", "-DSDL_ALSA_SHARED=ON",
    ], check=True)
    config = (build / "include-config-release/build_config/SDL_build_config.h").read_text()
    for backend in ("PIPEWIRE", "PULSEAUDIO", "ALSA"):
        if not re.search(rf'^#define SDL_AUDIO_DRIVER_{backend}_DYNAMIC "', config, re.MULTILINE):
            raise RuntimeError(
                f"SDL is missing optional {backend} support. Install its 32-bit development "
                "package and reconfigure; do not ship a build with this backend disabled."
            )
    subprocess.run(["cmake", "--build", str(build), "--parallel", str(min(8, os.cpu_count() or 1))], check=True)
    subprocess.run(["cmake", "--install", str(build)], check=True)
    validate_runtime(prefix / "lib/libSDL3.so.0", sdl=True)
    return prefix


if __name__ == "__main__":
    print(prepare())
