"""Retain original packet contract, connected regression and bounded box evidence."""
import csv
import shutil
from pathlib import Path
from record_pickup_basic import ROOT, active, read, reference, sha, write
from record_live_actor_clock import original
from record_runtime_fixes import main as record

def main():
    ledger, raw = reference()
    folder=ROOT/'META/m4/playable/pickup-box'
    folder.mkdir(parents=True,exist_ok=True)
    paths=list((ROOT/'META/m4').rglob('*.json'))+list((ROOT/'META/m4').rglob('*.tsv'))
    proof=[original(raw,a,paths) for a in ('004415a0','0041d7c0','0043a1d0','00435e80','0043cd00')]
    # Independent packet-base/reply-load anchors in both original pickup loops.
    anchors={'00441785':'8d442448','00441799':'8b442464','004417a4':'8b4c2460',
             '0044185a':'8d442448','0044186e':'8b442464','00441879':'8b4c2460'}
    for address,expected in anchors.items():assert raw(int(address,16),len(expected)//2).hex()==expected
    before=(ROOT/'build/pickup-before-fix.log').read_text(encoding='utf-8-sig')
    assert all('FAIL player-query '+str(n)+':' in before for n in (78,79,80,81))
    play=read(ROOT/'build/box-test/playthrough.json')
    game=read(ROOT/'build/game/build.json')
    assert play['passed'] and play['deterministic_trace_match']
    for run in play['runs']:assert run['executable_sha256']==game['executable_sha256'] and run['passed']
    assert sha(ROOT/'build/box-test/box-trace-run1.csv')==sha(ROOT/'build/box-test/box-trace-run2.csv')==play['runs'][0]['trace_sha256']
    limits=[
        'No raw-byte-match or full original/source differential-playthrough claim. Static original packet instructions establish the corrected ABI; source scenario success is not independent numerical fidelity evidence.',
        'Two fixed-seed 900-tick hardware box runs compare recorded state, not complete memory/RNG state. Human visual confirmation is pending: Windows screenshot capture timed out.',
        'The box driver is an isolated count-zero test adapter. It supplies level data and bounded input only, using real gameplay callbacks; it is not an original implementation.',
        'M4, 24-bit x87 equivalence, CD audio, peer gameplay and broader gameplay coverage remain incomplete. No software-renderer tests or tutorial/mission automation executed.'
    ]
    audit=dict(original_code_executed=False,original_bodies_verified_against_reference=proof,packet_anchors=anchors,
        contract='Original004415a0 packet base ESP+48. After two argument pushes, acceptance load ESP+64 corresponds to packet+14; after stack restoration consumed load ESP+60 corresponds to packet+18. Player0041d7c0 and actors0043a1d0/00435e80 write words5/6. Seven words, no reserved word.',
        defect='Source BHPickupTakePacket inserted a reserved word, so producer read words6/7. Real inventory could increase while pickup remained undepleted. Isolated mock used the same erroneous type and missed the cross-owner mismatch.',
        change='Remove reserved word; compile-time size28/accepted+14/consumed+18 assertions. Connected real pickup -> player query -> weapon registry check covers radius boundary, partial capacity, full rejection, final depletion and room/global unlink.',
        negative_control='Same connected fixture with old packet failed four assertions; retained before-fix log. Corrected real production closure passes.',
        reference_comparison_scope='Fresh original instruction/ABI review for pickup, player and actor reply owners, plus immutable-byte validation of wreck owner. Existing AI/collision/weapon/objective contracts remain linked below; no fresh whole-body matching claim.',
        prior_family_evidence=[{'function':r['function'],'original_va':r['original_va'],'source':r['source'],'evidence':r.get('evidence',[])} for r in active(ledger) if r['source'] in {'src/bh_actor_route.c','src/bh_physics_response.c','src/bh_entity_collision.c','src/bh_swept_collision.c','src/bh_weapon_fire.c','src/bh_ballistic.c','src/bh_script_wait.c','src/bh_script_vm.c','src/bh_target_step.c'}],
        limits=limits)
    write(folder/'original-contract.json',audit)
    record('pickup-box',changes=[audit['change'],
        'Separate BHunter-box.exe boots directly into two connected box rooms, solid post, pickups, route AI and waitfor/VM objective; original assets remain immutable.',
        'Fixed-seed input driver runs the real application loop and validates AI motion, projectiles, damage, destruction, wreck drop collection, objective and collision/bounds. Two traces match exactly.'
    ],limits=list(limits))
    evidence=read(folder/'validation.json')
    evidence.update(gameplay_automation=True,automation_scope='Explicitly user-authorized isolated diagnostic box only',
                    original_contract='META/m4/playable/pickup-box/original-contract.json',box_playthrough='META/m4/playable/pickup-box/playthrough.json',
                    dependencies=dict(integration='Real pickup/player query/weapon registry owners and production game loop; diagnostic .inc is a separate source adapter.',data='Generated loose BSP/LEV assets and existing immutable archive; no reference executable code loaded.'))
    evidence['limits']=limits
    for name in ('tools/record_pickup_box.py','tools/build_box_level.py','tools/test_box_level.py','tools/build_game.py'):
        snapshot=folder/'sources'/(sha(ROOT/name)+'-'+Path(name).name+'.txt');shutil.copyfile(ROOT/name,snapshot)
        evidence['source_snapshots'][name]=snapshot.relative_to(ROOT).as_posix()
    artifacts={}
    for name in ('playthrough.json','box-trace-run1.csv','box-trace-run2.csv','level.json','test.lev','bsp/blocks.txt','bsp/boxa.bsp','bsp/boxb.bsp','bsp/boxpost.bsp'):
        destination=folder/'box'/name;destination.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(ROOT/'build/box-test'/name,destination)
        artifacts[destination.relative_to(ROOT).as_posix()]=sha(destination)
    shutil.copyfile(ROOT/'build/box-test/playthrough.json',folder/'playthrough.json')
    shutil.copyfile(ROOT/'build/pickup-before-fix.log',folder/'pickup-before-fix.log')
    evidence['box_artifacts']=artifacts
    write(folder/'validation.json',evidence)
    ledger=read(ROOT/'META/function-status.json')
    for collection in ('records','support_implementations'):
        for row in ledger[collection]:
            if row['id'] not in evidence['refreshed_variants']:continue
            row['validation'].update(fixtures=['connected pickup/player-query regression','focused non-renderer source checks','hardware-only static startup','two bounded900-tick diagnostic box playthroughs'],limits=limits,
                raw_byte_match=False,original_contract=evidence['original_contract'],box_playthrough=evidence['box_playthrough'],game_target=game['target'])
    ledger['support_implementations'].append(dict(id='diagnostic-box-adapter',source='src/bh_game_box.inc',source_sha256=sha(ROOT/'src/bh_game_box.inc'),
        role='test_adapter',provenance_confidence='New diagnostic setup/input/observation adapter; no original owner claim.',implementation='adapter',integration='linked_game',
        target='build/box-test/BHunter-box.exe',original_function_count=0,game_admitted=False,
        validation=dict(original_code_executed=False,raw_byte_match=False,limits=limits),evidence=[ledger['latest_source_session'],evidence['box_playthrough']]))
    write(ROOT/'META/function-status.json',ledger)
    print('Retained pickup packet proof and two matching box playthroughs;679 original owners unchanged.')

if __name__=='__main__':main()
