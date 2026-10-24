"""Inspect original mission-event producer bytes and archive scripts; no execution."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--write-report', action='store_true')
    args = ap.parse_args()
    manifest = json.loads((ROOT / 'META/reference-manifest.json').read_text())
    pinned = {x['path']: x['sha256'] for x in manifest['files'] if x['root'] == 'repo'}
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    archive = (ROOT / 'GAME_DATA/Bhunter.bin').read_bytes()
    for p, b in [('GAME_DATA/BHUNTER.EXE', exe), ('GAME_DATA/Bhunter.bin', archive)]:
        assert hashlib.sha256(b).hexdigest() == pinned[p]
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    count = struct.unpack_from('<H', exe, pe + 6)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    base = struct.unpack_from('<I', exe, pe + 52)[0]
    def raw(va, size):
        rva = va - base
        for i in range(count):
            _, start, length, offset = struct.unpack_from('<IIII', exe, table + i * 40 + 8)
            if start <= rva and rva + size <= start + length:
                return exe[offset + rva - start:offset + rva - start + size]
        raise ValueError(hex(va))
    targets = list(struct.unpack('<25I', raw(0x4456e4, 100)))
    assert targets[0x12] == 0x445488
    assert raw(0x4454aa, 11).hex() == 'c78424900000000e000000'
    assert raw(0x4454c4, 5).hex() == 'e8271c0300'  # real bh_entities_query owner
    assert raw(0x4456e2, 1) == b'\xc3'
    assert raw(0x443ff0, 6).hex() == '81eca0000000'
    scripts = []
    for item in json.loads((ROOT / 'analysis/assets-index.json').read_text()):
        if not item['name'].upper().endswith('.TXT'):
            continue
        b = archive[item['offset']:item['end']]
        lines = b.decode('latin1').splitlines()
        broadcasts = [dict(line=i + 1, text=line.strip()) for i, line in enumerate(lines) if re.match(r'^\s*broadcast\b', line, re.I)]
        if broadcasts:
            scripts.append(dict(name=item['name'], sha256=hashlib.sha256(b).hexdigest(), broadcasts=broadcasts))
    index = json.loads((ROOT / 'analysis/assets-index.json').read_text())
    level = next(x for x in index if x['name'].upper() == 'TEST.LEV')
    level_bytes = archive[level['offset']:level['end']]
    placements_count = struct.unpack_from('<I', level_bytes)[0]
    placements = list(struct.iter_unpack('<16siiiI', level_bytes[4:4 + placements_count * 32]))
    candidates = []
    for link in struct.iter_unpack('<I8h', level_bytes[4 + placements_count * 32:]):
        if not set(link[1:]).intersection({2010, 2390, 2400}):
            continue
        name, x, y, z, flags = placements[link[0]]
        name = name.rstrip(b'\0').decode('ascii')
        script_name = 'BSP\\' + name[:-4] + '.TXT'
        script = next(v for v in index if v['name'].upper() == script_name.upper())
        data = archive[script['offset']:script['end']]
        candidates.append(dict(placement=link[0], name=name, position=[x, y, z], flags=flags, raw_signed_slots=list(link[1:]), script=script_name, script_sha256=hashlib.sha256(data).hexdigest(), script_text=data.decode('latin1')))
    review = ROOT / 'META/m3/mission-review'
    evidence = {str(p.relative_to(ROOT)).replace('\\', '/'): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(review.iterdir()) if p.is_file()}
    report = dict(scope='Static event producer and raw script inventory only', original_hashes={p: pinned[p] for p in ['GAME_DATA/BHUNTER.EXE', 'GAME_DATA/Bhunter.bin']}, script_opcode_table_address='004456e4', script_opcode_targets=[f'{v:08x}' for v in targets], broadcast_opcode=0x12, broadcast_branch='00445488', event_type=0x0e, query_call='004454c4', mission_consumer='004225f0', event_id_source='script entity +0x100, including placement slot substitution', broadcast_scripts=scripts, placement_candidates=candidates, placement_interpretation_limit='Raw slot matches identify capture candidates; complete define/slot substitution and activation chains still require review', evidence_sha256=evidence, runtime_objectives_validated=False, game_source_admission=False, m3_complete=False)
    path = ROOT / 'META/m3/mission-inventory.json'
    if args.write_report:
        path.write_text(json.dumps(report, indent=2) + '\n')
    else:
        assert json.loads(path.read_text()) == report
    print(json.dumps(dict(passed=True, opcode_table_entries=len(targets), broadcast_opcode=18, script_files_with_broadcast=len(scripts), game_launches=0, runtime_objectives_validated=False)))

if __name__ == '__main__':
    main()
