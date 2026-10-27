"""Record the sprite UV correction after full real/clean source checks."""
import copy
import hashlib
import shutil
from pathlib import Path
from record_pickup_basic import ROOT, active, read, reference, sha, write


def main():
    folder = ROOT / 'META/m4/playable/menu-uv'
    ledger, _ = reference()
    real = read(ROOT / 'build/m0/result.json')
    report = read(ROOT / 'build/playable-menu-uv-clean/clean-checkout.json')
    clean = report['result']
    game = read(ROOT / 'build/game/build.json')
    startup = read(ROOT / 'build/game/startup-check.json')
    assert all(x['passed'] for x in (real, clean, game, startup))
    assert not real['test']['failures'] and not clean['test']['failures']
    assert game['executable_sha256'] == startup['executable_sha256'] == sha(ROOT / game['target'])
    for name, digest in real['sources'].items():
        assert sha(ROOT / name) == digest, name
    for name, digest in game['sources'].items():
        assert sha(ROOT / name) == digest, name
    normalized = dict(read(ROOT / 'META/m4/playable/validation.json')['clean_normalized_test_snapshots'])
    for name in real['sources']:
        if clean['sources'][name] != real['sources'][name]:
            assert name in ('tests/software_raster.c', 'tests/sprite_quad_cases.h')
            data = (ROOT / name).read_bytes().replace(b'\r\n', b'\n')
            digest = hashlib.sha256(data).hexdigest()
            assert clean['sources'][name] == digest
            dest = folder / 'sources' / (digest + '-clean-' + Path(name).name + '.txt')
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
            normalized[name] = dest.relative_to(ROOT).as_posix()
    source = 'src/bh_sprite_quad.c'
    current = active(ledger)
    assert len(current) == 679
    old = next(r for r in current if r['source'] == source)
    assert old['source_sha256'] != real['sources'][source]
    old_snapshot = next(p for p in sorted((ROOT / 'META/m4').glob('**/sources/' + old['source_sha256'] + '-*.txt')) if sha(p) == old['source_sha256'])
    new = copy.deepcopy(old)
    new['id'] += '-menu-uv'
    new['source_sha256'] = real['sources'][source]
    real_fixture = next(s for s in real['scenarios'] if s.get('scenario') == 'sprite-quad')
    clean_fixture = next(s for s in clean['scenarios'] if s.get('scenario') == 'sprite-quad')
    assert real_fixture['checks'] == 660 and not real_fixture['failures'] and not clean_fixture['failures']
    new['validation'] = dict(boundary=old['validation'].get('boundary'),
        supersedes=old['id'], fixtures=real_fixture, clean_fixtures=clean_fixture,
        raw_byte_match=False, original_code_executed=False,
        limits='UV pairing corrected from retained original stack slots. Menu appearance awaits human confirmation; existing numerical, native and gameplay limits remain.')
    new['evidence'] = ['META/m4/playable/menu-uv/validation.json'] + old['evidence']
    old['historical_variant'] = True
    old['historical_source_path'] = source
    old['source'] = old_snapshot.relative_to(ROOT).as_posix()
    old['validation']['superseded_by'] = new['id']
    ledger['records'].append(new)
    snapshots = dict(read(ROOT / 'META/m4/playable/validation.json')['source_snapshots'])
    for name in (source, 'src/bh_sprite_quad.h', 'tests/sprite_quad.c',
                 'tests/sprite_quad_cases.h', 'tools/generate_sprite_quad_cases.py',
                 'tools/test_sprite_quad_regression.py'):
        dest = folder / 'sources' / (sha(ROOT / name) + '-' + Path(name).name + '.txt')
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, dest)
        snapshots[name] = dest.relative_to(ROOT).as_posix()
    for r in active(ledger):
        assert r['source_sha256'] == sha(ROOT / r['source']) == real['sources'][r['source']]
    write(folder / 'integration-real.json', real)
    write(folder / 'integration-clean.json', report)
    write(folder / 'game-build.json', game)
    write(folder / 'startup-check.json', startup)
    shutil.copyfile(ROOT / 'build/game/sprite-regression.json', folder / 'after.json')
    shutil.copyfile(ROOT / 'build/m0/bhunter-integration.map', folder / 'integration-map.txt')
    shutil.copyfile(ROOT / 'build/game/BHunter-source.map', folder / 'game-map.txt')
    write(folder / 'validation.json', dict(passed=True, new_owners=0, game_admitted=False,
        active_original_functions=679, implementation='wip', source_snapshots=snapshots,
        clean_normalized_test_snapshots=normalized,
        real_total=real['test'], clean_total=clean['test'], fixture=real_fixture,
        original_code_executed=False, automated_gameplay=False,
        original_evidence='META/m4/material-queue/original/00457590.c.txt',
        contract='A(-r,+t)=(u,v), B(-r,-t)=(u,v_end), C(+r,-t)=(u_end,v_end), D(+r,+t)=(u_end,v). Source previously exchanged B and D UV pairs. Original local_b4/local_78/local_3c/local_f0 identify these four corners.',
        human_observation='Menu now works and its frames are correct; interior text sprites are scaled incorrectly. Software rendering remains the default preview backend.',
        human_verification='Pending corrected label width/height/orientation inside existing frames.',
        limits='M4 incomplete. Preview defaults to software renderer; hardware, PC24, CD audio, native hardware and DirectPlay peers remain unvalidated. Bounded static startup only; no automated gameplay.'))
    ledger['latest_source_session'] = 'META/m4/playable/menu-uv/validation.json'
    write(ROOT / 'META/function-status.json', ledger)
    print('Recorded one UV source variant, zero new owners; 679 current harness owners.')


if __name__ == '__main__':
    main()
