"""Record whole frame/display owners only after real and clean integration pass."""
import hashlib,json,shutil,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HERE=ROOT/'META/m4/frame-pipeline'
BASE_FUNCTIONS=322
BASE_TUS=310
SPECS=[
 ('00471e00','bh_frame_worker_run','bh_frame_worker','frame-worker'),
 ('0045cdf0','bh_present_begin','bh_present_begin','present-begin'),
 ('0045b810','bh_capture_software','bh_capture','capture'),
 ('0045bc40','bh_capture_write','bh_capture','capture'),
 ('0045d750','bh_display_release','bh_display_shutdown','display-shutdown'),
 ('004738a0','bh_display_shutdown','bh_display_shutdown','display-shutdown'),
 ('004713c0','bh_force_feedback_stop','bh_force_feedback','force-feedback-stop'),
 ('00476030','bh_network_shutdown','bh_network_shutdown','network-shutdown'),
 ('00449b40','bh_cd_stop_close','bh_cd_close','cd-close'),
 ('00479140','bh_frame_service_tick','bh_frame_service','frame-service'),
 ('00471df0','bh_frame_callback_set','bh_frame_callback','frame-callback'),
 ('00479120','bh_frame_callback_start','bh_frame_callback','frame-callback')]
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,indent=2)+'\n',encoding='utf-8')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def active(ledger):return [r for r in ledger['records'] if r['implementation'] in ('func','bm') and r['integration']=='linked_harness' and not r.get('historical_variant') and not r.get('validation',{}).get('superseded_by')]
def reference():
    ledger=read(ROOT/'META/function-status.json');exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==ledger['reference_exe_sha256']
    for entry in read(ROOT/'META/reference-manifest.json')['files']:
        if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256'],entry['path']
    pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    base=struct.unpack_from('<I',exe,pe+24+28)[0]
    sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def raw(at,n):
        r=at-base;s=next(s for s in sections if s[1]<=r and r+n<=s[1]+s[2]);return exe[s[3]+r-s[1]:s[3]+r-s[1]+n]
    return ledger,raw

def boundaries(raw):
    owners=[]
    for address,function,module,scenario in SPECS:
        candidates=sorted(HERE.rglob(address+'.disasm.json'))
        if not candidates:candidates=sorted(HERE.rglob(address+'.raw.json'))
        assert candidates,address
        original=candidates[0];rows=read(original);at=int(address,16);body=bytearray()
        for row in rows:
            assert int(row['address'],16)==at,(address,row)
            b=bytes.fromhex(row['bytes']);assert b==raw(at,len(b)),row['address'];body.extend(b);at+=len(b)
        assert rows[-1]['mnemonic']==('JMP' if address=='00476030' else 'RET'),address
        # The original inventory fixes whole-owner lengths independently of exports.
        inventory=(ROOT/'analysis/function-map.tsv').read_text(encoding='utf-8-sig').splitlines()
        record=next(line.split('\t') for line in inventory if line.startswith(address+'\t'))
        assert len(body)==int(record[2]) and len(rows)==int(record[3]),address
        ranges=[dict(start=address,end_exclusive=f'{at:08x}',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest())]
        owners.append(dict(address=address,function=function,source='src/'+module+'.c',role=('game' if address=='00479120' else 'engine_candidate'),scenarios=([scenario,'frame-worker'] if address in ('00471df0','00479120') else [scenario]),original=original.relative_to(ROOT).as_posix(),bytes=len(body),instructions=len(rows),ranges=ranges))
    return owners

def native_dependencies(real,clean):
    manifest_bytes=(ROOT/'META/reference-manifest.json').read_bytes()
    assert real['dependency_manifest_sha256']==hashlib.sha256(manifest_bytes).hexdigest()
    # Git normalizes this tracked JSON to LF in the asset-free checkout; the
    # pinned dependency bytes inside it and every external SDK hash stay exact.
    assert clean['dependency_manifest_sha256']==hashlib.sha256(manifest_bytes.replace(b'\r\n',b'\n')).hexdigest()
    assert real['environment']==clean['environment']
    for kind in ('input','render','directplay'):
        key='native_'+kind+'_dependencies';manifest_path=ROOT/('META/native-'+kind+'-dependencies.json')
        expected=read(manifest_path);dependency=real[key]
        assert dependency==clean[key],key
        assert dependency['manifest_sha256']==sha(manifest_path),key
        assert dependency['files']==expected['files'],key
        for entry in dependency['files']:
            path=Path(dependency['root'])/entry['path']
            assert path.stat().st_size==entry['bytes'] and sha(path)==entry['sha256'],str(path)

def builds():
    real=read(ROOT/'build/m0/result.json');report=read(HERE/'clean/clean-checkout.json');clean=report['result']
    assert real['passed'] and clean['passed'] and real['sources']==clean['sources']
    native_dependencies(real,clean)
    assert not report['original_assets_present'] and not report['compiler_in_checkout']
    for build in (real,clean):
        assert not build['original_code_required'] and not build['test']['failures']
        controls=[r for k,r in build.items() if k.startswith('missing_')]
        assert controls and all('LNK2001' in r for r in controls)
        assert sum(r['checks'] for r in build['scenarios'] if 'checks' in r and r.get('scenario')!='m2-foundations')==build['test']['checks']
    expected_tus=BASE_TUS+2*len({spec[2] for spec in SPECS})
    assert sum(c['label'].startswith('compile-') for c in real['commands'])==sum(c['label'].startswith('compile-') for c in clean['commands'])==expected_tus
    assert sum(k.startswith('missing_') for k in real)==sum(k.startswith('missing_') for k in clean)
    for path,digest in real['sources'].items():assert sha(ROOT/path)==digest,path
    for _,_,module,scenario in SPECS:
        assert 'src/'+module+'.c' in real['sources']
        for build in (real,clean):
            rows=[r for r in build['scenarios'] if r.get('scenario')==scenario]
            assert len(rows)==1 and rows[0]['checks']>0 and not rows[0]['failures'],scenario
    return real,clean

def main():
    ledger,raw=reference();owners=boundaries(raw);real,clean=builds()
    current=active(ledger);assert len({r['original_va'] for r in current})==BASE_FUNCTIONS
    for o in owners:assert not any(r['original_va']==o['address'] for r in current),o['address']
    maps=[ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt']
    for owner in owners:
        for path in maps:assert ('_'+owner['function']).encode() in path.read_bytes(),owner['function']
        source=owner['source'];identifier=owner['address']+'-functional-c-frame-pipeline'
        assert not any(r['id']==identifier for r in ledger['records']),identifier
        fixtures={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in owner['scenarios']}
        clean_fixtures={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in owner['scenarios']}
        record=dict(id=identifier,original_va=owner['address'],function=owner['function'],role=owner['role'],provenance_confidence='Whole PE boundary and dependency role reviewed; historical engine authorship unresolved',source=source,source_sha256=real['sources'][source],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=owner['ranges'],raw_byte_match=False,original_code_executed=False,fixtures=fixtures,clean_fixtures=clean_fixtures,abi='Pinned x86 source binding; WINAPI thread adapter where explicit; PC53 nearest/masked profile',limits=['Explicit source contexts preserve live state; documented invalid-state guards and ABI adapters differ from original.','Source-only offscreen and lifecycle checks; no gameplay or desktop capture.','No complete startup, mission/gameplay, software raster dispatch, CD playback or DirectPlay session claim.','Offscreen Flip reaches DDERR_NOTFLIPPABLE; successful fullscreen Flip remains unvalidated.','Positive force-feedback Effect release is established statically, not exercised with an active hardware effect.','CD positive stop/close checks use a private WAV MCI surrogate, not a physical CD drive.']),evidence=[owner['original'],'META/m4/frame-pipeline/README.md','META/m4/frame-pipeline/validation.json','META/m4/frame-pipeline/integration-real.json','META/m4/frame-pipeline/clean/clean-checkout.json'])
        for old in ledger['records']:
            if old['original_va']==owner['address'] and not old.get('historical_variant'):
                assert old['implementation'] in ('not_started','wip'),old['id'];old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records'].append(record)
    count=len({r['original_va'] for r in active(ledger)});assert count==BASE_FUNCTIONS+len(owners)
    for row in active(ledger):assert real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
    sources=dict(real['sources'])
    for p in ['tools/m0.py','tools/record_frame_pipeline.py','tools/check_frame_pipeline.py','META/native-input-dependencies.json']:sources[p]=sha(ROOT/p)
    snapshots={}
    for name,digest in sources.items():
        target=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');target.parent.mkdir(exist_ok=True)
        if not target.exists():shutil.copyfile(ROOT/name,target)
        snapshots[name]=target.relative_to(ROOT).as_posix()
    focused={s for o in owners for s in o['scenarios']}
    validation=dict(passed=True,new_complete_functions=len(owners),complete_new_functions=len(owners),active_original_functions=count,baseline_functions=BASE_FUNCTIONS,baseline_translation_units=BASE_TUS,translation_units=sum(c['label'].startswith('compile-') for c in real['commands']),missing_module_controls=sum(k.startswith('missing_') for k in real),sources=sources,source_snapshots=snapshots,real_total=real['test'],clean_total=clean['test'],real_focused={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},clean_focused={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},original_code_executed=False,game_launches=0,gameplay_executed=False,native_input_capture_calls=0,native_input_recenter_calls=0,desktop_capture=False,notes=['M3 skipped by user instruction; no game execution.','Whole00479140 includes acknowledged cleanup, not only normal producer path.','Network shutdown includes its final CoUninitialize import tail JMP; import implementation excluded from count.'])
    # Commit evidence/ledger only after all validation and source snapshots succeed.
    write(HERE/'owners.json',owners);write(HERE/'integration-real.json',real);shutil.copyfile(maps[0],HERE/'link-map.txt');write(HERE/'validation.json',validation)
    ledger['updated']='2026-10-25';write(ROOT/'META/function-status.json',ledger)
    print(json.dumps({k:validation[k] for k in ('passed','complete_new_functions','active_original_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()





