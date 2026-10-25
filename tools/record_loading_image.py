"""Record whole loading image owners only after real and clean integration pass."""
import hashlib,json,shutil,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HERE=ROOT/'META/m4/loading-image'
BASE_FUNCTIONS=339
BASE_TUS=336
EXPECTED_CONTROLS=162
SPECS=[('00449e50','bh_loading_image_load','bh_loading_image','loading-image'),('0044a160','bh_loading_images_init','bh_loading_image','loading-image')]

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

def close_boundary(raw):
    path=ROOT/'META/m4/sound-loader/next-review/0044f1b0.disasm.json';rows=read(path);at=0x0044f1b0;body=bytearray()
    for row in rows:
        assert int(row['address'],16)==at
        data=bytes.fromhex(row['bytes']);assert data==raw(at,len(data));body.extend(data);at+=len(data)
    assert len(rows)==5 and len(body)==15 and rows[-1]['mnemonic']=='RET'
    return dict(start='0044f1b0',end_exclusive=f'{at:08x}',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest(),original=path.relative_to(ROOT).as_posix())

def boundaries(raw):
    layout=read(ROOT/'META/m4/sound-loader/next-review/asset-layout.json')
    assert layout['passed'] and layout['arena_bytes']==0x12c000
    with (ROOT/'GAME_DATA/Bhunter.bin').open('rb') as archive:
        for image in layout['images']:
            archive.seek(image['archive_begin']);header=archive.read(128)
            assert hashlib.sha256(header).hexdigest()==image['header_sha256']
            assert struct.unpack_from('<H',header,8)[0]+1==image['width']==640
            assert struct.unpack_from('<H',header,10)[0]+1==image['height']
            assert image['arena_end']-image['arena_start']==image['width']*image['height']
    for address,name in ((0x491318,'textures\\midas.pcx'),(0x491304,'textures\\iel.pcx'),(0x4912f0,'textures\\bad.pcx')):
        data=name.encode('ascii')+b'\0';assert raw(address,len(data))==data
    close_boundary(raw)
    owners=[]
    for address,function,module,scenario in SPECS:
        candidates=sorted(HERE.rglob(address+'.disasm.json'))
        if not candidates:candidates=sorted(HERE.rglob(address+'.raw.json'))
        if not candidates:candidates=sorted((ROOT/'META/m4/sound-loader/next-review').glob(address+'.disasm.json'))
        assert candidates,address
        original=candidates[0];rows=read(original);at=int(address,16);body=bytearray()
        for row in rows:
            assert int(row['address'],16)==at,(address,row)
            b=bytes.fromhex(row['bytes']);assert b==raw(at,len(b)),row['address'];body.extend(b);at+=len(b)
        assert rows[-1]['mnemonic']=='RET',address
        # The original inventory fixes whole-owner lengths independently of exports.
        inventory=(ROOT/'analysis/function-map.tsv').read_text(encoding='utf-8-sig').splitlines()
        record=next(line.split('\t') for line in inventory if line.startswith(address+'\t'))
        assert len(body)==int(record[2]) and len(rows)==int(record[3]),address
        ranges=[dict(start=address,end_exclusive=f'{at:08x}',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest())]
        owners.append(dict(address=address,function=function,source='src/'+module+'.c',role=('game' if address=='0044a160' else 'engine_candidate'),scenarios=[scenario],original=original.relative_to(ROOT).as_posix(),bytes=len(body),instructions=len(rows),ranges=ranges))
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
    assert sum(k.startswith('missing_') for k in real)==sum(k.startswith('missing_') for k in clean)==EXPECTED_CONTROLS
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
        source=owner['source'];identifier=owner['address']+'-functional-c-loading-image'
        assert not any(r['id']==identifier for r in ledger['records']),identifier
        fixtures={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in owner['scenarios']}
        clean_fixtures={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in owner['scenarios']}
        record=dict(id=identifier,original_va=owner['address'],function=owner['function'],role=owner['role'],provenance_confidence='Whole PE boundary and dependency role reviewed; historical engine authorship unresolved',source=source,source_sha256=real['sources'][source],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=owner['ranges'],raw_byte_match=False,original_code_executed=False,fixtures=fixtures,clean_fixtures=clean_fixtures,abi='Pinned x86 source binding; WINAPI thread adapter where explicit; PC53 nearest/masked profile',limits=['Explicit source contexts preserve live state; documented invalid-state guards and ABI adapters differ from original.','Source-only offscreen and lifecycle checks; no gameplay or desktop capture.','No complete startup, mission/gameplay, software raster dispatch, CD playback or DirectPlay session claim.','Memory-only decoded image and indexed/native LUT checks; no window, GPU rendering, input, audio or original-code execution.','Original unsafe allocation/span failures have explicit source guards; no pixel screenshot or full loading renderer acceptance.']),evidence=[owner['original'],'META/m4/loading-image/README.md','META/m4/loading-image/validation.json','META/m4/loading-image/integration-real.json','META/m4/loading-image/clean/clean-checkout.json'])
        for old in ledger['records']:
            if old['original_va']==owner['address'] and not old.get('historical_variant'):
                assert old['implementation'] in ('not_started','wip'),old['id'];old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records'].append(record)
    adapter_id='0044f1b0-owning-close-loading-adapter'
    assert not any(r['id']==adapter_id for r in ledger['records'])
    adapter_scenarios=['loading-image','pcx-buffered-reader']
    for path in maps:assert b'_bh_pcx_reader_close' in path.read_bytes()
    adapter_fixtures={prefix:{name:next(row for row in build['scenarios'] if row.get('scenario')==name) for name in adapter_scenarios} for prefix,build in [('real',real),('clean',clean)]}
    for fixtures in adapter_fixtures.values():assert all(row['checks']>0 and not row['failures'] for row in fixtures.values())
    ledger['records'].append(dict(id=adapter_id,original_va='0044f1b0',function='bh_pcx_reader_close',role='engine_candidate',source='src/bh_pcx_reader.c',source_sha256=real['sources']['src/bh_pcx_reader.c'],implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,original_function_count=0,validation=dict(boundary=close_boundary(raw),fixtures=adapter_fixtures['real'],clean_fixtures=adapter_fixtures['clean'],raw_byte_match=False,original_code_executed=False,scope='Actual owning VFILE closure reused unchanged; source clears reader state while original leaves palette/map globals retained. Loading owner retains palette externally before close. Full original global-state equivalence is unclaimed; not counted as complete original owner.'),evidence=['META/m4/loading-image/validation.json','META/m4/loading-image/README.md','META/m4/sound-loader/next-review/0044f1b0.disasm.json']))
    count=len({r['original_va'] for r in active(ledger)});assert count==BASE_FUNCTIONS+len(owners)
    for row in active(ledger):assert real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
    sources=dict(real['sources'])
    for p in ['tools/m0.py','tools/record_loading_image.py','tools/check_loading_image.py','META/native-input-dependencies.json','META/m4/sound-loader/next-review/asset-layout.json']:sources[p]=sha(ROOT/p)
    snapshots={}
    for name,digest in sources.items():
        target=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');target.parent.mkdir(exist_ok=True)
        if not target.exists():shutil.copyfile(ROOT/name,target)
        snapshots[name]=target.relative_to(ROOT).as_posix()
    focused={s for o in owners for s in o['scenarios']}
    validation=dict(passed=True,new_complete_functions=len(owners),complete_new_functions=len(owners),active_original_functions=count,baseline_functions=BASE_FUNCTIONS,baseline_translation_units=BASE_TUS,translation_units=sum(c['label'].startswith('compile-') for c in real['commands']),missing_module_controls=sum(k.startswith('missing_') for k in real),sources=sources,source_snapshots=snapshots,real_total=real['test'],clean_total=clean['test'],real_focused={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},clean_focused={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},original_code_executed=False,game_launches=0,gameplay_executed=False,native_input_capture_calls=0,native_input_recenter_calls=0,desktop_capture=False,notes=['M3 skipped by user instruction; no game execution.','Whole image loader covers full640 and half320 memory output plus indexed/native and quarter-bright LUT branches.','Three-image initializer retains original fixed arena offsets, palette snapshots, wrapped deadline/redraw_count2/stage0 ordering.'])
    # Commit evidence/ledger only after all validation and source snapshots succeed.
    write(HERE/'owners.json',owners);write(HERE/'integration-real.json',real);shutil.copyfile(maps[0],HERE/'link-map.txt');write(HERE/'validation.json',validation)
    ledger['updated']='2026-10-25';write(ROOT/'META/function-status.json',ledger)
    print(json.dumps({k:validation[k] for k in ('passed','complete_new_functions','active_original_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()





