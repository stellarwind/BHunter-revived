"""Retain original-backed launch correction and bounded box execution evidence."""
import shutil
from record_pickup_basic import ROOT, read, write, sha
from record_runtime_fixes import main as record

def main():
    folder=ROOT/'META/m4/playable/ballistic-launch'
    oracle=read(ROOT/'build/launch-review/original.json')
    assert oracle['passed']
    assert sha(ROOT/'tests/ballistic_launch_golden.h')==oracle['golden_sha256']
    before=read(ROOT/'build/launch-review/before.json')
    output=before['commands'][-1]['output']
    assert not before['passed'] and output.count('muzzle words match')==48 and output.count('velocity words match')==30
    limits=['Original execution is restricted to a call-free arithmetic block of game-owned ballistic firing004305f0. No whole-function, PC24, full-state or full-game equivalence claim.',
            '128 finite arithmetic cases use arbitrary input components; these are not128 physically orthonormal vehicle poses. Exactly representable axis controls validate the original wrapper.',
            'Source box runs exercise real AI, collision, shots, damage, objectives and pickups; matching paired source traces establish only recorded-state repeatability, not original scenario equivalence.',
            'Other firing families, recoil numerical equivalence, original degenerate normalization/wreck orientation, CD audio and peer gameplay remain unverified. M4 remains incomplete.']
    record('ballistic-launch',changes=['Preserve ballistic muzzle centers until after side offset; preserve X velocity as double across first spread RNG draw, retaining original Y/Z float spills.',
        'Connected real firing/projectile checks compare six output words with128 original-block captures. Before:48 muzzle and30 velocity differences; after:zero.',
        'Rebuild source-only game and run paired900-tick hardware box playthroughs.'],limits=limits)
    evidence=read(folder/'validation.json')
    evidence.update(original_code_executed=True,original_execution_scope=oracle['scope'],gameplay_automation=True,
                    automation_scope='User-authorized isolated box only',shipping_original_code_executed=False)
    evidence['dependencies']=dict(owner='004305f0 game-owned ballistic firing; arithmetic shared within owner, no new owner admitted',
        inputs='Entity position, basis, velocity, gun vector; weapon ammo/phase; existing CRT RNG',
        integration='Real weapon fire, projectile construction/lifetime, sound, world, pool and effect definitions in runtime-fixes.exe and BHunter-source.exe',
        oracle='Test-only call-free original block with relocated immutable constants; no imports or game globals, no mapper expansion')
    for name in ['tools/check_ballistic_launch_original.py','tools/record_ballistic_launch.py','tests/ballistic_launch_original.c','tests/ballistic_launch_golden.h']:
        snapshot=folder/'sources'/(sha(ROOT/name)+'-'+(ROOT/name).name+'.txt');shutil.copyfile(ROOT/name,snapshot)
        evidence['source_snapshots'][name]=snapshot.relative_to(ROOT).as_posix()
    for name in ['original.json','original.h','before.json','before-source.c','before-playthrough.json','before-trace.csv','after.log']:
        shutil.copyfile(ROOT/'build/launch-review'/name,folder/name)
    for name in ['playthrough.json','box-trace-run1.csv','box-trace-run2.csv','level.json']:
        shutil.copyfile(ROOT/'build/box-test'/name,folder/name)
    box=read(folder/'playthrough.json');assert box['passed'] and box['deterministic_trace_match']
    evidence['box_playthrough']=(folder/'playthrough.json').relative_to(ROOT).as_posix()
    evidence['box_artifacts']={p.relative_to(ROOT).as_posix():sha(p) for p in folder.iterdir() if p.is_file() and p.name not in ('validation.json',)}
    write(folder/'validation.json',evidence)
    ledger=read(ROOT/'META/function-status.json')
    for row in ledger['records']+ledger['support_implementations']:
        if row['id'] in evidence['refreshed_variants']:
            row['validation'].update(fixtures=['original captured muzzle/velocity words in connected firing/projectile closure','paired hardware-only900-tick diagnostic box'],
                original_comparison=(folder/'original.json').relative_to(ROOT).as_posix(),game_target='build/game/BHunter-source.exe')
    ledger['support_implementations'].append(dict(id='ballistic-launch-original-oracle',source='tests/ballistic_launch_original.c',source_sha256=sha(ROOT/'tests/ballistic_launch_original.c'),
        role='test_adapter',implementation='adapter',integration='linked_harness',target='build/launch-review/oracle.exe',original_function_count=0,game_admitted=False,
        validation=dict(original_code_executed=True,scope=oracle['scope'],raw_byte_match=False,limits=limits),evidence=[(folder/'original.json').relative_to(ROOT).as_posix()]))
    write(ROOT/'META/function-status.json',ledger)
if __name__=='__main__':main()
