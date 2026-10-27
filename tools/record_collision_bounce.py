"""Pin original collision contracts and retain focused source validation."""
import hashlib
import json
from record_pickup_basic import ROOT, active, reference, read, write
from record_runtime_fixes import main as record


def main():
    ledger, raw = reference()
    folder = ROOT / 'META/m4/playable/collision-bounce'
    originals = []
    for name in (
        'META/m4/hour443/b/004124d0.disassembly.json',
        'META/m4/target550/a/original/0041d7c0.disasm.json',
        'META/m4/entity-collision/original/0040b410.disassembly.json',
        'META/m4/swept-collision/original/0044da20.disassembly.json',
    ):
        rows = read(ROOT / name)
        body = bytearray()
        for row in rows:
            data = bytes.fromhex(row['bytes'])
            assert data == raw(int(row['address'], 16), len(data)), row['address']
            body.extend(data)
        owner = next(x for x in active(ledger) if x.get('original_va') == rows[0]['address'])
        originals.append(dict(path=name, role=owner['role'], address=rows[0]['address'],
                              bytes=len(body), instructions=len(rows), sha256=hashlib.sha256(body).hexdigest()))
    assert raw(0x412788, 11) == bytes.fromhex('8b480885c90f84d4000000')
    write(folder / 'original-contracts.json', dict(
        original_code_executed=False, byte_verified_functions=originals,
        boundary_roles='Existing whole game/engine collision owners; no CRT/library/EH/helper recovery assignments or independent shared-tail counts.',
        findings=[
            dict(id='lost-player-contact-state', status='fixed',
                 original='004124d0 calls event2 then retains entity acceleration/spin fields through finish. 0041d7c0 case2 subtracts normal impulse scaled by friction*900, applies contact torque scaled by mass*270/2, and damps spin by.85.',
                 source_before='Player query modified entity+60/+6c/+54; separate BHPhysicsState was republished over those writes. Before-fix regression recorded force/spin loss plus a torque assertion that wrongly assumed its centered mesh contact had nonzero torque; that assertion is replaced by an explicit off-center packet.',
                 source_after='Player constructor binds query.contact_physics. The actual event2 owner mirrors common entity state to that component before the enclosing physics finish; startup verifies the binding.',
                 validation='Actual decoded collision/sweep/contact chain with actual installed player query; bounce force and .85*.98 spin damping survive; controlled off-center event verifies angular-force transfer.'),
            dict(id='world-overlap-classification', status='fixed',
                 original='00412788 reads selected entity+8;0041278d JZ skips last-clear recovery for kind0 physical actors. Nonzero world/prop kind uses model+f8 and matrices+150/+180 for overlap retest.',
                 source_before='Last-clear recovery was gated on BH_WORLD_HIT_ACTOR instead of BH_WORLD_HIT_GEOMETRY.',
                 source_after='Recovery applies to world/prop geometry only. World penetration restores last-clear without RNG/effects; actor overlap follows ordinary contact path, emitting callbacks only for nonzero impulses.',
                 validation='Distinct real mesh fixtures for an embedded world collision and an embedded physical-actor collision.')],
        compared_contracts=[
            'Integrate acceleration/spin over dt, reset force accumulators, then perform broad/narrow collision detection.',
            'Original endpoint mesh tests, three bisections and optional origin-segment fallback retained; no replacement continuous solver introduced.',
            'Tangential slide uses (movement-normal*dot)*(friction*(1-first_fraction)); second sweep disables segment fallback.',
            'Normal push30+contact_speed*.00125 and signed impulse partition by masses retained; negative mass world objects do not receive a moving-body impulse.',
            'Moving world geometry subtracts per-tick translation and retains original1.5 contact-motion scaling.',
            'Final linear velocity is accepted displacement/dt*.995; angular damping.98 follows callback damping.85; original gravity sign retained.',
            'Ordinary contacts and special surface flags2/4/8 retain separate response arithmetic; original PC24 rounding behavior remains unresolved.'
        ],
        limits=['Static original comparison and source-only fixtures; no original or source gameplay execution.',
                'This is a selected collision/bounce audit, not proof of every collision mesh, portal, contact normal or extreme-speed case.',
                'Human tunnel-wall and prop comparisons remain pending; no software-renderer tests.']))
    record('collision-bounce', changes=[
        'Retain car event2 bounce acceleration and angular response by binding and synchronizing the actual source physics component.',
        'Correct zero-fraction penetration recovery to world/prop geometry, matching the original selected entity kind branch.',
        'Add real player callback regression and distinct world/actor overlap controls; extend non-renderer checks to response, sweep, broad phase and player query.'
    ], limits=[
        'Selected original bodies byte-verified as reference data only; no original code executed and no byte-match transfer to changed sources.',
        'Before-fix log retains three failures; force/spin are confirmed loss, while the torque fixture was corrected to use an explicitly off-center contact. Final checks also include a passing negative control with the state bridge intentionally absent.',
        'Focused source fixtures and hardware fullscreen bounded startup only; no full-suite, independent clean-checkout, software renderer or gameplay pass claimed.',
        'Manual tunnel-wall and prop fidelity review, PC24, CD audio and DirectPlay gameplay remain unvalidated; M4 incomplete.'
    ])
    evidence = read(folder / 'validation.json')
    evidence['original_contracts'] = 'META/m4/playable/collision-bounce/original-contracts.json'
    write(folder / 'validation.json', evidence)


if __name__ == '__main__':
    main()
