"""Read-only audit of original menu objects and child-only key-message traces."""
import json
from check_m3_startup_evidence import ROOT,read,sha,events,event

def menus(r,phase):return {e['global_slot']:e for e in events(r) if e['event']=='menu-inventory' and e['phase']==phase}
def labels(m):return [e['label'] for e in m['items']]
def action_flag(e):return e.get('episode_action_flag',e.get('start_pending'))

def main():
    names=['3fa87b3191c2','9e7445abb9a7','1ad993c8a666','3e8c09b297f2','1a9aaba8c7d0','73beffa6a63f']
    records={n:read('META/m3/startup/'+n+'.json') for n in names}
    refs={v['path']:v['sha256'] for v in read('META/reference-manifest.json')['files'] if v['root']=='repo'}
    for p,h in refs.items():assert sha(p)==h,p
    for n,r in records.items():
        assert r['original_file_patches']==0 and r['original_hashes']==refs and not r['m3_complete']
        assert r['registry_before']==r['registry_after'] and r['private_key_absent_after']
        assert r['protected_windows_files_before']==r['protected_windows_files_after']
        assert r['crt_initial_heap_mib_adapter']==128 and r['cursor_warp_failure_adapter']
        for p,h in r['source_hashes'].items():assert sha(r['source_snapshots'][p])==h,(n,p)
        for f in r['profile_files']:
            p=r['profile_path']+'/'+f['name'];assert sha(p)==f['sha256'] and (ROOT/p).stat().st_size==f['bytes']
        assert event(r,'source-check-return')['eax']==1 and event(r,'bootstrap-return')['eax']==1
        assert event(r,'first-surface-flip-result')['hresult']==0
        assert event(r,'private-key-cleanup')['result']==0 and event(r,'process-exit')['probe_termination']
        assert not any(e['event'] in ('volume-write-boundary','exception','timeout') for e in events(r))
        log=r['compatibility_log'];assert sha(log['path'])==log['sha256']
        if r['passed']:assert event(r,'breakpoints-restored')['verified'] and event(r,'probe-complete')['passed']
        m=menus(r,'bootstrap-return');assert len(m)==18 and sum(len(e['items']) for e in m.values())==118
        assert labels(m[0x495e40])==['GAME','NETWORK','SETUP','QUIT']
        assert labels(m[0x4cdc9c])==['BARLETTA','STINGER','VIPER','BACK']
        assert labels(m[0x4ce450])==['prologue']+['episode '+str(i) for i in range(1,9)]+['BACK']
        for e in events(r):
            if 'x87_control' in e:assert e['x87_control']==0x27f
            if e['event']=='child-key-message':assert e['child_pid']==e['verified_window_owner'] and e['posted']
    idle=records['3fa87b3191c2'];drift=records['9e7445abb9a7'];guard=records['3e8c09b297f2'];final=records['73beffa6a63f']
    assert idle['passed'] and event(idle,'observation-boundary')['completed_simulation_callbacks']==300
    assert not any(e['event']=='child-key-message' for e in events(idle))
    assert menus(idle,'observation-stop')[0x495e40]['step_callback']==0x42b2e0
    assert not any(e['event']=='episode-action-return' for e in events(drift))
    assert not guard['passed'] and event(guard,'probe-error')['kind']==17
    assert next(e for e in events(guard) if e['event']=='menu-input-precondition' and e['after_simulation_callbacks']==360)['selection']==1
    assert final['passed'] and final['requested_menu_path_observed'] and final['input_injection_used']
    assert final['relative_cursor_adapter'] and not final['desktop_input_sent'] and not final['visual_ui_tooling_used']
    # Historical menu evidence belongs to its archived source snapshots, checked above.
    assert event(final,'observation-boundary')['completed_simulation_callbacks']==600
    keys=[e for e in events(final) if e['event']=='child-key-message']
    assert [(e['after_simulation_callbacks'],e['virtual_key'],e['down']) for e in keys]==[(320,13,True),(322,13,False),(360,13,True),(362,13,False),(400,13,True),(402,13,False)]
    e=event(final,'episode-action-entry');assert e['episode_index']==0
    e=event(final,'episode-action-return');assert action_flag(e)==1 and e['selected_episode']==0
    states={e['after_simulation_callbacks']:e for e in events(final) if e['event']=='menu-input-state'}
    for down,up in [(322,324),(362,364),(402,404)]:assert states[down]['enter_key_state']==1 and states[up]['enter_key_state']==0
    assert states[440]['menus'][1]['selection']==1 and action_flag(states[440])==1
    cursor=[e for e in events(final) if e['event']=='relative-cursor-adapter'];assert len(cursor)==600
    previous=None
    for e in cursor:
        assert e['native_success'] and e['initial_sample']==(previous is None)
        expected=[160,100] if previous is None else [160+e['physical_x']-previous['physical_x'],100+e['physical_y']-previous['physical_y']]
        assert [e['relative_x'],e['relative_y']]==expected
        previous=e
    end=menus(final,'observation-stop')
    assert all(end[s]['step_callback']==0x47d680 for s in [0x495e40,0x4cdc9c,0x4ce450])
    assert labels(end[0x4ce450])==labels(menus(final,'bootstrap-return')[0x4ce450])
    assert [e['action_callback'] for e in end[0x4ce450]['items'][:9]]==[0x42ca10]+[0]*8
    print(json.dumps(dict(passed=True,scope='Original constructed menu inventory and actual child-key navigation only',menus=18,menu_items=118,idle_callbacks=300,navigation_callbacks=600,ship='STINGER',episode_index=0,episode_action_observed=True,relative_cursor_samples=600,desktop_input_sent=False,visual_tools_used=False,mission_movement_objectives_validated=False,normal_shutdown=False,m3_complete=False)))

if __name__=='__main__':main()
