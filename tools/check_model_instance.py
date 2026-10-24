"""Audit the integrated model-instance owner without executing original game code."""
import csv
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/model-instance'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    review = read(FOLDER / 'review.json')
    validation = read(FOLDER / 'validation.json')
    ledger = read(ROOT / 'META/function-status.json')
    real = read(FOLDER / 'integration-real.json')
    clean_record = read(FOLDER / 'clean/clean-checkout.json')
    clean = clean_record['result']
    scene = read(FOLDER / 'scene-real.json')
    assert validation['passed'] and real['passed'] and clean['passed'] and scene['passed']
    assert not clean_record['original_assets_present'] and not clean_record['compiler_in_checkout']
    for name, digest in validation['sources'].items():
        assert sha(ROOT / validation.get('historical_source_snapshots', {}).get(name, name)) == digest, name
        for build in (real, clean, scene):
            if name in build['sources']:
                assert build['sources'][name] == digest, name
    for build in (real, clean):
        assert not build['original_code_required'] and not build['test']['failures']
        assert sum(c['label'].startswith('compile-') for c in build['commands']) == 134
        controls = [v for k, v in build.items() if k.startswith('missing_')]
        assert len(controls) == 67 and all('LNK2001' in v for v in controls)
        focused = next(s for s in build['scenarios'] if s.get('scenario') == 'model-instance')
        assert focused['checks'] == validation['focused_checks'] and not focused['failures']
    assert real['test'] == validation['real_total'] and clean['test'] == validation['clean_total']
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    for row in read(ROOT / 'META/reference-manifest.json')['files']:
        if row['root'] == 'repo':
            assert sha(ROOT / row['path']) == row['sha256'], row['path']
    pe = struct.unpack_from('<I', exe, 60)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    sections = [struct.unpack_from('<IIII', exe, table + i * 40 + 8)
                for i in range(struct.unpack_from('<H', exe, pe + 6)[0])]

    def original(va, size):
        rva = va - 0x400000
        _, start, _, offset = next(s for s in sections if s[1] <= rva and rva + size <= s[1] + s[2])
        return exe[offset + rva - start:offset + rva - start + size]

    path = ROOT / 'META/m4/model-draw/original/0045aa60.tsv'
    with path.open() as stream:
        rows = list(csv.DictReader(stream, delimiter='\t'))
    at = 0x45aa60
    body = bytearray()
    for row in rows:
        assert int(row['address'], 16) == at
        data = bytes.fromhex(row['bytes'])
        body.extend(data)
        at += len(data)
    assert len(rows) == review['body_instructions'] == 268
    assert len(body) == review['body_bytes'] == 899
    assert original(0x45aa60, len(body)) == body
    assert hashlib.sha256(body).hexdigest() == review['body_sha256']
    assert original(0x489f18, 4) == struct.pack('<f', -255)
    assert original(0x48c030, 48) == struct.pack('<12f', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0)
    assert original(0x495e30, 12) == bytes(12)
    record = next(r for r in ledger['records'] if r['id'] == '0045aa60-functional-c')
    assert record['implementation'] == 'func' and record['integration'] == 'linked_harness'
    assert not record['game_admitted'] and not record['validation']['raw_byte_match']
    assert record['source_sha256'] == sha(ROOT / record['source'])
    for name in ('link-map.txt', 'clean/link-map.txt', 'scene-link-map.txt'):
        assert b'_bh_model_instance_draw' in (FOLDER / name).read_bytes()
    assert scene['scene'] == validation['scene']
    assert scene['scene']['triangles'] == 3112 and scene['scene']['textured_faces'] == 1798
    assert sha(FOLDER / 'scene.bmp') == scene['image_sha256'] == sha(ROOT / 'META/m4/parallel-hour/scene.bmp')
    for preset in ('red', 'point'):
        result = read(FOLDER / ('light-' + preset + '.json'))
        assert result['passed'] and not result['original_code_required']
        assert sha(FOLDER / ('light-' + preset + '.bmp')) == sha(ROOT / ('META/m4/parallel-hour/light-' + preset + '.bmp'))
    assert not validation['original_code_executed'] and validation['game_launches'] == 0
    print(json.dumps(dict(passed=True, complete_functions=1, original_instructions=268,
                         focused_checks=validation['focused_checks'], real_checks=real['test']['checks'],
                         clean_checks=clean['test']['checks'], image_identical=True, game_launches=0)))


if __name__ == '__main__':
    main()
