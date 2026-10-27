"""Retain per-owner original review separately from source regression results."""
import csv
import hashlib
import shutil
from pathlib import Path
from record_pickup_basic import ROOT, active, reference, read, write, sha
from record_live_actor_clock import original
from record_runtime_fixes import main as record

# Scope is deliberately per owner: a flow review is not an instruction match.
REVIEWS = [
 ('AI','00421610','Player eligibility, type2 filtering, query3 positions, weighted nearest target and list order.'),
 ('AI','004344f0','Preferred exit before exclusion, flags10/20, no-exit return, actor-local RNG step and rank.'),
 ('AI','00434650','Opposite portal direction, eligibility flags, up-cross-normal, minimum-score choice.'),
 ('AI','004355c0','Sound countdown, incoming portal search, guided/random selection, two actor RNG lerps, promotion.'),
 ('AI','00435790','Signed16 due test and original now-plus-overrun stamp.'),
 ('AI','00435720','Sound stop, room-list unlink and pool release.'),
 ('AI','00438ff0','Retarget RNG gates, forced path temporary state, room lookup, route promotion, same-room pursuit controls.'),
 ('AI','00437ae0','Target query, cooldown, immunity/range/aim gates, phase cadence, three weapon branches, effect RNG order and contact/damage packets.'),
 ('AI','004398c0','Attack then phase progression and pursuit transition.'),
 ('AI','004357d0','Damage decay, displacement damping, route due, smoke/death gates, actor-local wreck RNG, loot and retirement. Damping corrected.'),
 ('AI','00435e80','Event cases, pickup reply words5/6, impulse handling, damage transition, motion query and cleanup. Floating intermediates outside damping not exhaustively matched.'),
 ('AI','00439910','Attack, relaxation, kind6 decay, displacement damping, phase, smoke/death, loot and notifications. Damping corrected against original executed instructions.'),
 ('AI','0043a1d0','Event dispatch, kind6 damage cap, threat selection, impulse, pickup replies, target/motion query and retirement. Full numerical equivalence remains unverified.'),
 ('collision','0040abe0','Root/child/portal DFS order, visited flags, inclusive geometry bounds.'),
 ('collision','0040af20','Excluded actor, strict radius/bounds comparisons, global actor order and terminating sentinel.'),
 ('collision','0040b300','Cached room containment, zero-margin then radius lookup, previous-room fallback, geometry before actors.'),
 ('collision','0040b410','Zero/single/multiple candidates, moving-world relative motion, undo on miss, earliest selection and candidate swap.'),
 ('collision','0044da20','Full-pose overlap, segment fallback with recursion disabled, three bisections, cached-face fallback, clear pose and contact motion.'),
 ('collision','0044c200','Node sphere/plane pruning, source-face suppression, face traversal and recursive front-before-back order. Arithmetic not fully instruction-matched.'),
 ('collision','0044d2f0','Node0 traversal and result publication.'),
 ('collision','0044d480','Null geometry rejection and local*pose transform before collision query.'),
 ('collision','0040ba40','First strict portal crossing, aperture, radius actor query, room mesh then child meshes, recursion only without local hit.'),
 ('collision','0040b020','Global actor sphere projection, strict intersection, mesh test, nearest selection, material flag preservation.'),
 ('collision','0040c4a0','Cached-room/locate fallback, normalization, room pass then global actor pass, hit fraction. Projection sum order checked against x87, not decompiler order.'),
 ('collision','004124d0','Integration and acceleration reset, broad phase, first sweep, world-only last-clear recovery, slide, material response, mass sharing, contact callbacks and final pose. Scalar FP equivalence not exhaustively matched.'),
 ('shooting','004305f0','Ballistic cadence, ammo decrement, alternating muzzle, three spread RNG draws, creation/sound/network/recoil and phase increment.'),
 ('shooting','00430a30','Homing cadence, ammo, muzzle/target, RNG, projectile creation, sound/network/recoil.'),
 ('shooting','00431db0','Area activation gating, ammo and five-tick duration.'),
 ('shooting','00431f00','Five strengths, alternating recoil, projectile emission and duration decrement.'),
 ('shooting','0043ea90','Lifespan, segment/no-hit proximity, material branches, ricochet, absorb, effects/RNG, contact then damage, cleanup. Source degenerate-vector guard differs from original unwritten normalization output; unresolved.'),
 ('objectives','00443270','Wait template variable resolution, child48-byte state copy, loaded/actor overwrite, callback registration and forwarding.'),
 ('objectives','00442f40','Wait event matching, callback-before-decrement, zero unlimited, deferred delete and unloaded teardown; read actual instructions because decompile export is an error.'),
 ('objectives','004457c0','VM selector sum+5, loaded state, next pointer, origin, allocation/bind and output.'),
 ('objectives','00443ff0','Event4/20 teardown, event8 selector substitution, opcode dispatch, original13 pickup type partition, broadcast/owner/radar/portal operations. Source depth/count guards remain explicit adapter limits.'),
 ('objectives','00443ac0','Quarter-second schedule, player-only radar scan, radius versus owner bounds, latch, broadcast14, decrement/release.'),
 ('objectives','00443d40','Latch reset, one-second schedule, retained .98 health threshold, event14, +4 seconds, event17 on last count, release; checked instructions.'),
 ('objectives','00422570','Signed16 progression comparison/save and delayed4-second first/unlocked sound.'),
 ('objectives','004225f0','Story message action table, progression/unlocks, ending/control transitions, availability and heavy-ammo clearing. Strings previously recovered; no new string-byte claim.'),
 ('pickups','00441b80','Resource lookup and constructor initialization. Removed invented type<10 restriction; original VM sends20/21 unchanged.'),
 ('pickups','00441a00','Resource fallback, two RNG calls, offer min, draw callback, room/global insertion and event binding.'),
 ('pickups','004415a0','Timer, type filtering, query3/query13, strict6-unit offer, packet reply offsets, remaining/offer, sound and cleanup. Original-derived rounding-boundary fixture passes current compiled source; no speculative distance rewrite.'),
 ('pickups','00441570','Event20 defers cleanup, disables refresh/query.'),
 ('pickups','004414d0','Room/global doubly linked unlink and pool release.'),
 ('pickups','0041d7c0','Pickup event13 consumer contract only: accepted+14/consumed+18 and inventory response. Other player query events are outside this review row.'),
]

def main():
    ledger,raw=reference();folder=ROOT/'META/m4/playable/original-gameplay-review'
    paths=list((ROOT/'META/m4').rglob('*.json'))+list((ROOT/'META/m4').rglob('*.tsv'))
    owners={r['original_va']:r for r in active(ledger)}
    inventory={line.split('\t')[0]:line.split('\t') for line in (ROOT/'analysis/function-map.tsv').read_text(encoding='utf-8-sig').splitlines()}
    reviews=[]
    for system,address,scope in REVIEWS:
        row=owners[address]
        try:proof=original(raw,address,paths)
        except RuntimeError:
            size=int(inventory[address][2]);body=raw(int(address,16),size)
            artifacts=[p.relative_to(ROOT).as_posix() for p in (ROOT/'META').rglob(address+'.asm.txt')]
            proof=dict(address=address,bytes=size,sha256=hashlib.sha256(body).hexdigest(),reference='GAME_DATA/BHUNTER.EXE',assembly_artifacts=artifacts,
                       export_validation='Original reference body hashed; assembly text reviewed, not claimed as machine-readable byte-validated export.')
        reviews.append(dict(system=system,original_va=address,function=row['function'],source=row['source'],source_sha256=sha(ROOT/row['source']),
                            original=proof,review_scope=scope,result='corrected' if address in ('004357d0','00439910','00441b80') else 'reviewed_with_stated_limits',
                            whole_function_byte_match=False,whole_function_differential_execution=False))
    oracle=read(ROOT/'build/original-review/actor-damping.json');before=read(ROOT/'build/original-review/actor-damping-before.json')
    assert oracle['passed'] and oracle['source_sha256']==sha(ROOT/'src/bh_actor_response.c')
    assert before['exit_code']==1 and '"different_cases":171' in before['output']
    assert '"different_cases":0' in oracle['output']
    limits=[
      'Function-by-function static flow/ABI review is distinct from byte matching. No whole-function or whole-game1:1 equivalence claim; review scope is explicit for every owner.',
      'Independent dynamic evidence is limited to original00439910 damping instructions underPC53 and their instruction-identical arithmetic in004357d0;256 finite input cases, six output words each. PC24 remains unresolved.',
      'Original ballistic zero-vector normalization leaves output unwritten; source fails that degenerate path. Original wreck orientation is also unwritten input supplied by an explicit adapter. These are unresolved fidelity boundaries, not proven matches.',
      'Source validation/generation/topology/capacity/depth guards are adapters. Full numerical/RNG/mutation equivalence outside the reviewed scopes, other projectile families, CD audio and peer gameplay remain unverified.',
      'No software-renderer tests or automated gameplay ran. Hardware static startup is integration evidence only. Historical paired box traces do not compare against the original and do not validate this executable.'
    ]
    changes=['Allow original scripted basic pickup types20/21 using the actual22-entry resource table.',
             'Restore original retained displacement, sqrt, damping and asymmetric X/Y/Z spill order in traffic damage004357d0 and response00439910.',
             'Add immutable-original instruction oracle with failing-before171/256 and passing-after0/256; retain captured outputs in source-only regression checks.',
             'Expand focused connected VM, pickup, weapon, projectile, wait, segment and actor checks; add original-derived strict pickup-radius rounding boundary.']
    write(folder/'function-review.json',dict(reference_exe_sha256=ledger['reference_exe_sha256'],method='Original disassembly/decompilation versus linked source, not two source runs',
        owners=reviews,original_owners_added=0,limits=limits))
    record('original-gameplay-review',changes=changes,limits=limits)
    evidence=read(folder/'validation.json');evidence['function_review']='META/m4/playable/original-gameplay-review/function-review.json'
    evidence['original_code_executed']=True
    evidence['original_execution_scope']=oracle['scope']
    evidence['shipping_original_code_executed']=False
    for name in ('tools/record_original_gameplay_review.py','tools/check_actor_damping_original.py','tests/actor_damping_original.c','tests/actor_damping_golden.h'):
        snapshot=folder/'sources'/(sha(ROOT/name)+'-'+Path(name).name+'.txt');shutil.copyfile(ROOT/name,snapshot);evidence['source_snapshots'][name]=snapshot.relative_to(ROOT).as_posix()
    for name in ('actor-damping.json','actor-damping-before.json','actor-damping.log','actor-damping-before.log','actor_damping_original.h','pickup-boundary-before.log'):
        shutil.copyfile(ROOT/'build/original-review'/name,folder/name)
    evidence['comparison_artifacts']={p.relative_to(ROOT).as_posix():sha(p) for p in folder.iterdir() if p.is_file() and p.name!='validation.json'}
    write(folder/'validation.json',evidence)
    ledger=read(ROOT/'META/function-status.json')
    for collection in ('records','support_implementations'):
        for row in ledger[collection]:
            if row['id'] in evidence['refreshed_variants']:
                row['validation'].update(fixtures=['focused non-renderer gameplay closure','original-captured PC53 damping words','hardware-only static startup'],limits=limits,raw_byte_match=False,
                    function_review=evidence['function_review'],game_target='build/game/BHunter-source.exe')
    ledger['support_implementations'].append(dict(id='original-actor-damping-oracle',source='tests/actor_damping_original.c',source_sha256=sha(ROOT/'tests/actor_damping_original.c'),
       role='test_adapter',provenance_confidence='Isolated extracted original arithmetic block, constants relocated; never a shipping game dependency.',implementation='adapter',integration='linked_harness',
       target='build/original-review/actor_damping_original.exe',original_function_count=0,game_admitted=False,
       validation=dict(original_code_executed=True,raw_byte_match=False,scope=oracle['scope'],limits=limits),evidence=[evidence['function_review'],'META/m4/playable/original-gameplay-review/actor-damping.json']))
    write(ROOT/'META/function-status.json',ledger)
    print('Retained %d per-owner original review scopes; three corrected owners; no full1:1 claim.'%len(reviews))

if __name__=='__main__':main()
