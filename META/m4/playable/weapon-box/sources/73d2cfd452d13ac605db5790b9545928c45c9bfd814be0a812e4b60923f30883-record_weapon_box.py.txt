"""Retain all-weapon box coverage and original sound-error control flow."""
import hashlib, shutil
from record_pickup_basic import ROOT, read, write, sha, reference
from record_runtime_fixes import main as record

def main():
    ledger,raw=reference();folder=ROOT/'META/m4/playable/weapon-box'
    originals=[]
    for address,path in [('00479140','META/m4/frame-pipeline/service/original/00479140.disasm.json'),('0047c200','META/m4/sound-update/original/0047c200.disassembly.json')]:
        rows=read(ROOT/path);body=bytearray();start=int(address,16)
        for row in rows:
            at=int(row['address'],16);data=bytes.fromhex(row['bytes'])
            assert at==start+len(body) and raw(at,len(data))==data
            body.extend(data)
        originals.append(dict(address=address,role='game application orchestration' if address=='00479140' else 'engine audio owner; provenance unresolved',
            export=path,bytes=len(body),sha256=hashlib.sha256(body).hexdigest(),instructions=rows))
    service=originals[0]['instructions'];sound=originals[1]['instructions']
    assert next(r for r in service if r['address']=='004791c4')['operands']==['0x0047c200']
    assert next(r for r in service if r['address']=='004791c9')['mnemonic']=='MOV'
    for at in ('0047c61f','0047c9e1'):
        i=next(i for i,r in enumerate(sound) if r['address']==at)
        assert sound[i]['mnemonic']=='CALL' and '0x44' in sound[i]['operands'][0]
        assert sound[i+1]['mnemonic']=='MOV' # cache written without HRESULT gate
    failed=read(ROOT/'build/weapon-box-review/attempt1.json')
    assert not failed['passed'] and 'service=5 sound=4' in failed['runs'][0]['startup_log']
    box=read(ROOT/'build/box-test/playthrough.json')
    assert box['passed'] and box['deterministic_trace_match']
    for run in box['runs']:
        assert run['ticks']==2100 and len(run['weapons'])==10
        assert all(w['picked'] and w['fired'] for w in run['weapons'])
        assert run['checks']['all_projectile_families_spawned']
        assert 'frequency=-127800' in run['startup_log']
    limits=['User session ended at a generic frame-service error; it lacks the failing subsystem. The exact reported rocket-pickup exit is not conclusively attributed. Expanded box reproduces the same exit class during weapon8 effect playback.',
       'Two source-only2100-tick traces prove recorded-state repeatability, not original gameplay, complete state/RNG equivalence, or PC24 numerical equivalence.',
       'Pickup positions are staged fixture inputs at the current player position; real radius/timer/event13/selection/inventory handlers run. The driver never writes weapon availability/ammo or invokes firing callbacks directly.',
       'Seven projectile families have actual callback-bearing entities observed; lead buster/stun ray use emission counters plus ammo; static coating uses ammo plus actual positive boost timer. Exhaustive targeting/damage/lifetime equivalence is not claimed.',
       'Native sound failures remain logged. Original frequency/cache order is unchanged; no clamp, fallback frequency or RNG change. A later nonfatal draw diagnostic remains, and visual correctness is not verified.',
       'Software-renderer tests and original gameplay were not run. M4, CD audio and peer gameplay remain incomplete.']
    record('weapon-box',changes=['Extend isolated diagnostic to pick/select/fire all10 weapon types through actual gameplay owners; assert inventory, selection, emissions and seven real projectile callback families.',
        'Native sound API rejection no longer terminates frame production; original004791c4..c9 ignores the sound return. Source topology/numerical errors still fail.',
        'Record rejected frequency/resource/slot and detailed frame dependency errors; preserve failed-before box trace and user log.',
        'Connected real DirectSound silent-PCM fixture verifies rejection, unchanged cache, continuing frame publication and malformed-topology failure.'],limits=limits)
    evidence=read(folder/'validation.json')
    evidence.update(gameplay_automation=True,automation_scope='User-authorized isolated all-weapon box',box_expected_ticks=2100)
    evidence['dependencies']=dict(owner='00479140 frame producer consumes0047c200 sound owner. No new original owners.',
        integration='Real native DirectSound secondary buffers, source sound/input/entity/frame owners; no renderer in the focused sound fixture.',
        test_driver='Only diagnostic asset/setup and keyboard input are synthetic. All pickup, selection, fire, projectile and effect owners are linked production source.')
    for name in ['tools/record_weapon_box.py','tools/test_box_level.py','src/bh_game_box.inc']:
        snapshot=folder/'sources'/(sha(ROOT/name)+'-'+(ROOT/name).name+'.txt');shutil.copyfile(ROOT/name,snapshot);evidence['source_snapshots'][name]=snapshot.relative_to(ROOT).as_posix()
    write(folder/'original-contract.json',dict(reference_exe_sha256=ledger['reference_exe_sha256'],owners=originals,whole_function_match_claimed=False,
        conclusions=['Both original SetFrequency calls write their cache immediately without checking HRESULT.','Frame sound call overwrites EAX, then applies packet and publishes frame.'],limits=limits))
    for name in ['attempt1.json','attempt1-trace.csv','attempt1-build.json','user-startup.log','user-build.json']:
        shutil.copyfile(ROOT/'build/weapon-box-review'/name,folder/name)
    write(folder/'user-build-identity.json',dict(executable_sha256=sha(ROOT/'build/weapon-box-review/user-BHunter-source.exe'),retained_ignored_copy='build/weapon-box-review/user-BHunter-source.exe'))
    for name in ['playthrough.json','box-trace-run1.csv','box-trace-run2.csv','level.json']:
        shutil.copyfile(ROOT/'build/box-test'/name,folder/name)
    evidence['box_playthrough']=(folder/'playthrough.json').relative_to(ROOT).as_posix()
    evidence['box_artifacts']={p.relative_to(ROOT).as_posix():sha(p) for p in folder.iterdir() if p.is_file() and p.name!='validation.json'}
    write(folder/'validation.json',evidence)
    ledger=read(ROOT/'META/function-status.json')
    for row in ledger['records']+ledger['support_implementations']:
        if row['id'] in evidence['refreshed_variants']:
            row['validation'].update(fixtures=['real DirectSound rejection through frame producer','paired all-weapon hardware box2100 ticks','hardware static startup'],original_contract=(folder/'original-contract.json').relative_to(ROOT).as_posix(),game_target='build/game/BHunter-source.exe')
    write(ROOT/'META/function-status.json',ledger)
if __name__=='__main__':main()
