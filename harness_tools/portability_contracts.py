"""Run focused existing subsystem tests against the GCC portability build.

These complement the playable arena; they do not certify OS event delivery,
physical devices, audible output, or multiplayer interoperability.
"""

import argparse
import hashlib
import json
import shutil
import subprocess

from portability_regression import OUT, ROOT, build

CASES = (
    "input_evaluate",
    "input_messages",
    "input_collection",
    "player_input",
    "player_input_events",
    "network_codec",
    "config",
    "wave",
    "vfile",
    "sound_queue",
    "sound_listener",
    "sound_tick",
    "sdl_timing",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--no-build", action="store_true", help="Use the existing isolated build"
    )
    args = parser.parse_args()
    runtime = OUT / "runtime"
    if not args.no_build:
        build()
    cache = (
        ROOT
        / "build-cache/gcc"
        / hashlib.sha256(str(runtime).encode()).hexdigest()[:12]
    )
    record = json.loads((cache / "build.json").read_text())
    for name, expected in record["sources"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
            raise RuntimeError(f"Stale build: {name}; rerun without --no-build")
    directory = OUT / "contracts"
    directory.mkdir(exist_ok=True)
    for name in ("BHunterGPU.dll", "SDL3.dll"):
        shutil.copy2(runtime / name, directory / name)
    response = directory / "objects.rsp"
    # Read the exact linked set, not arbitrary stale objects in the cache.
    objects = (cache / "objects.rsp").read_text().splitlines()
    response.write_text(
        "\n".join(line for line in objects if not line.endswith('bh_game_main.o"'))
        + "\n"
    )
    results = []
    for name in CASES:
        case_dir = directory / name
        case_dir.mkdir(exist_ok=True)
        source = ROOT / "harness_tools/fixtures" / (name + ".c")
        entry = directory / (name + "_entry.c")
        adaptations = []
        sources = [source]
        if name == "wave":
            sources.append(ROOT / "harness_tools/fixtures/riff.c")
        if name != "sdl_timing":
            if name == "wave":
                prototype = "const char *, unsigned long *"
                call = (
                    '"' + (runtime / "GAME_DATA/Bhunter.bin").as_posix() + '", &count'
                )
            elif name == "vfile":
                prototype = "const char *, unsigned long *, unsigned long *"
                call = (
                    '"'
                    + (runtime / "GAME_DATA/Bhunter.bin").as_posix()
                    + '", &count, &entries'
                )
            else:
                prototype, call = "unsigned long *", "&count"
            entry.write_text(
                '#include "bh_fp.h"\n'
                f"int bh_{name}_checkpoint({prototype});\n"
                "int main(void)\n{\n"
                "    unsigned long count = 0, entries = 0;\n"
                "    BHMathEnvironment math;\n"
                "    int result;\n"
                "    if (!bh_math_environment_enter(&math))\n        return 2;\n"
                f"    result = bh_{name}_checkpoint({call});\n"
                "    bh_math_environment_leave(&math);\n"
                "    return result;\n}\n"
            )
            subprocess.run(
                [
                    "clang-format",
                    "--style=file:" + str(ROOT / "src/.clang-format"),
                    "-i",
                    str(entry),
                ],
                check=True,
            )
            sources.append(entry)
        target = case_dir / (name + ".exe")
        command = [
            record["compiler"]["cc"],
            "-std=gnu99",
            "-O2",
            "-fno-strict-aliasing",
            "-fwrapv",
            "-ffunction-sections",
            "-fdata-sections",
            "-static",
            "-static-libgcc",
            "-DBH_SDL_BACKEND",
            "-DDIRECT3D_VERSION=0x0500",
            "-DDIRECTDRAW_VERSION=0x0500",
            "-DDIRECTINPUT_VERSION=0x0500",
            "-I" + str(ROOT / "src"),
            "-I" + str(ROOT / "harness_tools/fixtures"),
            "-include",
            str(ROOT / "src/bh_sdl_adapter.h"),
            *[str(path) for path in sources],
            "@" + str(response),
            str(ROOT / "build-cache/sdl3/gcc/libBHunterGPU.dll.a"),
            "-Wl,--gc-sections",
            "-ldinput",
            "-ldsound",
            "-lwinmm",
            "-lole32",
            "-ladvapi32",
            "-lcomctl32",
            "-lgdi32",
            "-luser32",
            "-o",
            str(target),
        ]
        compiled = subprocess.run(
            command, cwd=case_dir, text=True, capture_output=True, check=False
        )
        result = {
            "name": name,
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "fixture_adaptations": adaptations,
            "compile_exit": compiled.returncode,
            "compile_output": compiled.stdout + compiled.stderr,
        }
        if compiled.returncode == 0:
            # Load only this fixture's staged DLLs; never copy its generated
            # texture/config fixtures into the playable arena.
            for dll in ("BHunterGPU.dll", "SDL3.dll"):
                shutil.copy2(runtime / dll, case_dir / dll)
            try:
                executed = subprocess.run(
                    [str(target)],
                    cwd=case_dir,
                    text=True,
                    capture_output=True,
                    check=False,
                    timeout=40,
                )
                result.update(
                    exit_code=executed.returncode,
                    stdout=executed.stdout,
                    stderr=executed.stderr,
                )
            except subprocess.TimeoutExpired:
                result.update(exit_code=None, timed_out=True)
        result["passed"] = result.get("exit_code") == 0
        results.append(result)
        print(f"{name}: {'PASS' if result['passed'] else 'FAIL'}", flush=True)
        (directory / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    return int(any(not result["passed"] for result in results))


if __name__ == "__main__":
    raise SystemExit(main())
