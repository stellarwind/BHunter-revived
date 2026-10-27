"""Record instruction-backed draw-loop exit diagnosis and its actual limits."""
import hashlib
import shutil
from record_pickup_basic import ROOT, read, write, sha, reference
from record_runtime_fixes import main as record


def main():
    ledger, raw = reference()
    folder = ROOT / 'META/m4/playable/nan-draw'
    verified = []
    for name in ('META/m4/draw-submit/original/004775f0.disassembly.json',
                 'META/m4/target600/b/original/00434920.disasm.json'):
        rows = read(ROOT / name)
        for row in rows:
            data = bytes.fromhex(row['bytes'])
            assert raw(int(row['address'], 16), len(data)) == data
        verified.append(dict(path=name, sha256=sha(ROOT / name), byte_verified=True))
    ranges = read(ROOT / 'META/m4/hud-continuation/original/00479090.ranges.json')
    for span in ranges['ranges']:
        body = bytes.fromhex(span['hex'])
        assert raw(int(span['start'], 16), len(body)) == body
        assert hashlib.sha256(body).hexdigest() == span['sha256']
    assert raw(0x477638, 8) == bytes.fromhex('8b465083c4385ec3')
    entry = bytes.fromhex(ranges['ranges'][0]['hex'])
    assert bytes.fromhex('50ff1083c40485c075f6') in entry
    report = dict(
        original_code_executed=False, original_gameplay_executed=False,
        verified_originals=verified, frame_owner_ranges=ranges,
        classification='Existing engine_candidate frame owner00479090 with its owned tail, game traffic draw00434920 and existing queued callback004775f0. No new original owner, CRT/library/import/EH recovery or general original mapper.',
        reported_session=dict(path='reported-source-session.log.txt', sha256=sha(folder / 'reported-source-session.log.txt'),
                              capture='reported-frame-times.csv', capture_sha256=sha(folder / 'reported-frame-times.csv'),
                              model_resource=475, command_index=196,
                              pose=[645.083496, 49.1670189, 10, -.671133101, 0, 'negative indefinite'],
                              first_draw_error=1, first_model_error=0, first_clip_error=0, hresult=0,
                              mechanism='Nonfinite resource pose rejected by bh_draw_resource before model dispatch; callback returns0; render_execute marks frame unsuccessful; render_frame calls failure13; normal source cleanup exits game.'),
        confirmed_root_cause=dict(
            mismatch='Source promoted callback drawing failure into frame/application failure. Original callback returns next command independently of the draw result, and original frame loop follows that pointer with no draw-success gate.',
            original_callback_anchor='00477633 CALL0045adf0;00477638 MOV EAX,[ESI+50];ADD ESP,38;POP ESI;RET',
            original_loop_anchor='00479090 entry contains PUSH EAX;CALL[EAX];ADD ESP,4;TEST EAX,EAX;JNZ loop;then jumps to owned tail00450930.',
            correction='Keep callback error/first failed pose as diagnostics; do not set frame return false for callback draw failures. Selector/topology/missing-binding boundaries and native tail failure handling remain explicit.',
            scope='Consumer-level correction applies to all queued callback draw failures, not only a resource475/NaN special case.'),
        nan_origin=dict(
            reproduced='Unchanged original traffic banking formula produces indefinite for zero horizontal forward or backward curve tangent. Repeated controls, vertical routes and endpoint tangents cover this class.',
            attribution='Plausible producer class; exact entity/route that produced the reported pose is NOT established. Session recorded no producer identity/control points/camera snapshot. Model handle alone does not identify a player.',
            math_changed=False, epsilon_added=False, neutral_angle_added=False, random_sequence_changed=False),
        related_review=[
            dict(site='bh_actor_vehicle_draw banking', status='Original unguarded arithmetic retained; reproducible nonfinite output. Frame-consumer fatal promotion corrected.'),
            dict(site='All BHRenderExecute callback kinds', status='Shared fatal promotion removed; diagnostic errors still retained. Missing selector/bindings remain source boundary failures.'),
            dict(site='bh_flyby_camera banking', status='Already checks both zero tangent lengths and uses original explicit zero-turn branches; no new guard added.'),
            dict(site='bh_light_eval coincident point/spot', status='Already skips stored zero distance; no new clamp added.'),
            dict(site='bh_math vector normalization', status='Original zero-length branch returns without reciprocal; no new fallback added.'),
            dict(site='bh_camera_modes7/9 normalized offsets', status='Unprotected denominator sites identified. Mode9 can have zero target-history displacement with a distant retained camera. Original-equivalence/invariant audit outstanding; not declared safe or silently modified.'),
            dict(site='bh_player_query/debris_response blast distance', status='Coincident blast/body distance can reach division. Original-contract audit outstanding; no epsilon or arbitrary force direction added.'),
            dict(site='bh_effect_orbit_tick target distance', status='Proximity branch usually retires before division, but nonzero identity bypass requires further original-contract review.'),
            dict(site='bh_actor_basis reciprocals', status='Depends on nonzero constructed direction/torque groups; broader asset/group invariant proof remains outstanding.')
        ],
        validation_design_failure='Earlier tests assumed well-conditioned moving curves and treated source-added failure returns as expected behavior. Their count was not independent original-equivalence evidence. New source checks preserve specific regressions; fidelity argument here comes from byte-verified original control flow.',
        proof_limits=[
            'No claim of complete root attribution of the historical NaN producer; confirmed cause of application exit is independently established.',
            'Unchanged numerical/RNG producer behavior is preserved, including nonfinite payloads. This does not establish byte/PC24 identity across the whole game.',
            'Original pixel/device behavior for degenerate transforms is not established. Current source resource guards reject such a draw; frame continuation matches original control flow.',
            'Other denominator sites are recorded risks, not completed or proven-safe fixes. No whole-port fidelity or release acceptance is claimed.',
            'No new human playtest prerequisite, gameplay automation or software renderer test.'
        ])
    write(folder / 'root-cause.json', report)
    record('nan-draw', changes=[
        'Restore original frame callback continuation: rendering failure is diagnostic and no longer becomes an application exit condition.',
        'Preserve first failing command/resource/pose diagnostics; add one bounded application warning.',
        'Keep all original numerical formulas, nonfinite payloads and RNG consumption unchanged; no epsilon or replacement angle.',
        'Hardware static startup consumes the recorded pose and nonfinite payload family followed by valid commands and finishes frames.'
    ], limits=report['proof_limits'])
    evidence = read(folder / 'validation.json')
    evidence['root_cause'] = 'META/m4/playable/nan-draw/root-cause.json'
    evidence['software_fixture_scope'] = 'Legacy render_execute assertions adjusted to original return semantics but not executed; no software test.'
    evidence['dependencies'] = dict(
        frame='Existing BHRenderExecute real callback, resource and hardware tail owners.',
        lifetime='Application warning flag only; resource/model/entity lifetimes unchanged.',
        numerical='All producer arithmetic and RNG behavior unchanged; recorded nonfinite resource rejection remains diagnostic.')
    for name in ('tools/record_nan_draw.py', 'tests/render_execute.c'):
        snapshot = folder / 'sources' / (sha(ROOT / name) + '-' + name.split('/')[-1] + '.txt')
        shutil.copyfile(ROOT / name, snapshot)
        evidence['source_snapshots'][name] = snapshot.relative_to(ROOT).as_posix()
    write(folder / 'validation.json', evidence)
    ledger = read(ROOT / 'META/function-status.json')
    for collection in ('records', 'support_implementations'):
        for item in ledger[collection]:
            if item['id'] not in evidence['refreshed_variants']:
                continue
            item['validation']['fixtures'] = ['hardware-only static callback continuation with recorded pose and nonfinite payload family']
            item['validation']['game_target'] = 'build/game/BHunter-source.exe'
            item['validation']['root_cause'] = 'META/m4/playable/nan-draw/root-cause.json'
    write(ROOT / 'META/function-status.json', ledger)


if __name__ == '__main__':
    main()
