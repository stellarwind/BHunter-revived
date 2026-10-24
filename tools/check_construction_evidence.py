"""Verify source geometry checkpoints and the original read-only contracts."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/construction'


def read(p):
    return json.loads(p.read_text(encoding='utf-8-sig'))


def sha(p):
    h = hashlib.sha256()
    with p.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    review = read(FOLDER / 'review.json')
    for f in read(ROOT / 'META/reference-manifest.json')['files']:
        if f['root'] == 'repo':
            assert sha(ROOT / f['path']) == f['sha256']
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', exe, 60)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    def raw(va, size):
        for i in range(struct.unpack_from('<H', exe, pe + 6)[0]):
            _, rva, n, at = struct.unpack_from('<IIII', exe, table + i * 40 + 8)
            if rva <= va - 0x400000 and va - 0x400000 + size <= rva + n:
                return exe[at + va - 0x400000 - rva:at + va - 0x400000 - rva + size]
        raise AssertionError(hex(va))
    for row in review['records']:
        start, end = (int(v, 16) for v in row['boundary'].split('-'))
        assert hashlib.sha256(raw(start, end - start + 1)).hexdigest() == row['body_sha256']
        for path, expected in row['evidence_sha256'].items():
            assert sha(ROOT / path) == expected
    for va, value in review['numerical_constants'].items():
        assert struct.unpack('<d', raw(int(va, 16), 8))[0] == value
    real = read(FOLDER / 'world-integration-real.json')
    clean = read(FOLDER / 'clean/clean-checkout.json')
    assert real['passed'] and clean['result']['passed'] and not clean['original_assets_present']
    modules = ('src/bh_geometry.c', 'src/bh_geometry.h', 'src/bh_rotation.c', 'src/bh_rotation.h',
               'src/bh_world_geometry.c', 'src/bh_world_geometry.h', 'tests/geometry.c',
               'tests/geometry_cases.h', 'tests/world_geometry.c', 'tests/world_geometry_cases.h')
    verified = {}
    for p in modules:
        expected = real['sources'][p]
        assert expected == clean['result']['sources'][p], p
        path = ROOT / p
        if sha(path) != expected:
            path = FOLDER / 'sources' / (expected + '-' + path.name + '.txt')
        assert sha(path) == expected, p
        verified[p] = dict(sha256=expected, artifact=str(path.relative_to(ROOT)))
    for result in (real, clean['result']):
        assert not result['original_code_required'] and not result['test']['failures']
        for name in ('geometry', 'rotation', 'world_geometry'):
            assert result['missing_' + name + '_control'] == 'rejected with LNK2001'
    for name, number in (('geometry-faces', 48609), ('world-geometry', 84714)):
        scenario = next(r for r in real['scenarios'] if r['scenario'] == name)
        assert scenario['checks'] == number and scenario['failures'] == 0
    report = dict(passed=True, functions=len(review['records']), real_total=real['test'],
                  clean_total=clean['result']['test'], reference_hashes_verified=4,
                  source_hashes_verified=verified,
                  game_launches=0, original_code_executed=False, game_admitted=False)
    for name in ('geometry-validation.json', 'world-validation.json'):
        (FOLDER / name).write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'functions', 'real_total', 'clean_total', 'game_launches')}))


if __name__ == '__main__':
    main()
