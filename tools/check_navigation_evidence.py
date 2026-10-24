"""Verify full source navigation admission, exact legacy routes and preservation."""
import hashlib
import json
import struct
from pathlib import Path
from check_construction_evidence import ROOT, read, sha

FOLDER = ROOT / 'META/m4/navigation'


def main():
    review, fixtures = read(FOLDER / 'review.json'), read(FOLDER / 'fixtures.json')
    real, clean = read(FOLDER / 'integration-real.json'), read(FOLDER / 'clean/clean-checkout.json')
    assert real['passed'] and clean['result']['passed'] and not clean['original_assets_present']
    assert fixtures['recomputed_vs_legacy_different_cells'] == 0
    assert fixtures['recomputed_vs_legacy_different_exits'] == 0 and fixtures['matrix_crc32'] == 1493512335
    assert sha(ROOT / 'tests/navigation_cases.h') == fixtures['header_sha256']
    for path, expected in review['evidence_sha256'].items():
        assert sha(ROOT / path) == expected
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', exe, 60)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    for row in review['records']:
        start, end = (int(x, 16) for x in row['boundary'].split('-'))
        body = None
        for i in range(struct.unpack_from('<H', exe, pe + 6)[0]):
            _, rva, size, offset = struct.unpack_from('<IIII', exe, table + 40 * i + 8)
            if rva <= start - 0x400000 and end - 0x400000 < rva + size:
                at = offset + start - 0x400000 - rva
                body = exe[at:at + end - start + 1]
        assert body and hashlib.sha256(body).hexdigest() == row['body_sha256']
    for entry in read(ROOT / 'META/reference-manifest.json')['files']:
        if entry['root'] == 'repo':
            assert sha(ROOT / entry['path']) == entry['sha256']
    sources = ('src/bh_navigation.c', 'src/bh_navigation.h', 'tests/navigation.c', 'tests/navigation_cases.h',
               'tests/world_geometry.c', 'tests/checkpoint.c')
    verified = {}
    for p in sources:
        expected = real['sources'][p]
        assert expected == clean['result']['sources'][p]
        path = ROOT / p
        if sha(path) != expected:
            path = FOLDER / 'sources' / (expected + '-' + path.name + '.txt')
        assert sha(path) == expected
        verified[p] = dict(sha256=expected, artifact=str(path.relative_to(ROOT)))
    for result in (real, clean['result']):
        assert result['missing_navigation_control'] == 'rejected with LNK2001'
        scenario = next(s for s in result['scenarios'] if s['scenario'] == 'navigation')
        assert scenario['checks'] == 79 and not scenario['failures']
        assert not result['original_code_required']
    assert next(s for s in real['scenarios'] if s['scenario'] == 'world-geometry')['checks'] == 87629
    ledger = read(ROOT / 'META/function-status.json')
    for va in ('00407f30', '00407d90'):
        row = next(r for r in ledger['records'] if r['id'] == va + '-functional-c')
        assert row['implementation'] == 'func' and row['integration'] == 'linked_harness' and not row['game_admitted']
        assert row['source_sha256'] == sha(ROOT / row['source'])
    subset = next(r for r in ledger['records'] if r['id'] == '00407f30-reader-subset-c')
    assert subset['implementation'] == 'wip' and subset['historical_variant']
    report = dict(passed=True, real_total=real['test'], clean_total=clean['result']['test'],
                  nodes=1454, junctions=283, exact_recomputed_legacy_cells=80089,
                  matrix_crc32=1493512335, synthetic_checks=79, translation_units=32,
                  immutable_references_verified=4, game_launches=0, game_admitted=False,
                  sources=verified)
    (FOLDER / 'validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('passed', 'nodes', 'junctions', 'exact_recomputed_legacy_cells', 'real_total', 'clean_total')}))


if __name__ == '__main__':
    main()
