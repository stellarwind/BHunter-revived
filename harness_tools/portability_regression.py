"""Build/run an isolated full-game portability regression arena.

Only generated application copies are instrumented. No desktop control,
screenshots, retail asset edits, or production source changes are used.
"""

import argparse
import hashlib
import json
import math
import os
import shutil
import struct
import subprocess
import sys
import wave
from pathlib import Path

from portal_box_assets import write_box

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "harness_tools/output/portability-regression"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, **kwargs):
    result = subprocess.run(
        [str(item) for item in command],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=False,
        **kwargs,
    )
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result


def archive_file(archive, wanted):
    with archive.open("rb") as stream:
        size = archive.stat().st_size
        count = struct.unpack("<I", stream.read(4))[0]
        if count > (size - 4) // 256:
            raise ValueError("Invalid archive directory")
        for _ in range(count):
            entry = stream.read(256)
            name = entry[:248].split(b"\0", 1)[0].decode("ascii")
            if name.replace("\\", "/").lower() != wanted.lower():
                continue
            start, end = struct.unpack_from("<II", entry, 248)
            if not 4 + count * 256 <= start <= end <= size:
                raise ValueError("Invalid archive entry")
            stream.seek(start)
            return stream.read(end - start)
    raise FileNotFoundError(wanted)


def tone(path, seconds, frequency, stereo=False):
    rate = 22050
    samples = bytearray()
    for index in range(round(seconds * rate)):
        t = index / rate
        envelope = min(1.0, t / 0.015, (seconds - t) / 0.015)
        sample = round(3500 * envelope * math.sin(2 * math.pi * frequency * t))
        samples.extend(struct.pack("<h", sample))
        if stereo:
            samples.extend(struct.pack("<h", sample))
    with wave.open(str(path), "wb") as output:
        output.setparams((2 if stereo else 1, 2, rate, 0, "NONE", "not compressed"))
        output.writeframes(samples)


def room_texture(path, color):
    header = bytearray(128)
    header[:4] = bytes((10, 5, 1, 8))
    struct.pack_into("<4H", header, 4, 0, 0, 63, 63)
    header[65] = 1
    struct.pack_into("<HH", header, 66, 64, 1)
    pixels = bytes(
        3 if x in (0, 31, 32, 63) or y in (0, 31, 32, 63) else 1 + (x // 8 + y // 8) % 2
        for y in range(64)
        for x in range(64)
    )
    palette = bytearray(768)
    palette[3:6] = bytes(color)
    palette[6:9] = bytes(component // 2 for component in color)
    palette[9:12] = bytes((230, 230, 230))
    path.write_bytes(header + pixels + b"\x0c" + palette)


def assets(runtime):
    bsp = runtime / "bsp"
    bsp.mkdir(exist_ok=True)
    aperture = (-12, 12, -18, 18)
    rooms = [
        ("reg_start.bsp", (-60, -30, -100), (60, 30, 100), {(2, 1): aperture}),
        (
            "reg_audio.bsp",
            (-60, -30, 100),
            (60, 30, 200),
            {(2, 0): aperture, (2, 1): aperture},
        ),
        ("reg_end.bsp", (-60, -30, 200), (60, 30, 300), {(2, 0): aperture}),
    ]
    blocks = archive_file(runtime / "GAME_DATA/Bhunter.bin", "bsp/blocks.txt")
    names = []
    for line in blocks.decode("ascii").splitlines():
        if line.lower() == "end":
            break
        if line and not line.startswith(";"):
            names.append(line)
    (runtime / "textures").mkdir(exist_ok=True)
    colors = ((80, 160, 240), (80, 220, 140), (240, 170, 70))
    for index, (name, low, high, doors) in enumerate(rooms):
        path = bsp / name
        write_box(path, low, high, doors)
        texture_name = f"reg{index}.pcx"
        data = bytearray(path.read_bytes())
        data[13:36] = texture_name.encode().ljust(23, b"\0")
        path.write_bytes(data)
        room_texture(runtime / "textures" / texture_name, colors[index])
        path.with_suffix(".txt").write_text(
            "grid 1\ncollision on\nrender on\n", encoding="ascii"
        )
    (bsp / "blocks.txt").write_text(
        "\n".join(names + [room[0] for room in rooms] + ["end"]) + "\n",
        encoding="ascii",
    )
    (runtime / "test.lev").write_bytes(
        struct.pack("<i", len(rooms))
        + b"".join(
            name.encode().ljust(16, b"\0") + struct.pack("<4i", 0, 0, 0, 0)
            for name, *_ in rooms
        )
    )
    # A valid mismatching count requests reconstruction rather than retail cache reuse.
    (runtime / "test.pth").write_bytes(struct.pack("<I", 0xFFFFFFFF))
    tone(runtime / "reg-tone.wav", 0.4, 440)
    tone(runtime / "reg-loop.wav", 0.25, 220)
    # A short file makes actual completion/loop notifications observable.
    (runtime / "MUSIC_DATA").mkdir(exist_ok=True)
    tone(runtime / "MUSIC_DATA/track02.wav", 3, 330, stereo=True)
    manifest = {
        "rooms": [
            {"name": name, "minimum": low, "maximum": high}
            for name, low, high, _ in rooms
        ],
        "retail_assets_modified": False,
        "loose_overrides": ["bsp/blocks.txt", "test.lev", "test.pth"],
        "music": "Synthetic 3-second stereo 330 Hz tone, isolated runtime only",
        "input_boundary": "Targeted window messages to this process, no global input",
        "limits": [
            "Input route must be adapted when native message handling is replaced",
            "Focus/minimize, physical devices, sound quality and appearance require manual checks",
            "No multiplayer peer, physical CD, or force-feedback hardware certification",
        ],
    }
    (runtime / "arena.json").write_text(json.dumps(manifest, indent=2) + "\n")


def replace_once(source, before, after):
    if source.count(before) != 1:
        raise RuntimeError(f"Instrumentation anchor changed: {before[:90]}")
    return source.replace(before, after)


def build():
    runtime = OUT / "runtime"
    run(
        [
            sys.executable,
            ROOT / "tools/build_game.py",
            "--output-dir",
            runtime,
            "--debugging",
            "--symbols",
            "--incremental",
        ]
    )
    cache = (
        ROOT
        / "build-cache/gcc"
        / hashlib.sha256(str(runtime).encode()).hexdigest()[:12]
    )
    record = json.loads((cache / "build.json").read_text())
    if not record["passed"]:
        raise RuntimeError("Production build did not pass")
    assets(runtime)
    app = (ROOT / "src/bh_game_app.c").read_text()
    include = (
        '#include "'
        + (ROOT / "harness_tools/portability_scenario.inc").as_posix()
        + '"\n'
    )
    app = replace_once(
        app,
        "static int manual_loop(BHGameApp *a)",
        include + "static int manual_loop(BHGameApp *a)",
    )
    app = replace_once(
        app,
        "    while (!a->quit && !a->error)",
        "    regression_begin(a);\n    while (!a->quit && !a->error)",
    )
    app = replace_once(
        app,
        "if (!bh_sdl_active())",
        'if (!regression_automated && !getenv("BH_REGRESSION_STATIC_CHECK") && !bh_sdl_active())',
    )
    app = replace_once(
        app,
        "            a->flyby.input_profile = a->profile;",
        "            regression_before_tick(a);\n            a->flyby.input_profile = a->profile;",
    )
    app = replace_once(
        app,
        "            a->frames.produce(a->frames.produce_user);",
        "            a->frames.produce(a->frames.produce_user);\n            regression_after_tick(a);",
    )
    app = replace_once(
        app,
        "            if (!render_ok)",
        "            regression_after_render(a);\n            if (!render_ok)",
    )
    app = replace_once(
        app,
        "    result = a->error;",
        "    regression_finish(a);\n    result = a->error;",
    )
    app = replace_once(
        app,
        "bh_sdl_open(0, sdl_input, sdl_music, a)",
        'bh_sdl_open(getenv("BH_REGRESSION_VISIBLE") ? 0 : 1, sdl_input, sdl_music, a)',
    )
    generated = OUT / "regression_game_app.c"
    generated.write_text(app)
    run(
        [
            "clang-format",
            "--style=file:" + str(ROOT / "src/.clang-format"),
            "-i",
            generated,
        ]
    )
    obj = OUT / "regression_game_app.o"
    # Incremental build records contain only commands executed this time.
    # Compile the generated owner with the production application's GCC flags.
    command = [
        record["compiler"]["cc"],
        "-std=gnu99",
        "-O2",
        "-g",
        "-fno-strict-aliasing",
        "-fwrapv",
        "-ffunction-sections",
        "-fdata-sections",
        "-DBH_SDL_BACKEND",
        "-DDEBUGGING",
        "-DDIRECT3D_VERSION=0x0500",
        "-DDIRECTDRAW_VERSION=0x0500",
        "-DDIRECTINPUT_VERSION=0x0500",
        "-include",
        ROOT / "src/bh_sdl_adapter.h",
        "-I" + str(ROOT / "src"),
        "-I" + str(ROOT / "toolchain/SDL3/SDL3-3.4.18/include"),
        "-c",
        generated,
        "-o",
        obj,
    ]
    run(command)
    main_source = (ROOT / "src/bh_game_main.c").read_text()
    main_source = replace_once(
        main_source,
        "        bh_sdl_error_dialog();",
        '        fprintf(stderr, "Regression game exited with error %d\\n", result);',
    )
    generated_main = OUT / "regression_game_main.c"
    generated_main.write_text("#include <stdio.h>\n" + main_source)
    run(
        [
            "clang-format",
            "--style=file:" + str(ROOT / "src/.clang-format"),
            "-i",
            generated_main,
        ]
    )
    main_obj = OUT / "regression_game_main.o"
    main_command = command[:]
    main_command[main_command.index(generated)] = generated_main
    main_command[main_command.index(obj)] = main_obj
    run(main_command)
    objects = [
        obj
        if path.stem == "bh_game_app"
        else main_obj
        if path.stem == "bh_game_main"
        else path
        for path in (
            Path(line.strip('"'))
            for line in (cache / "objects.rsp").read_text().splitlines()
        )
    ]
    response = OUT / "regression_objects.rsp"
    response.write_text(
        "\n".join('"' + path.as_posix() + '"' for path in objects) + "\n"
    )
    link = record["commands"][-1]["argv"][:]
    link = ["@" + str(response) if arg.startswith("@") else arg for arg in link]
    target = runtime / "BHunter-regression.exe"
    link[link.index("-o") + 1] = str(target)
    link.insert(
        link.index("-o"), str(ROOT / "toolchain/SDL3/SDL3-3.4.18/lib/x86/SDL3.lib")
    )
    run(link)
    provenance = {
        "production_sources": record["sources"],
        "harness_sha256": {
            name: digest(ROOT / "harness_tools" / name)
            for name in (
                "portability_regression.py",
                "portability_scenario.inc",
                "portal_box_assets.py",
                "comic_font_comparison.py",
                "comic_text_probe.c",
                "check_static_vehicles.py",
                "portability_contracts.py",
            )
        },
        "executable_sha256": digest(target),
        "diagnostic_code_in_production_source": False,
    }
    (OUT / "build.json").write_text(json.dumps(provenance, indent=2) + "\n")
    print(f"Built {target}", flush=True)
    return runtime, target


def execute(runtime, target, manual, visible, negative):
    env = dict(os.environ)
    for key in (
        "BH_REGRESSION_MANUAL",
        "BH_REGRESSION_VISIBLE",
        "BH_REGRESSION_DROP_KEYUP",
        "BH_REGRESSION_STATIC_CHECK",
    ):
        env.pop(key, None)
    if manual:
        env["BH_REGRESSION_MANUAL"] = "1"
    if manual or visible:
        env["BH_REGRESSION_VISIBLE"] = "1"
    if negative:
        env["BH_REGRESSION_DROP_KEYUP"] = "1"
    events = runtime / "regression-events.jsonl"
    summary = runtime / "regression-summary.json"
    events.unlink(missing_ok=True)
    summary.unlink(missing_ok=True)
    if not manual:
        # This dedicated runtime is test-owned. Keep each replay independent of
        # previous manual edits without touching the user's normal game saves.
        (runtime / "source-preview.cfg").unlink(missing_ok=True)
        (runtime / "video-sdl3.ini").unlink(missing_ok=True)
    process = subprocess.Popen([str(target), "--profile"], cwd=OUT, env=env)
    (OUT / "session.json").write_text(
        json.dumps({"pid": process.pid, "manual": manual}, indent=2) + "\n"
    )
    if manual:
        print(f"Manual arena running, PID {process.pid}. Close the game when finished.")
        return 0
    try:
        code = process.wait(timeout=100)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
        raise RuntimeError("Regression timed out; no pass recorded")
    if not summary.exists():
        raise RuntimeError(
            f"Game exited {code} without a regression summary; inspect startup/debugger logs"
        )
    result = json.loads(summary.read_text())
    result.update(process_exit_code=code, negative_control=negative)
    passed = code == 0 and result["failures"] == 0 and result["completed"]
    if negative:
        rows = [json.loads(line) for line in events.read_text().splitlines()]
        failed_checks = [row["check"] for row in rows if row.get("passed") is False]
        detected = failed_checks == ["keyboard-release"]
        result["negative_control_detected"] = detected
        passed = code == 90 and detected and result["completed"]
    result["expectation_passed"] = passed
    destination = OUT / ("negative-results.json" if negative else "results.json")
    destination.write_text(json.dumps(result, indent=2) + "\n")
    shutil.copy2(
        events, OUT / ("negative-events.jsonl" if negative else "events.jsonl")
    )
    print(json.dumps(result, indent=2), flush=True)
    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument(
        "--suite",
        action="store_true",
        help="Run replay, negative control, and subsystem contracts",
    )
    parser.add_argument(
        "--manual",
        action="store_true",
        help="Visible arena with live controls and recurring sound cues",
    )
    parser.add_argument("--visible", action="store_true", help="Show the scripted run")
    parser.add_argument(
        "--negative-control",
        action="store_true",
        help="Drop a key release and require detection",
    )
    args = parser.parse_args()
    if args.manual and args.negative_control:
        parser.error("Negative controls require the scripted mode")
    if args.suite and (args.manual or args.build_only or args.negative_control):
        parser.error("--suite runs both scripted modes and contracts")
    OUT.mkdir(parents=True, exist_ok=True)
    session = OUT / "session.json"
    if session.exists() and os.name == "nt":
        pid = json.loads(session.read_text())["pid"]
        # Read-only process inventory; never terminate a prior manual session.
        inventory = run(
            ["tasklist", "/FI", f"PID eq {pid}", "/FO", "CSV", "/NH"]
        ).stdout
        if "BHunter-regress" in inventory:
            parser.error("Close the existing regression game before rebuilding")
    before = {
        path: digest(path) for path in (ROOT / "src").rglob("*") if path.is_file()
    }
    runtime, target = build()
    changed = [str(path) for path, value in before.items() if digest(path) != value]
    if changed:
        raise RuntimeError(
            "Production source changed while building: " + ", ".join(changed)
        )
    if args.build_only:
        return 0
    if args.suite:
        outcomes = {
            "replay_exit": execute(runtime, target, False, args.visible, False),
            "negative_control_exit": execute(
                runtime, target, False, args.visible, True
            ),
        }
        contracts = subprocess.run(
            [
                sys.executable,
                str(ROOT / "harness_tools/portability_contracts.py"),
                "--no-build",
            ],
            cwd=ROOT,
            check=False,
        )
        outcomes["contracts_exit"] = contracts.returncode
        comic_text = subprocess.run(
            [
                sys.executable,
                str(ROOT / "harness_tools/comic_font_comparison.py"),
                "--runtime",
                str(runtime),
            ],
            cwd=ROOT,
            check=False,
        )
        outcomes["comic_text_exit"] = comic_text.returncode
        static_vehicles = subprocess.run(
            [sys.executable, str(ROOT / "harness_tools/check_static_vehicles.py")],
            cwd=ROOT,
            check=False,
        )
        outcomes["static_vehicles_exit"] = static_vehicles.returncode
        outcomes["production_source_unchanged"] = all(
            digest(path) == value for path, value in before.items()
        )
        outcomes["passed"] = (
            all(
                outcomes[key] == 0
                for key in (
                    "replay_exit",
                    "negative_control_exit",
                    "contracts_exit",
                    "comic_text_exit",
                    "static_vehicles_exit",
                )
            )
            and outcomes["production_source_unchanged"]
        )
        (OUT / "suite-results.json").write_text(json.dumps(outcomes, indent=2) + "\n")
        return 0 if outcomes["passed"] else 1
    return execute(runtime, target, args.manual, args.visible, args.negative_control)


if __name__ == "__main__":
    raise SystemExit(main())
