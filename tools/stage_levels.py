"""Copy the shipped level packages into a build's GAME_DATA directory."""

import shutil
from pathlib import Path


def stage_levels(out):
    levels = Path(__file__).resolve().parents[1] / "levels"
    game_data = out / "GAME_DATA"
    game_data.mkdir(parents=True, exist_ok=True)
    staged = {}
    for source in sorted(levels.glob("*.bhlevel")):
        target = game_data / source.name
        if not target.is_file() or source.read_bytes() != target.read_bytes():
            shutil.copy2(source, target)
        staged["GAME_DATA/" + source.name] = str(source)
    return staged
