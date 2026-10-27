"""Record producer-side clock recovery, original contracts and residual limits."""
import hashlib
import csv
import shutil
from pathlib import Path
from record_pickup_basic import ROOT, active, reference, read, sha, write
from record_runtime_fixes import main as record


def original(raw, address, paths):
    candidates = [p for p in paths if p.name in (address+'.disasm.json', address+'.disassembly.json', address+'.raw.json', address+'.tsv')]
    for path in sorted(candidates):
        if path.suffix == '.tsv':
            with path.open(encoding='utf-8-sig', newline='') as stream:
                rows = list(csv.DictReader(stream, delimiter='\t'))
        else:
            rows = read(path)
        if not isinstance(rows, list) or not rows or rows[0].get('address') != address:
            continue
        body = bytearray()
        for row in rows:
            data = bytes.fromhex(row['bytes'])
            assert raw(int(row['address'], 16), len(data)) == data
            body.extend(data)
        return dict(address=address, path=path.relative_to(ROOT).as_posix(), bytes=len(body),
                    instructions=len(rows), sha256=hashlib.sha256(body).hexdigest())
    raise RuntimeError('Missing original body '+address)


def main():
    ledger, raw = reference()
    folder = ROOT / 'META/m4/playable/live-actor-clock'
    folder.mkdir(exist_ok=True)
    sources = {
        'src/bh_actor_route.c','src/bh_actor_route_lifetime.c','src/bh_actor_damage.c',
        'src/bh_actor_response.c','src/bh_actor_pursuit_route.c','src/bh_actor_construct.c',
        'src/bh_actor_vehicle_draw.c','src/bh_script_actor_draw.c','src/bh_script_actor_tick.c',
        'src/bh_script_actor_query.c','src/bh_script_actor_construct.c',
        'src/bh_script_actor_route.c','src/bh_actor_launch.c'}
    owners = [r for r in active(ledger) if r['source'] in sources]
    paths = list((ROOT / 'META/m4').rglob('*.json')) + list((ROOT / 'META/m4').rglob('*.tsv'))
    proof = []
    for r in owners:
        entry = original(raw, r['original_va'], paths)
        entry.update(function=r['function'], role=r['role'])
        proof.append(entry)
    proof.append(original(raw, '0041e850', paths))
    # Independent original anchors: the player callback updates the shared
    # clock; traffic/script draws read that address when actually called.
    for address in ('0041e850','00434920','004366d0','00436cd0'):
        entry = next(x for x in proof if x['address']==address)
        rows = read(ROOT / entry['path'])
        assert any('004dfac4' in str(x.get('operands', [])).lower() for x in rows), address
    related = []
    for address in ('0047a2f0','0047a7b0','0043c8f0','00403870','00413420','00413f80'):
        related.append(original(raw, address, paths))
    report = dict(
        original_code_executed=False, game_executions='Hardware-only bounded static startup; no gameplay/input dispatch.',
        original_proof=proof, related_original_proof=related,
        classification='Existing whole source game/engine owners. Clock accessor, binding and passive trace are count-zero source adapters; no CRT/helper/import/shared-tail recovery counts.',
        producer=dict(
            traffic='00434920/bh_actor_vehicle_draw signed normalized horizontal cross-product banking.',
            script_vehicle='004366d0/bh_script_actor_vehicle_draw has the same two projected tangent denominators.',
            defect='Application copied player_state.tick into route/draw/launch contexts before entity stepping. Original player0041e850 increments shared004dfac4 during stepping; later query/update/refresh callbacks read the new global. Source consumers kept the pre-step copy.',
            causal_path=['A normal actor constructor initializes its path at room center.',
                         'First route promotion can make first control and first tangent identical at that center.',
                         'When actor promotion runs before player increment, route stamp is tickN.',
                         'Original draw after entity stepping reads tickN+1; old source draw reads snapshotN.',
                         'Source therefore samples phase0 instead of1/duration_ticks. At phase0 the previous and current sample coincide, the backward projected tangent is zero, and banking computes0/0.',
                         'The scalar oracle given the original live tick and the newly bound source produce identical pose bytes. Deliberately stale binding reproduces NaN. Both traffic and scripted vehicle owners are checked.'],
            correction='One mutable application-owned live clock, read at each original consumption site. Removed per-frame actor clock copies. Route/query/damage/pursuit/constructor/script and launch consumers share it. Startup checks actual binding identity.',
            no_numerical_fallback=True, bank_formula_changed=False, rng_calls_changed=False,
            historical_limit='The old session did not retain actor identity or route inputs. This proves a producer-side integration defect and causal NaN path; exact retrospective identification of the resource475 instance is not recoverable from that log.'),
        lifetime_and_order=dict(
            clock_owner='BHGameApp.player_state.tick remains allocated until after actor contexts/entities are disposed.',
            wrap='Original signed16 elapsed/duration behavior retained across65535->0.',
            packet='Original00426d50 temporarily writes the low clock word for construction and restores it. Setter/getter now operate on the shared live word, including construction failure. Upper word preserved as in original.'),
        related_audit=[
            dict(family='Script vehicle bank004366d0, ordinary script draw00436cd0, script queries/ticks/constructors', finding='Same pre-step shadow-clock defect; corrected throughout the shared dependency closure, retaining paths and RNG.'),
            dict(family='Damage/response queries, route promotion/pursuit, actor construction', finding='Same004dfac4 dependencies; live reads replace snapshots, preventing inconsistent route stamp/phase and queried positions during the same entity pass.'),
            dict(family='Launch packet timestamp/receive constructor', finding='Same shared clock plus temporary override; repaired override/restore ownership so binding the live clock does not break packet construction.'),
            dict(family='Camera7/9 reciprocal offsets', finding='Byte-reviewed originals contain the same unprotected normalizations and camera-distance branches. No actor tick snapshot dependency identified here. No invented zero-distance handling added; degenerate original behavior remains unvalidated.'),
            dict(family='Player/debris coincident blast', finding='Original player0041d7c0 and debris0043c8f0 also divide by distance after the radius comparison, without a center-distance guard. No proven new integration omission in that arithmetic; no epsilon or force-direction fallback added.'),
            dict(family='Orbit00403870 identity/proximity', finding='Original has the same proximity-before-normalization distinction. Original nonzero-identity degeneracy remains a fidelity risk, not grounds to invent an acceleration.'),
            dict(family='Actor basis00413420/00413f80', finding='Original group normalization reciprocals also rely on constructed vectors; no original zero-vector branch found to restore. No blanket safety claim or numerical alteration.'),
            dict(family='Flyby and common vector/light helpers', finding='Previously recovered explicit zero checks remain; no clamp/fallback added in this block.')],
        passive_evidence='If a producer still emits nonfinite angles, source captures first owner address, entity/generation/model, live clock/stamp/duration, all curve controls and three actual samples as raw binary32 words. This never changes a sample, consumes RNG or advances gameplay. It is additional evidence, not the producer fix.',
        acceptance_limits=['No claim that the original never generatesNaN on genuinely degenerate input; original formulas are retained.',
                           'No 1:1 pixel/PC24/full-port completion or successful human scenario claimed.',
                           'Source coupled checks validate shared state and causal ordering; their count is not independent fidelity evidence. Original global loads/writes supply the contract.',
                           'No software-renderer test or automatic gameplay. No new human playtest prerequisite.'])
    write(folder / 'producer-audit.json', report)
    record('live-actor-clock', changes=[
        'Replace actor pre-step clock snapshots with original-style live shared clock reads at call sites.',
        'Repair route/query/constructor/script/launch clock closure and preserve packet low-word override/restore.',
        'Capture raw producer inputs on any remaining nonfinite curve output; no mathematics or RNG changes.',
        'Compile/link real owners and check live/fresh clock pose equality, stale-clock NaN causality, wrap and packet failure cleanup.'
    ], limits=report['acceptance_limits'])
    evidence = read(folder / 'validation.json')
    evidence['producer_audit'] = 'META/m4/playable/live-actor-clock/producer-audit.json'
    evidence['dependencies'] = dict(clock='One application-owned live004dfac4 equivalent; read-only views plus explicit packet low-word override.',
                                    owners=sorted(sources), arithmetic='Original curve/bank/normalization and RNG routines retained.')
    for name in ('tools/record_live_actor_clock.py',):
        snapshot = folder / 'sources' / (sha(ROOT/name)+'-'+Path(name).name+'.txt')
        shutil.copyfile(ROOT/name,snapshot)
        evidence['source_snapshots'][name]=snapshot.relative_to(ROOT).as_posix()
    write(folder/'validation.json',evidence)
    ledger = read(ROOT/'META/function-status.json')
    for collection in ('records','support_implementations'):
        for r in ledger[collection]:
            if r['id'] not in evidence['refreshed_variants']:
                continue
            r['validation']['fixtures']=['focused actor route/draw/query/construction/lifetime/packet closure',
                                         'live-clock versus fresh original-clock input equivalence and stale-clock rejection controls',
                                         'hardware static bootstrap clock identity']
            r['validation']['game_target']='build/game/BHunter-source.exe'
            r['validation']['producer_audit']='META/m4/playable/live-actor-clock/producer-audit.json'
    write(ROOT/'META/function-status.json',ledger)


if __name__ == '__main__':
    main()
