"""Read-only audit of the isolated original bootstrap trace, not source recovery."""
import json
from check_m3_startup_evidence import ROOT, read, sha, events, event

def main():
    initial=read('META/m3/startup/89994fa7c8f0.json')
    final=read('META/m3/startup/f7a780ff1c4d.json')
    refs={v['path']:v['sha256'] for v in read('META/reference-manifest.json')['files'] if v['root']=='repo'}
    for p,h in refs.items():assert sha(p)==h,p
    for r in (initial,final):
        assert r['passed'] and r['through_bootstrap'] and not r['m3_complete']
        assert r['original_hashes']==refs and r['original_file_patches']==0
        assert r['registry_before']==r['registry_after'] and r['private_key_absent_after']
        assert r['protected_windows_files_before']==r['protected_windows_files_after']
        assert r['cursor_warp_failure_adapter'] and r['stop_before_system_volume_write']
        for p,h in r['source_hashes'].items():assert sha(r['source_snapshots'][p])==h
        for f in r['profile_files']:
            p=r['profile_path']+'/'+f['name'];assert sha(p)==f['sha256'] and (ROOT/p).stat().st_size==f['bytes']
        assert event(r,'source-check-return')['eax']==1
        assert event(r,'directdraw-set-display-mode-result')['hresult']==0
        assert event(r,'render-mode-return')['eax']==1
        assert event(r,'breakpoints-armed')['count']==79
        assert event(r,'bootstrap-entry')['continue_bootstrap']
        assert event(r,'bootstrap-return')['eax']==1
        assert event(r,'breakpoints-restored')['verified']
        assert event(r,'private-key-cleanup')['result']==0 and event(r,'probe-complete')['passed']
        assert event(r,'process-exit')['probe_termination']
        assert not any(e['event'] in ('exception','message-box-boundary','volume-write-boundary','timeout') for e in events(r))
        denied=[e for e in events(r) if e['event']=='denied-cursor-warp']
        assert len(denied)==3 and all(not e['returned_success'] for e in denied)
        assert any(e['caller_return']==0x473ef1 and [e['x'],e['y']]==[160,100] for e in denied)
        stages=[e for e in events(r) if e['event']=='bootstrap-stage']
        assert [e['address'] for e in stages]==[0x4792bb,0x4792ce,0x4474c0,0x44757a,0x44758b,0x4475bc,0x479891,0x4733f2]
        assert stages[0]['eax']==1 and stages[0]['sound_device']!=0
        assert stages[1]['eax']==0
        assert stages[2]['entity_capacity']==9600 and stages[2]['entity_used']==0
        assert stages[3]['eax']==1 and stages[3]['model_count']==454 and stages[3]['entity_used']==755
        assert stages[4]['entity_used']==7796
        assert stages[5]['navigation_count_raw']==283 and stages[5]['eax']==80089 and stages[5]['entity_used']==7797
        assert stages[-1]['model_count']==454 and stages[-1]['entity_used']==7831 and stages[-1]['entity_capacity']==9600 and stages[-1]['navigation_count_raw']==283
        for e in events(r):
            if 'x87_control' in e:assert e['x87_control']==0x27f
            if e['event']=='thread-created':assert e['start_address']!=0x471e00,'Game rendering worker was outside this stop boundary'
        log=r['compatibility_log'];assert sha(log['path'])==log['sha256']
    # Both bootstrap captures belong to archived source variants; the later
    # first-frame extension has independent compile/link/run evidence.
    assert event(final,'bootstrap-return')['game_bootstrap_executed']
    stages=[e for e in events(final) if e['event']=='bootstrap-stage']
    assert stages[1]['directinput_interface'] and stages[1]['joystick_device']==0
    print(json.dumps(dict(passed=True,scope='Actual original bootstrap and world-loading boundaries only',bootstrap_return=1,model_count=454,entity_capacity=9600,entity_used=7831,navigation_nodes=283,directsound_init_return=1,directinput_interface_created=True,joystick_device_selected=False,main_thread_control='027f',game_render_worker_executed=False,orderly_shutdown=False,m3_complete=False)))

if __name__=='__main__':main()
