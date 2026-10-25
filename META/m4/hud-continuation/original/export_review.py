"""Read-only Ghidra export with independent immutable-PE byte verification.

This records original-code provenance only; source admission lives in the
shared function ledger and linked scenario evidence.
"""
from pathlib import Path
import hashlib
import json
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
EXE = ROOT / "GAME_DATA/BHUNTER.EXE"
data = EXE.read_bytes()
pe_sha = hashlib.sha256(data).hexdigest()
assert pe_sha == "c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1"
pe = struct.unpack_from("<I", data, 60)[0]
section_table = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
image_base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
sections = [struct.unpack_from("<IIII", data, section_table + i * 40 + 8)
            for i in range(struct.unpack_from("<H", data, pe + 6)[0])]


def original_bytes(va, count):
    rva = va - image_base
    for virtual_size, start, raw_size, file_offset in sections:
        if start <= rva and rva + count <= start + min(virtual_size, raw_size):
            at = file_offset + rva - start
            return data[at:at + count]
    raise AssertionError(f"body outside raw PE section: {va:08x}+{count}")


base = ["ghidra", "--project", "bhunter-frame", "--program", "BHUNTER.EXE"]
def query(args):
    # The persistent bridge can inherit redirected stdout when it auto-starts;
    # a temporary file lets the CLI process exit without pipe-EOF ambiguity.
    with tempfile.TemporaryFile(mode="w+t", encoding="utf-8") as output:
        subprocess.run(base + args + ["--json"], stdout=output,
                       stderr=subprocess.DEVNULL, check=True, timeout=90)
        output.seek(0)
        return json.load(output)
boundaries = [
    (0x00421BC0, 346, "player_reset", "gameplay", "src/bh_player_reset.c"),
    (0x00428F50, 1286, "flyby_scene", "gameplay", "src/bh_flyby_scene.c"),
    (0x0042A9B0, 34, "flyby_rearm", "gameplay", "src/bh_flyby_scene.c"),
    (0x0045CA10, 429, "frame_clear_depth", "renderer", "src/bh_frame_render.c"),
    (0x0045CBC0, 558, "frame_clear_color", "renderer", "src/bh_frame_render.c"),
]
limits = {
    "player_reset": "Source prevalidates player and ten live weapon identities; original assumes pointers. Sound-stop failure is reported only after all original mutations.",
    "flyby_scene": "Source rejects invalid waypoint indexing, released entities, malformed input and queue/resource failures; normal callback mutation and handoff are live.",
    "flyby_rearm": "Source callback rearm is guarded by explicit entity/generation binding; original uses a global mode counter.",
    "frame_clear_depth": "Source guards invalid viewport, cyclic texture list and out-of-bounds locked pixels; original trusts raw pointers and widths.",
    "frame_clear_color": "Source bounds RGB to 0..255 and requires valid surfaces; original arbitrary signed color overflow/native failure state is not promised.",
}
existing_decompile = {
    "00428f50": ROOT / "META/m4/scene-callers-review/original/00428f50.c.txt",
    "0042a9b0": ROOT / "META/m4/scene-callers-review/original/0042a9b0.c.txt",
}
report = []
for va, size, name, role, source in boundaries:
    stem = f"{va:08x}"
    try:
        meta = query(["function", "get", hex(va)])
    except subprocess.CalledProcessError:
        # This fresh autoanalysis omits two known game functions. Their size
        # and decompile were independently exported by the earlier full project;
        # the raw disassembly below is still generated and PE-checked here.
        assert stem in existing_decompile
        meta = [{"address": stem, "size": size, "name": "FUN_" + stem,
                 "provenance": "META/function-size-map.tsv; prior full-project export"}]
    assert meta[0]["size"] == size, (stem, meta[0].get("size"), size)
    (OUT / f"{stem}.function.json").write_text(json.dumps(meta, indent=2) + "\n")
    if stem in existing_decompile:
        decomp = [{"address": stem, "code": existing_decompile[stem].read_text(),
                   "provenance": str(existing_decompile[stem].relative_to(ROOT))}]
    else:
        decomp = query(["function", "decompile", hex(va)])
    (OUT / f"{stem}.decompile.json").write_text(json.dumps(decomp, indent=2) + "\n")
    rows = query(["disasm", hex(va), "-n", str(size)])
    at = va
    body = []
    calls = set()
    for row in rows:
        if at >= va + size:
            break
        raw = bytes.fromhex(row["bytes"])
        assert int(row["address"], 16) == at, (stem, row["address"], at)
        assert raw == original_bytes(at, len(raw)), (stem, row["address"], "PE mismatch")
        at += len(raw)
        body.append(row)
        if row["mnemonic"].startswith("CALL"):
            calls.update(row["operands"])
    assert at == va + size and body[-1]["mnemonic"] == "RET", (stem, at, size)
    (OUT / f"{stem}.disasm.json").write_text(json.dumps(body, indent=2) + "\n")
    report.append({
        "address": stem,
        "end_exclusive": f"{at:08x}",
        "bytes": size,
        "instructions": len(body),
        "raw_sha256": hashlib.sha256(original_bytes(va, size)).hexdigest(),
        "immutable_exe_sha256": pe_sha,
        "whole_raw_body_verified": True,
        "role": role,
        "boundary": "complete contiguous function; no shared tail, CRT helper or import",
        "source_owner": source,
        "call_operands": sorted(calls),
        "source_limit": limits[name],
        "original_code_executed": False,
        "source_admission_decided_here": False,
    })
tail_path = OUT / "00479090.ranges.json"
if tail_path.exists():
    tail = json.loads(tail_path.read_text())
    assert tail["original_exe_sha256"] == pe_sha and tail["function"] == "00479090"
    chunks = []
    for region in tail["ranges"]:
        start = int(region["start"], 16)
        raw = bytes.fromhex(region["hex"])
        assert len(raw) == region["bytes"] and raw == original_bytes(start, len(raw))
        assert hashlib.sha256(raw).hexdigest() == region["sha256"]
        assert int(region["end_exclusive"], 16) == start + len(raw)
        chunks.append(raw)
    assert len(chunks) == 2 and sum(map(len, chunks)) == 207
    assert sum(region["instructions"] for region in tail["ranges"]) == 64
    # 00479111 is the final five-byte near JMP of the entry range.
    displacement = struct.unpack_from("<i", chunks[0], len(chunks[0]) - 4)[0]
    assert chunks[0][-5] == 0xE9 and 0x00479116 + displacement == 0x00450930
    report.append({
        "address": "00479090", "ranges": tail["ranges"],
        "bytes": 207, "instructions": 64,
        "concatenated_ranges_sha256": hashlib.sha256(b"".join(chunks)).hexdigest(),
        "immutable_exe_sha256": pe_sha, "whole_raw_body_verified": True,
        "role": "renderer", "boundary": "one discontiguous owner; 00450930 tail is not a separate function",
        "source_owner": tail["source_owner"],
        "call_operands": tail["direct_calls"],
        "source_limit": "Typed semantic command selectors and explicit borrowed source contexts replace original function pointers; callback mutation remains live.",
        "original_code_executed": False,
        "source_admission_decided_here": False,
    })
(OUT / "raw-review.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps([{"address": row["address"], "bytes": row["bytes"],
                   "instructions": row["instructions"],
                   "sha256": row.get("raw_sha256", row.get("concatenated_ranges_sha256"))}
                  for row in report], indent=2))
