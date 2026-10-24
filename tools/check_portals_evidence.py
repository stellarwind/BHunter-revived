"""Audit linked portal restoration evidence and immutable originals; no execution."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FOLDER = ROOT / 'META/m4/portals'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def main():
    review = read(FOLDER / 'review.json')
    fixture = read(FOLDER / 'fixture.json')
    real = read(FOLDER / 'integration-real.json')
    clean = read(FOLDER / 'clean/clean-checkout.json')
    for path, sha in review['evidence_sha256'].items():
        assert digest(ROOT / path) == sha, path
    assert digest(ROOT / fixture['source']) == fixture['source_sha256']
    assert digest(ROOT / fixture['fixture']) == fixture['fixture_sha256']
    assert fixture['unique_candidate_check'] and fixture['connected'] == 3120
    assert fixture['game_launches'] == 0
    for record in read(ROOT / 'META/reference-manifest.json')['files']:
        if record['root'] == 'repo':
            assert digest(ROOT / record['path']) == record['sha256']
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', exe, 60)[0]
    base = struct.unpack_from('<I', exe, pe + 52)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    def raw(va, size):
        for i in range(struct.unpack_from('<H', exe, pe + 6)[0]):
            _, rva, length, offset = struct.unpack_from('<IIII', exe, table + 40 * i + 8)
            if rva <= va - base and va - base + size <= rva + length:
                return exe[offset + va - base - rva:offset + va - base - rva + size]
        raise AssertionError(hex(va))
    start, end = int(review['address'], 16), int(review['end'], 16)
    assert hashlib.sha256(raw(start, end - start + 1)).hexdigest() == review['body_sha256']
    call = int(review['caller'], 16)
    assert raw(call, 5).hex() == review['caller_bytes']
    assert call + 5 + struct.unpack('<i', raw(call + 1, 4))[0] == start
    for address, value in review['dependencies']['constants'].items():
        assert struct.unpack('<d', raw(int(address, 16), 8))[0] == value
    assert real['passed'] and clean['exit_code'] == 0 and not clean['original_assets_present']
    for result in (real, clean['result']):
        assert result['passed'] and not result['original_code_required']
        assert result['missing_portals_control'] == 'rejected with LNK2001'
        for source, sha in result['sources'].items():
            assert digest(ROOT / source) == sha, source
        scenario = next(s for s in result['scenarios'] if s['scenario'] == 'portal-matching')
        assert scenario['checks'] == 57578 and scenario['failures'] == 0
        compile_commands = [c for c in result['commands'] if c['label'].startswith('compile-')]
        assert len(compile_commands) == 25 and all(c['exit_code'] == 0 for c in compile_commands)
    link_map = (FOLDER / 'link-map.txt').read_text(encoding='latin-1')
    assert '_bh_portals_match' in link_map and 'bh_portals.obj' in link_map
    ledger = read(ROOT / 'META/function-status.json')
    row = next(r for r in ledger['records'] if r['id'] == '00408940-functional-c')
    assert row['implementation'] == 'func' and row['integration'] == 'linked_harness'
    assert not row['game_admitted'] and row['source_sha256'] == digest(ROOT / row['source'])
    report = dict(passed=True, implementation=row['id'], target=real['target'],
                  translation_units=25, portal_checks=57578, captured_portals=3739,
                  captured_connections=3120, real_total=real['test'], clean_total=clean['result']['test'],
                  missing_module_control=real['missing_portals_control'], immutable_references_verified=4,
                  game_launches=0, original_code_executed=False, source_game_admission=False,
                  source_sha256={s: digest(ROOT / s) for s in ('src/bh_portals.c', 'src/bh_portals.h', 'tests/portals.c', 'tests/portal_cases.h')})
    (FOLDER / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
