"""Audit recovered sprite and recursive resource owners; never execute game code."""
import csv
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/sprite-world'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    validation = read(FOLDER / 'validation.json')
    owners = read(FOLDER / 'owners.json')
    real = read(FOLDER / 'integration-real.json')
    clean_record = read(FOLDER / 'clean/clean-checkout.json')
    clean = clean_record['result']
    scene = read(FOLDER / 'scene-real.json')
    ledger = read(ROOT / 'META/function-status.json')
    assert validation['passed'] and real['passed'] and clean['passed'] and scene['passed']
    assert not clean_record['original_assets_present'] and not clean_record['compiler_in_checkout']
    assert len(owners) == 4 and sum(o['instructions'] for o in owners) == validation['original_instructions']
    for name, digest in validation['sources'].items():
        assert sha(ROOT / validation.get('historical_source_snapshots', {}).get(name, name)) == digest, name
        for build in (real, clean, scene):
            if name in build['sources']:
                assert build['sources'][name] == digest, name
    for build in (real, clean):
        assert not build['original_code_required'] and not build['test']['failures']
        assert sum(c['label'].startswith('compile-') for c in build['commands']) == 142
        controls = [v for k, v in build.items() if k.startswith('missing_')]
        assert len(controls) == 71 and all('LNK2001' in v for v in controls)
        for scenario in ('sprite-billboard', 'sprite-oriented', 'draw-resource', 'sprite-native'):
            result = next(s for s in build['scenarios'] if s.get('scenario') == scenario)
            assert result == validation['focused'][scenario] and not result['failures']
    assert real['test'] == validation['real_total'] and clean['test'] == validation['clean_total']
    for row in read(ROOT / 'META/reference-manifest.json')['files']:
        if row['root'] == 'repo':
            assert sha(ROOT / row['path']) == row['sha256'], row['path']
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', exe, 60)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    sections = [struct.unpack_from('<IIII', exe, table + i * 40 + 8)
                for i in range(struct.unpack_from('<H', exe, pe + 6)[0])]

    def original(va, size):
        rva = va - 0x400000
        _, start, _, offset = next(s for s in sections if s[1] <= rva and rva + size <= s[1] + s[2])
        return exe[offset + rva - start:offset + rva - start + size]

    for owner in owners:
        with (ROOT / owner['original']).open() as stream:
            rows = list(csv.DictReader(stream, delimiter='\t'))
        at = int(owner['address'], 16)
        body = bytearray()
        for row in rows:
            assert int(row['address'], 16) == at
            data = bytes.fromhex(row['bytes'])
            body.extend(data)
            at += len(data)
        assert len(rows) == owner['instructions'] and len(body) == owner['bytes']
        assert hashlib.sha256(body).hexdigest() == owner['body_sha256']
        assert original(int(owner['address'], 16), len(body)) == body
        record = next(r for r in ledger['records'] if r['id'] == owner['address'] + '-functional-c')
        assert record['implementation'] == 'func' and record['integration'] == 'linked_harness'
        assert not record['game_admitted'] and not record['validation']['raw_byte_match']
        assert record['source_sha256'] == sha(ROOT / record['source'])
        for name in ('link-map.txt', 'clean/link-map.txt', 'scene-link-map.txt'):
            assert ('_' + owner['function']).encode() in (FOLDER / name).read_bytes()
    assert original(0x489f18, 4) == struct.pack('<f', -255)
    assert original(0x489f28, 8) == struct.pack('<d', -.8)
    assert original(0x489f30, 4) == struct.pack('<f', -.8)
    adapter = next(r for r in ledger['records'] if r['id'] == '0046e430-native-branch-wip')
    assert adapter['implementation'] == 'wip' and not adapter['validation']['complete_original_function']
    assert adapter['source_sha256'] == sha(ROOT / adapter['source'])
    assert scene['scene'] == validation['scene'] and scene['scene']['triangles'] == 3112
    assert sha(FOLDER / 'scene.bmp') == scene['image_sha256'] == sha(ROOT / 'META/m4/model-instance/scene.bmp')
    for name in validation['native_images']:
        assert sha(FOLDER / name) == validation['native_images'][name]
        assert (FOLDER / name).read_bytes()[:2] == b'BM'
    assert not validation['original_code_executed'] and validation['game_launches'] == 0
    print(json.dumps(dict(passed=True, complete_functions=4, original_instructions=validation['original_instructions'],
                         focused_checks=sum(s['checks'] for s in validation['focused'].values()),
                         real_checks=real['test']['checks'], clean_checks=clean['test']['checks'],
                         static_scene_identical=True, game_launches=0)))


if __name__ == '__main__':
    main()
