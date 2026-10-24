"""Audit queued resource construction/bindings without executing original game code."""
import csv
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/queued-resource'


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
        assert sum(c['label'].startswith('compile-') for c in build['commands']) == 149
        controls = [v for k, v in build.items() if k.startswith('missing_')]
        assert len(controls) == 74 and all('LNK2001' in v for v in controls)
        for scenario, expected in validation['real_focused' if build is real else 'clean_focused'].items():
            result = next(s for s in build['scenarios'] if s.get('scenario') == scenario)
            assert result == expected and not result['failures']
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
        for name in ('link-map.txt', 'clean/link-map.txt'):
            assert ('_' + owner['function']).encode() in (FOLDER / name).read_bytes()
    assert original(0x4796a9, 18).hex() == '681c2b4900e80d71fdff83c404a3b0004f00'
    assert original(0x492b1c, 13) == b'flare_yl.bmp\0'
    for va, value in [(0x489300, .05), (0x489438, -.1), (0x489d80, .87),
                      (0x489d98, 2.74), (0x489d88, 5.17), (0x489d90, -6),
                      (0x489558, .3), (0x489218, -.25), (0x489280, .5)]:
        assert original(va, 8) == struct.pack('<d', value)
    updated = next(r for r in ledger['records'] if r['id'] == '0045adf0-functional-c-queue-integration')
    assert updated['source_sha256'] == sha(ROOT / updated['source'])
    historical = next(r for r in ledger['records'] if r['id'] == '0045adf0-functional-c')
    assert historical['historical_variant'] and historical['source_sha256'] == sha(ROOT / historical['source'])
    assert scene['scene'] == validation['scene'] and scene['scene']['triangles'] == 3112
    assert sha(FOLDER / 'scene.bmp') == scene['image_sha256'] == sha(ROOT / 'META/m4/sprite-world/scene.bmp')
    assert b'_bh_draw_submit_execute' in (FOLDER / 'scene-link-map.txt').read_bytes()
    for name, digest in validation['native_images'].items():
        assert sha(FOLDER / name) == digest and (FOLDER / name).read_bytes()[:2] == b'BM'
    assert not validation['original_code_executed'] and validation['game_launches'] == 0
    print(json.dumps(dict(passed=True, complete_functions=4, original_instructions=validation['original_instructions'],
                         focused_checks=sum(s['checks'] for s in validation['real_focused'].values()),
                         real_checks=real['test']['checks'], clean_checks=clean['test']['checks'],
                         static_scene_identical=True, game_launches=0)))


if __name__ == '__main__':
    main()
