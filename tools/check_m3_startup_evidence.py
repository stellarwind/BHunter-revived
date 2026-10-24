"""Read-only audit of original startup boundaries and the compatibility control."""
from pathlib import Path
import hashlib
import json

ROOT=Path(__file__).resolve().parents[1]
def read(p):return json.loads((ROOT/p).read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256((ROOT/p).read_bytes()).hexdigest()
def events(r):
    if 'events' in r:return r['events']
    output=next(v['output'] for v in r['commands'] if v['label']=='startup')
    return [json.loads(v) for v in output.splitlines() if v.startswith('{')]
def event(r,name):return next(v for v in events(r) if v['event']==name)

def main():
    names=['c57437263011','c4fa4eb8b111','dedff5a4e727','62d00616a929','dae86876751f','e1ee374903ef']
    records={n:read('META/m3/startup/'+n+'.json') for n in names}
    current_refs={v['path']:v['sha256'] for v in read('META/reference-manifest.json')['files'] if v['root']=='repo'}
    for p,h in current_refs.items():assert sha(p)==h,p
    for n,r in records.items():
        assert r['registry_before']==r['registry_after']
        assert r['protected_windows_files_before']==r['protected_windows_files_after']
        assert r['private_key_absent_after'] and not r['original_file_patches'] and not r['m3_complete']
        assert r['original_hashes']==current_refs
        for p,h in r['source_hashes'].items():
            if 'source_snapshots' in r:captured=r['source_snapshots'][p]
            elif n=='c57437263011':captured='META/m3/startup/first-boundary-'+Path(p).name+'.txt'
            else:captured='META/m3/startup/sources/'+h+'-'+Path(p).name+'.txt'
            assert sha(captured)==h,(n,p)
        assert event(r,'source-check-return')['eax']==1
        assert event(r,'private-key-cleanup')['result']==0
        assert event(r,'devices-enumerated')['devices']==['Primary Display Driver']
        if r['passed']:
            assert event(r,'breakpoints-restored')['verified'] and event(r,'probe-complete')['passed']
        for f in r['profile_files']:
            p=r['profile_path']+'/'+f['name'];assert sha(p)==f['sha256'] and (ROOT/p).stat().st_size==f['bytes']
    first=records['c57437263011'];native=records['62d00616a929'];compat=records['e1ee374903ef']
    # These runs validate their archived source variants. Later bootstrap probe
    # edits do not inherit the earlier native/compatibility comparison evidence.
    assert first['passed'] and event(first,'message-box-boundary')['caption']=='3D Acceleration' and not event(first,'message-box-boundary')['dialog_executed']
    assert native['passed'] and event(native,'message-box-boundary')['caption']=='Fatal Error!'
    native_modes=[v for v in events(native) if v['event']=='render-mode-call']
    assert [v['arguments'] for v in native_modes]==[[640,480,16,1],[640,480,8,1],[320,240,16,1],[320,240,8,1],[320,200,16,1],[320,200,8,1]]
    assert all(v['eax']==0 for v in events(native) if v['event']=='render-mode-return')
    assert event(native,'directdraw-set-display-mode-result')['hresult']==0x80004001
    assert not any(v['event']=='bootstrap-entry' for v in events(native))
    assert compat['passed'] and event(compat,'directdraw-set-display-mode-result')['hresult']==0 and event(compat,'render-mode-return')['eax']==1
    assert not event(compat,'bootstrap-entry')['game_bootstrap_executed']
    assert event(native,'config-bootstrap-return')['selection_words']==event(compat,'config-bootstrap-return')['selection_words']==[1,0,1]
    assert native['show_parameter']==compat['show_parameter']==1
    assert native['source_hashes']['tools/m3_startup_probe.c']==compat['source_hashes']['tools/m3_startup_probe.c']
    dependency=read('META/m3/compat/ddrawcompat-v0.7.2.json')
    assert dependency['archive_sha256']==dependency['asset_digest_from_github'].split(':')[1]
    assert sha(dependency['staged_dll'])==dependency['dll_sha256'] and dependency['prerelease']
    log=compat['compatibility_log'];assert sha(log['path'])==log['sha256']
    assert 'DDrawCompat version: v0.7.2' in (ROOT/log['path']).read_text(errors='replace')
    for r in [native,compat]:
        for e in events(r):
            if 'x87_control' in e:assert e['x87_control']==0x27f
    print(json.dumps(dict(passed=True,scope='CLI/native-debugger startup evidence only; not full M3 or gameplay validation',original_files_unchanged=True,first_configuration_boundary=True,native_render_modes_failed=6,native_first_display_hresult='80004001',compatibility_version='v0.7.2 prerelease',compatibility_renderer_reached_bootstrap=True,game_bootstrap_executed=False,m3_complete=False)))

if __name__=='__main__':main()
