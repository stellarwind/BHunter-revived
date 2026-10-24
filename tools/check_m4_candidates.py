"""Verify candidate dispositions against actual source/build/reference evidence."""
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads((ROOT / path).read_text())


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(exe, start, end):
    pe = struct.unpack_from('<I', exe, 60)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    for i in range(struct.unpack_from('<H', exe, pe + 6)[0]):
        _, rva, size, offset = struct.unpack_from('<IIII', exe, table + 40 * i + 8)
        if rva <= start - 0x400000 and end - 0x400000 < rva + size:
            at = offset + start - 0x400000 - rva
            return exe[at:at + end - start + 1]
    raise AssertionError('Boundary outside immutable image')


def main():
    ledger = read('META/function-status.json')
    audit = read('META/m4/candidate-audit.json')
    build = read('build/m0/result.json')
    assert build['passed'] and not build['original_code_required']
    records = {r['id']: r for r in ledger['records']}
    assert len(records) == len(ledger['records'])
    completed = []
    for row in records.values():
        if row.get('historical_variant') or row['implementation'] not in ('func', 'bm'):
            continue
        if row['integration'] != 'linked_harness':
            continue
        path = ROOT / row['source']
        assert sha(path) == row['source_sha256'] == build['sources'][row['source']]
        completed.append(row['original_va'])
    assert len(completed) == len(set(completed))
    verified_references = 0
    for item in read('META/reference-manifest.json')['files']:
        if item['root'] == 'repo':
            assert sha(ROOT / item['path']) == item['sha256']
            verified_references += 1
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert len(audit['records']) == 10
    admitted, unfinished = [], []
    for row in audit['records']:
        start, end = (int(x, 16) for x in row['boundary'].split('-'))
        assert hashlib.sha256(body(exe, start, end)).hexdigest() == row['body_sha256']
        assert sha(ROOT / row['decompilation']) == row['decompilation_sha256']
        if row['disposition'] != 'completed_linked_source':
            assert row['remaining']
            original = records[row['id']]
            assert original['implementation'] == 'wip' and not original.get('historical_variant')
            unfinished.append(row['address'])
            continue
        admission = records[row['admission']]
        assert admission['implementation'] == 'func' and admission['integration'] == 'linked_harness'
        assert admission['original_va'] == row['address'] and not admission['game_admitted']
        old = records[row['id']]
        assert old['historical_variant'] and old['validation']['superseded_by'] == admission['id']
        evidence = read(row['evidence'])
        folder = (ROOT / row['evidence']).parent
        real = json.loads((folder / 'integration-real.json').read_text())
        clean = json.loads((folder / 'clean/clean-checkout.json').read_text())
        assert evidence['passed'] and real['passed'] and clean['result']['passed']
        assert not clean['original_assets_present'] and not real['original_code_required']
        assert evidence['real_total'] == real['test']
        assert evidence['clean_total'] == clean['result']['test']
        for name, expected in evidence['sources'].items():
            assert expected == real['sources'][name] == clean['result']['sources'][name]
            path = ROOT / name
            if sha(path) != expected:
                path = folder / 'sources' / (expected + '-' + path.name + '.txt')
            assert sha(path) == expected, (row['address'], name)
        admitted.append(row['address'])
    assert len(admitted) == 4 and len(unfinished) == 6
    missing = {k: v for k, v in build.items() if k.startswith('missing_') and k.endswith('_control')}
    assert len(missing) >= 23 and set(missing.values()) == {'rejected with LNK2001'}
    timer_validation = read('META/m4/script-timers/validation.json')
    assert timer_validation['passed'] and timer_validation['restored_functions'] == 6
    assert build['missing_script_timer_control'] == 'rejected with LNK2001'
    timer_review = read('META/m4/script-timers/review.json')
    for row in timer_review['records']:
        start, end = (int(x, 16) for x in row['boundaries'][0].split('-'))
        assert hashlib.sha256(body(exe, start, end)).hexdigest() == row['body_sha256']
        admission = records[row['address'] + '-functional-c']
        assert admission['implementation'] == 'func' and admission['integration'] == 'linked_harness'
    for name, expected in timer_validation['sources'].items():
        assert expected == build['sources'][name] == sha(ROOT / name)
    report = dict(passed=True, audited_active_candidates=10, admitted_from_audit=admitted,
                  unfinished=unfinished, current_distinct_linked_functions=len(completed),
                  real_total=build['test'], missing_definition_controls=len(missing),
                  timer_dependency_functions=6,
                  immutable_references_verified=verified_references, game_launches=0,
                  original_code_executed=False, source_game_admission=False,
                  limits=['Six owner routines remain incomplete; their data adapters are not promoted.',
                          'The source integration target is not a playable source game.'])
    (ROOT / 'META/m4/candidate-admission-validation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
