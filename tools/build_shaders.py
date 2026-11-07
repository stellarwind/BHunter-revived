"""Regenerate the checked-in Vulkan shader bytecode from legacy.slang."""

import hashlib
import os
import shutil
import subprocess
import tempfile
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/shaders/legacy.slang"
HEADER = SOURCE.with_name("bh_gpu_shaders.h")
SLANG_VERSION = "2026.20"
SLANG_ARCHIVE = f"slang-{SLANG_VERSION}-windows-x86_64.zip"
SLANG_SHA256 = "e6dd32bbddf1720ebb4e679d55e9635ce792865a5bd84f3d2cd9af28647c0174"


def compile_with_pinned_slang():
    deps = ROOT / "toolchain/Slang"
    deps.mkdir(parents=True, exist_ok=True)
    archive = deps / SLANG_ARCHIVE
    with tempfile.TemporaryDirectory(dir=deps) as work:
        work = Path(work)
        download = archive
        if not archive.exists():
            download = work / SLANG_ARCHIVE
            urllib.request.urlretrieve(
                f"https://github.com/shader-slang/slang/releases/download/v{SLANG_VERSION}/{SLANG_ARCHIVE}",
                download,
            )
        if hashlib.sha256(download.read_bytes()).hexdigest() != SLANG_SHA256:
            raise RuntimeError(f"Slang archive SHA256 mismatch: {download}")
        if download != archive:
            download.replace(archive)
        with zipfile.ZipFile(archive) as z:
            for name in ("slangc.exe", "slang-compiler.dll", "slang-glslang.dll"):
                z.extract("bin/" + name, work)
        write_shader_header(work / "bin/slangc.exe")


def write_shader_header(slangc):
    lines = ["// Generated from legacy.slang by tools/build_shaders.py."]
    with tempfile.TemporaryDirectory(dir=HEADER.parent) as work:
        for stage, entry in (("vertex", "VSMain"), ("fragment", "PSMain")):
            output = Path(work) / f"{stage}.spv"
            # Slang's GLSL path supports SPIR-V 1.0, which SDL's Vulkan 1.0 device accepts.
            subprocess.run(
                [
                    slangc, SOURCE, "-entry", entry, "-stage", stage,
                    "-target", "spirv", "-profile", "spirv_1_0+GLSL_150", "-emit-spirv-via-glsl",
                    "-fvk-use-entrypoint-name", "-O3", "-warnings-as-errors", "all", "-o", output,
                ],
                check=True,
            )
            code = output.read_bytes()
            lines.append(f"alignas(4) static constexpr unsigned char spirv_{stage}[] = {{")
            for start in range(0, len(code), 12):
                row = ", ".join(f"0x{byte:02x}" for byte in code[start:start + 12])
                lines.append(f"    {row},")
            lines.append("};")
        result = Path(work) / HEADER.name
        result.write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")
        result.replace(HEADER)


def main():
    configured = os.environ.get("BH_SLANGC")
    if os.name == "nt" and configured is None:
        compile_with_pinned_slang()
    else:
        slangc = shutil.which(configured or "slangc")
        if not slangc:
            raise SystemExit("Install Slang (slangc with its glslang library) or set BH_SLANGC")
        write_shader_header(slangc)
    print(f"Generated {HEADER}")


if __name__ == "__main__":
    main()
