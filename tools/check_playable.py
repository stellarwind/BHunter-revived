"""Read-only audit of retained source-preview evidence; never launches the game."""
from record_pickup_basic import ROOT, active, read, reference, sha


def main():
    ledger, _ = reference()
    here = (ROOT / ledger['latest_source_session']).parent
    evidence = read(here / 'validation.json')
    real = read(here / 'integration-real.json')
    clean = read(here / 'integration-clean.json')['result']
    game = read(here / 'game-build.json')
    startup = read(here / 'startup-check.json')
    assert all(b['passed'] for b in (evidence, real, clean, game, startup))
    assert not real['test']['failures'] and not clean['test']['failures']
    for name, digest in real['sources'].items():
        assert sha(ROOT / name) == digest, name
        clean_digest = clean['sources'][name]
        if clean_digest != digest:
            assert sha(ROOT / evidence['clean_normalized_test_snapshots'][name]) == clean_digest
    for name, digest in game['sources'].items():
        assert sha(ROOT / name) == digest, name
    for name, snapshot in evidence['source_snapshots'].items():
        assert sha(ROOT / name) == sha(ROOT / snapshot), name
    owners = active(ledger)
    assert len(owners) == 679
    for r in owners:
        assert r['source_sha256'] == real['sources'][r['source']] == sha(ROOT / r['source'])
    assert game['executable_sha256'] == startup['executable_sha256'] == sha(ROOT / game['target'])
    assert not game['original_code_required'] and not startup['original_code_required']
    assert not startup['input_automation'] and not startup['gameplay_automation']
    for name in ('input_collection_calls', 'entity_step_calls', 'entity_refresh_calls', 'application_frames'):
        assert startup['summary'][name] == 0, name
    print('Preview audit passed: 679 current harness owners, 299 game translation units; bounded static startup only; human retry pending.')


if __name__ == '__main__':
    main()
