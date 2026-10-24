"""Audit actual first-tick failure controls and adapted first-Flip evidence."""
import json
from check_m3_startup_evidence import ROOT,read,sha,events,event

def main():
    names=['dbb63b9aaab8','e95185853a02','0a085eeb1173','0a1b09078440','75fef4730703','b8497ba5a485']
    records={n:read('META/m3/startup/'+n+'.json') for n in names}
    refs={v['path']:v['sha256'] for v in read('META/reference-manifest.json')['files'] if v['root']=='repo'}
    for p,h in refs.items():assert sha(p)==h,p
    for n,r in records.items():
        assert r['through_first_frame'] and r['through_bootstrap'] and not r['m3_complete']
        assert r['original_file_patches']==0 and r['original_hashes']==refs
        assert r['registry_before']==r['registry_after'] and r['private_key_absent_after']
        assert r['protected_windows_files_before']==r['protected_windows_files_after']
        assert r['cursor_warp_failure_adapter'] and r['stop_before_system_volume_write']
        for p,h in r['source_hashes'].items():assert sha(r['source_snapshots'][p])==h,(n,p)
        for f in r['profile_files']:
            p=r['profile_path']+'/'+f['name'];assert sha(p)==f['sha256'] and (ROOT/p).stat().st_size==f['bytes']
        assert event(r,'source-check-return')['eax']==1 and event(r,'render-mode-return')['eax']==1
        assert event(r,'bootstrap-return')['eax']==1
        worker=next(e for e in events(r) if e['event']=='thread-created' and e['start_address']==0x471e00)
        assert worker['x87_control']==0x27f
        assert event(r,'private-key-cleanup')['result']==0
        assert not any(e['event']=='volume-write-boundary' for e in events(r))
        assert event(r,'process-exit')['probe_termination']
        log=r['compatibility_log'];assert sha(log['path'])==log['sha256']
        for e in events(r):
            if 'x87_control' in e:assert e['x87_control']==0x27f,(n,e)
    control=records['b8497ba5a485'];positive=records['75fef4730703'];aslr=records['0a085eeb1173']
    assert control['source_hashes']==positive['source_hashes']
    assert control['renderer_preset']['initial_cfg_sha256']==positive['renderer_preset']['initial_cfg_sha256']
    # Later menu/input variants have separate execution evidence; these first-
    # presentation comparisons retain their archived source snapshots.
    assert not control['passed'] and not control['crt_initial_heap_mib_adapter']
    assert not control['child_bottom_up_aslr_disabled'] and not positive['child_bottom_up_aslr_disabled']
    for r in (control,aslr):
        assert not r['passed'] and not any(e['event']=='first-simulation-return' for e in events(r))
        errors=[e for e in events(r) if e['event']=='exception']
        assert [(e['code'],e['first_chance'],e['address']) for e in errors]==[(0xc0000005,1,0x4755ce),(0xc0000005,0,0x4755ce)]
        failure=event(r,'access-violation-context')
        assert failure['operation']==1 and failure['fault_address']==4
        assert failure['eax']==failure['esi']==failure['binding_head']>=0x10000000 and failure['binding_head_readable']
        assert failure['memory_query_bytes']==28 and failure['region_state']==0x1000 and failure['region_type']==0x20000 and failure['region_protect']==4
        assert event(r,'breakpoints-restored')['verified']
        assert event(r,'process-aslr-policy')['query_succeeded'] and event(r,'process-aslr-policy')['flags']==0
    assert aslr['child_bottom_up_aslr_disabled'] and event(aslr,'process-aslr-policy')['bottom_up_off_requested']
    assert positive['passed'] and positive['crt_initial_heap_mib_adapter']==128
    adapter=event(positive,'crt-heap-initial-size-adapter')
    assert [adapter[k] for k in ('original_initial_bytes','adapted_initial_bytes','heap_flags','maximum_bytes')]==[4096,128*1024*1024,1,0]
    assert event(positive,'crt-heap-create-return')['actual_heap_handle']
    sim_in=event(positive,'first-simulation-entry');sim_out=event(positive,'first-simulation-return')
    render_in=event(positive,'first-render-entry');render_out=event(positive,'first-render-return')
    assert sim_in['simulation_tick']==0 and sim_out['simulation_tick']==1
    assert sim_in['thread_id']==sim_out['thread_id']!=render_in['thread_id']==render_out['thread_id']
    worker=next(e for e in events(positive) if e['event']=='thread-created' and e['start_address']==0x471e00)
    assert render_in['thread_id']==worker['thread_id']
    assert 0<sim_in['binding_head']<0x10000000
    assert event(positive,'first-surface-flip-result')['hresult']==0
    assert event(positive,'first-presentation-boundary')['thread_id']==worker['thread_id']
    assert event(positive,'breakpoints-restored')['verified'] and event(positive,'probe-complete')['passed']
    print(json.dumps(dict(passed=True,scope='Original first simulation/render/Flip diagnostics with explicit heap-size adapter',default_heap_first_tick_failed=True,aslr_only_trial_failed=True,heap_initial_mib=128,first_simulation_returned=True,renderer_callback_returned=True,first_surface_flip_hresult=0,observed_main_and_render_controls='027f',original_files_unchanged=True,visual_inspection=False,input_injection=False,normal_shutdown=False,m3_complete=False)))

if __name__=='__main__':main()
