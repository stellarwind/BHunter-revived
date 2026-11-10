"""Pack level.lev, optional level.pth, and custom assets into a BHunter level."""

import argparse
import shutil
import struct
from pathlib import Path
from tempfile import NamedTemporaryFile


def pack_level(source, output):
    if output.suffix != ".bhlevel":
        raise ValueError("The output filename must end with .bhlevel")
    if not (source / "level.lev").is_file():
        raise FileNotFoundError(source / "level.lev")

    files = [source / "level.lev"]
    if (source / "level.pth").is_file():
        files.append(source / "level.pth")
    for folder in ("bsp", "textures", "sound"):
        files.extend(path for path in (source / folder).rglob("*") if path.is_file())
    files.sort(key=lambda path: path.relative_to(source).as_posix().lower())

    offset = 4 + 256 * len(files)
    directory = []
    names = set()
    for path in files:
        name = "\\".join(path.relative_to(source).parts).lower().encode("ascii")
        if len(name) >= 248 or name in names:
            raise ValueError(f"Invalid or duplicate archive entry: {path}")
        names.add(name)
        end = offset + path.stat().st_size
        if end > 0x7FFFFFFF:
            raise ValueError("Level packages must be smaller than 2 GiB")
        directory.append(struct.pack("<248sII", name, offset, end))
        offset = end

    output.parent.mkdir(parents=True, exist_ok=True)
    archive = NamedTemporaryFile(dir=output.parent, delete=False)
    temporary = Path(archive.name)
    try:
        with archive:
            archive.write(struct.pack("<I", len(directory)))
            archive.writelines(directory)
            for path in files:
                with path.open("rb") as asset:
                    shutil.copyfileobj(asset, archive)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    pack_level(args.source, args.output)
