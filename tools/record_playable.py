"""Retain preview integration evidence without admitting untested gameplay."""
import copy
import hashlib
import json
import shutil
from pathlib import Path
from record_pickup_basic import ROOT, active, read, reference, sha, write

HERE = ROOT / 'META/m4/playable'


def main():
    ledger, _ = reference()
    real = read(ROOT / 'build/m0/result.json')
    report = read(ROOT / 'build/playable-menu-clean/clean-checkout.json')
    clean = report['result']
    game = read(ROOT / 'build/game/build.json')
    startup = read(ROOT / 'build/game/startup-check.json')
    assert all(b['passed'] for b in (real, clean, game, startup))
    assert not real['test']['failures'] and not clean['test']['failures']
    assert game['executable_sha256'] == startup['executable_sha256'] == sha(ROOT / game['target'])
    for name, digest in real['sources'].items():
        assert sha(ROOT / name) == digest, name
    for name, digest in game['sources'].items():
        assert sha(ROOT / name) == digest, name
    # Git normalizes the new raster fixture in the independent checkout.
    # Preserve both exact variants; do not claim byte-identical test inputs.
    normalized = {}
    for name, digest in clean['sources'].items():
        if digest != real['sources'][name]:
            data = (ROOT / name).read_bytes().replace(b'\r\n', b'\n')
            assert hashlib.sha256(data).hexdigest() == digest, name
            dest = HERE / 'sources' / (digest + '-clean-' + Path(name).name + '.txt')
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(data)
            normalized[name] = dest.relative_to(ROOT).as_posix()
    current = active(ledger)
    assert len(current) == 679
    changed = {r['source'] for r in current if r['source_sha256'] != real['sources'][r['source']]}
    assert changed == {
        'src/bh_world_segment.c', 'src/bh_horizon.c', 'src/bh_flyby_scene.c',
        'src/bh_script_motion.c', 'src/bh_render_execute.c',
        'src/bh_software_raster.c', 'src/bh_game_bootstrap.c'}
    snapshots = {}
    names = sorted(set(real['sources']) | set(game['sources']))
    # The complete input/hash manifests below retain unchanged contexts;
    # snapshot changed owners, adjacent headers, fixtures and preview adapters.
    names = [n for n in names if Path(n).stem in {
        Path(s).stem for s in changed} or n in (
            'tests/flyby_scene.c', 'tests/software_raster.c', 'tests/game_bootstrap.c',
            'tests/horizon.c', 'tests/script_motion.c', 'tests/world_segment.c',
            'src/bh_game_app.c', 'src/bh_game_app.h', 'src/bh_game_main.c')]
    names += ['tools/build_game.py', 'tools/game_compile_context.json',
              'tools/test_game_startup.py', 'tools/test_raster_regression.py',
              'play-source.cmd', 'build-source.cmd']
    for name in names:
        dest = HERE / 'sources' / (sha(ROOT / name) + '-' + Path(name).name + '.txt')
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / name, dest)
        snapshots[name] = dest.relative_to(ROOT).as_posix()
    historical = {}
    refreshed = []
    for old in current:
        source = old['source']
        if source not in changed:
            continue
        candidates = sorted((ROOT / 'META/m4').glob('**/sources/' + old['source_sha256'] + '-*.txt'))
        snapshot = next(p for p in candidates if sha(p) == old['source_sha256'])
        historical[source] = snapshot.relative_to(ROOT).as_posix()
        new = copy.deepcopy(old)
        new['id'] += '-preview-ownership'
        new['source_sha256'] = real['sources'][source]
        # Retain reviewed boundary/ABI provenance; replace execution evidence.
        boundary = old['validation'].get('boundary')
        new['validation'] = dict(boundary=boundary, supersedes=old['id'],
            raw_byte_match=False, original_code_executed=False,
            fixtures_evidence='META/m4/playable/integration-real.json',
            clean_fixtures_evidence='META/m4/playable/integration-clean.json',
            limits='Preview ownership variants; source fixtures and bounded startup only. Human gameplay remains pending; PC24, CD, DirectPlay peers and native hardware contracts remain open.')
        new['evidence'] = ['META/m4/playable/validation.json'] + old['evidence']
        old['historical_variant'] = True
        old['historical_source_path'] = source
        old['source'] = historical[source]
        old['validation']['superseded_by'] = new['id']
        ledger['records'].append(new)
        refreshed.append(dict(old=old['id'], new=new['id'], source=source))
    for old in list(ledger['support_implementations']):
        source = old.get('source')
        validation = old.get('validation', {})
        if source not in changed or old.get('historical_variant') or (isinstance(validation, dict) and validation.get('superseded_by')):
            continue
        new = copy.deepcopy(old)
        new['id'] += '-preview-ownership'
        new['source_sha256'] = real['sources'][source]
        new['validation'] = dict(supersedes=old['id'], original_code_executed=False,
                                 evidence='META/m4/playable/validation.json')
        new['evidence'] = ['META/m4/playable/validation.json']
        old['historical_variant'] = True
        old['historical_source_path'] = source
        old['source'] = historical[source]
        if not isinstance(old.get('validation'), dict):
            old['validation'] = dict(historical_contract=old.get('validation'))
        old.setdefault('validation', {})['superseded_by'] = new['id']
        ledger['support_implementations'].append(new)
    for name in ('src/bh_game_app.c', 'src/bh_game_main.c'):
        ledger['support_implementations'].append(dict(
            id='preview-' + Path(name).stem, source=name, source_sha256=sha(ROOT / name),
            role='unknown', provenance_confidence='New explicit source composition/platform adapter; no independent original function claim.',
            implementation='wip', integration='linked_game', target=game['target'],
            game_admitted=False, original_function_count=0,
            validation=dict(bounded_startup=True, original_code_executed=False,
                            human_gameplay='Pending menu/camera/sound retry'),
            evidence=['META/m4/playable/validation.json']))
    for r in active(ledger):
        assert r['source_sha256'] == sha(ROOT / r['source'])
    assert len(active(ledger)) == 679
    write(HERE / 'integration-real.json', real)
    write(HERE / 'integration-clean.json', report)
    write(HERE / 'game-build.json', game)
    write(HERE / 'startup-check.json', startup)
    shutil.copyfile(ROOT / 'build/m0/bhunter-integration.map', HERE / 'integration-map.txt')
    shutil.copyfile(ROOT / 'build/game/BHunter-source.map', HERE / 'game-map.txt')
    write(HERE / 'validation.json', dict(passed=True, new_owners=0,
        active_original_functions=679, game_admitted=False, implementation='wip',
        real_total=real['test'], clean_total=clean['test'],
        source_snapshots=snapshots, clean_normalized_test_snapshots=normalized,
        refreshed_variants=refreshed, game_translation_units=game['translation_units'],
        original_code_executed=False, automated_gameplay=False,
        execution_limit='One initial static scene render during bounded startup. No collected input, entity dispatch or application frames. Flyby constructor retains 50 settling calls.',
        human_evidence='META/m4/playable/human/menu-camera-sound.json',
        limits='Human gameplay pending. M4 incomplete; PC24, CD audio, native hardware and DirectPlay peers unvalidated.'))
    ledger['updated'] = '2026-10-26'
    ledger['current_source_frontier'] = 'META/m4/playable/plan.json'
    ledger['latest_source_session'] = 'META/m4/playable/validation.json'
    write(ROOT / 'META/function-status.json', ledger)
    observation = read(HERE / 'human/menu-camera-sound.json')
    observation['status'] = 'full_real_and_clean_source_checks_passed_human_retry_pending'
    write(HERE / 'human/menu-camera-sound.json', observation)
    print('Retained preview evidence; refreshed', len(refreshed), 'variants, zero new owners; M4 incomplete')


if __name__ == '__main__':
    main()
