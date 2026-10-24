"""Verify the integrated 2026-10-24 source-restoration batch; never run original code."""
import csv
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/parallel-hour'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    for row in read(ROOT / 'META/reference-manifest.json')['files']:
        if row['root'] == 'repo':
            assert sha(ROOT / row['path']) == row['sha256'], row['path']
    validation = read(FOLDER / 'validation.json')
    owners = read(FOLDER / 'owners.json')
    ledger = read(ROOT / 'META/function-status.json')
    real = read(FOLDER / 'integration-real.json')
    clean_record = read(FOLDER / 'clean/clean-checkout.json')
    clean = clean_record['result']
    scene = read(FOLDER / 'scene-real.json')
    assert validation['passed'] and real['passed'] and clean['passed'] and scene['passed']
    assert not clean_record['original_assets_present'] and not clean_record['compiler_in_checkout']
    assert len(owners) == validation['new_complete_functions'] == 31
    assert sum(o['instructions'] for o in owners) == validation['original_instructions'] == 10007
    assert validation['real_total'] == real['test'] and validation['clean_total'] == clean['test']
    for build in (real, clean):
        assert not build['original_code_required'] and not build['test']['failures']
        assert sum(c['label'].startswith('compile-') for c in build['commands']) == 132
        controls = [v for k, v in build.items() if k.startswith('missing_')]
        assert len(controls) == 66 and all('LNK2001' in v for v in controls)
    for name, digest in validation['sources'].items():
        assert sha(ROOT / validation.get('historical_source_snapshots', {}).get(name, name)) == digest, name
        for build in (real, clean, scene):
            if name in build['sources']:
                assert build['sources'][name] == digest, name
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', exe, 60)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    sections = [struct.unpack_from('<IIII', exe, table + i * 40 + 8)
                for i in range(struct.unpack_from('<H', exe, pe + 6)[0])]
    maps = [(FOLDER / p).read_bytes() for p in ('link-map.txt', 'clean/link-map.txt')]
    for owner in owners:
        path = ROOT / owner['original']
        if path.suffix == '.tsv':
            with path.open() as stream:
                rows = list(csv.DictReader(stream, delimiter='\t'))
            assert len(rows) == owner['instructions']
            body = bytearray()
            at = int(owner['address'], 16)
            for row in rows:
                assert int(row['address'], 16) == at
                data = bytes.fromhex(row['bytes'])
                body.extend(data)
                at += len(data)
        else:
            records = read(path)
            records = records if isinstance(records, list) else ([records] if 'address' in records else list(records.values()))
            body = bytes.fromhex(next(r for r in records if r['address'] == owner['address'])['raw_hex'])
        assert len(body) == owner['bytes']
        assert hashlib.sha256(body).hexdigest() == owner['body_sha256']
        rva = int(owner['address'], 16) - 0x400000
        _, start, _, offset = next(s for s in sections if s[1] <= rva and rva + len(body) <= s[1] + s[2])
        assert exe[offset + rva - start:offset + rva - start + len(body)] == body
        record = next(r for r in ledger['records'] if r['id'] == owner['address'] + '-functional-c')
        assert record['implementation'] == 'func' and record['integration'] == 'linked_harness'
        assert not record['game_admitted'] and not record['validation']['raw_byte_match']
        assert record['source_sha256'] == sha(ROOT / record['source'])
        assert all(('_' + owner['function']).encode() in data for data in maps)
    for family in validation['families']:
        for build in (real, clean):
            assert family['scenario'] in build['scenarios']
    assert scene['scene'] == validation['scene'] and scene['scene']['textured_faces'] == 1798
    assert scene['scene']['triangles'] == 3112 and not scene['original_code_required']
    assert sha(FOLDER / 'scene.bmp') == validation['scene_sha256']
    for index, preset in enumerate(('red', 'green', 'point'), 1):
        result = read(FOLDER / ('light-' + preset + '.json'))
        assert result['passed'] and result['lighting_test'] == index and not result['original_code_required']
        assert (FOLDER / ('light-' + preset + '.bmp')).read_bytes()[:2] == b'BM'
    native = next(s for s in real['scenarios'] if s.get('scenario') == 'model-draw-native')
    assert native['checks'] == 71 and not native['failures']
    assert not validation['original_code_executed'] and validation['game_launches'] == 0
    print(json.dumps(dict(passed=True, complete_functions=31, original_instructions=10007,
                         real_checks=real['test']['checks'], clean_checks=clean['test']['checks'],
                         native_composition_checks=71, game_launches=0)))


if __name__ == '__main__':
    main()
