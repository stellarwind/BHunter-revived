"""Record whole script wait query pair only after real and clean integration pass."""
import hashlib,json,shutil,struct,copy,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HERE=ROOT/'META/m4/script-wait'
BASE_FUNCTIONS=362
BASE_TUS=354
EXPECTED_CONTROLS=171
SPECS=[('00443270','bh_script_wait_template','bh_script_wait','script-wait'),('00442f40','bh_script_wait_child','bh_script_wait','script-wait')]

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
        if not candidates:candidates=sorted((ROOT/'META/m4/script-frontier').glob(address+'.disasm.json'))
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
        owners.append(dict(address=address,function=function,source='src/'+module+'.c',role='engine_candidate',scenarios=[scenario],original=original.relative_to(ROOT).as_posix(),bytes=len(body),instructions=len(rows),ranges=ranges))
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

def timer_prefix_proof():
    meta=read(ROOT/'META/m4/script-frontier/wait-timer-affected-records.json')
    prior=read(ROOT/'META/m4/rail-wagon/validation.json')
    name='src/bh_script_timer.c';previous=prior['source_snapshots'][name]
    original=(ROOT/previous).read_bytes();current=(ROOT/name).read_bytes()
    assert hashlib.sha256(original).hexdigest()==meta['source_sha256']==prior['sources'][name]
    assert current.startswith(original), 'timer existing source prefix changed'
    suffix=current[len(original):].decode('utf-8')
    assert re.fullmatch(r'\s*BHEntityStep\s+(?:__cdecl\s+)?bh_script_timer_delete_step\s*\(\s*void\s*\)\s*\{\s*return\s+delete_tick\s*;\s*\}\s*',suffix),suffix
    return meta,dict(original_source_snapshot=previous,original_source_sha256=meta['source_sha256'],current_source_sha256=sha(ROOT/name),original_prefix_bytes=len(original),suffix=suffix,original_body_unchanged=True,getter_new_original_count=0)

def refresh_timer_variants(ledger,real,clean):
    meta,proof=timer_prefix_proof();variants=[]
    for item in meta['records']:
        old=next(r for r in ledger['records'] if r['id']==item['id'])
        assert old in active(ledger) and old['source_sha256']==item['source_sha256']
        new=copy.deepcopy(old);new['id']=old['id']+'-wait-delete-exposure'
        assert not any(r['id']==new['id'] for r in ledger['records'])
        old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=new['id']
        new.pop('historical_variant',None);new['source_sha256']=real['sources'][new['source']]
        fixtures={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in ('script-timers','script-wait')}
        clean_fixtures={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in ('script-timers','script-wait')}
        assert all(r['checks']>0 and not r['failures'] for r in list(fixtures.values())+list(clean_fixtures.values()))
        new['validation']=dict(abi=old.get('validation',{}).get('abi'),limits=copy.deepcopy(old.get('validation',{}).get('limits',[]))+['Count-zero append-only getter exposes actual delete callback; current variant revalidated in real/clean script-timers and script-wait scenarios without byte-match transfer.'],raw_byte_match=False,original_code_executed=False,fixtures=fixtures,clean_fixtures=clean_fixtures,source_variant='Append-only count-zero actual delete callback getter',original_body_prefix_proof=proof)
        new['evidence']=['META/m4/script-wait/validation.json','META/m4/script-wait/integration-real.json','META/m4/script-wait/clean/clean-checkout.json']
        ledger['records'].append(new);variants.append(dict(historical_id=old['id'],current_id=new['id'],original_va=old['original_va'],original_function_count_delta=0,prefix_proof=proof))
    assert len(variants)==6
    for build in (real,clean):assert build['sources']['src/bh_script_timer.c']==proof['current_source_sha256']
    for path in (ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt'):assert b'_bh_script_timer_delete_step' in path.read_bytes()
    return variants

def main():
    ledger,raw=reference();owners=boundaries(raw);real,clean=builds()
    current=active(ledger);assert len({r['original_va'] for r in current})==BASE_FUNCTIONS
    for o in owners:assert not any(r['original_va']==o['address'] for r in current),o['address']
    timer_variants=refresh_timer_variants(ledger,real,clean)
    maps=[ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt']
    for owner in owners:
        for path in maps:assert ('_'+owner['function']).encode() in path.read_bytes(),owner['function']
        source=owner['source'];identifier=owner['address']+'-functional-c-script-wait'
        assert not any(r['id']==identifier for r in ledger['records']),identifier
        fixtures={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in owner['scenarios']}
        clean_fixtures={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in owner['scenarios']}
        record=dict(id=identifier,original_va=owner['address'],function=owner['function'],role=owner['role'],provenance_confidence='Ordinary entity/script behavior, not verified CRT; historical engine authorship unresolved',source=source,source_sha256=real['sources'][source],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=owner['ranges'],raw_byte_match=False,original_code_executed=False,fixtures=fixtures,clean_fixtures=clean_fixtures,abi='Pinned x86 cdecl entity query callbacks, real timer context and48-byte copied wait state',limits=['Real timer delete callback is exposed through a count-zero getter; no duplicated owner or guessed callback.','Actual pool, timer context, actors and callback entities must outlive queries; generation guards protect recycling callbacks.','Original unspecified40-byte event payload is documented without claiming deterministic original stack bytes.','No game/original execution or physical input/UI operations; full-suite existing silent PCM Play calls remain separately recorded.']),evidence=[owner['original'],'META/m4/script-wait/README.md','META/m4/script-wait/validation.json','META/m4/script-wait/integration-real.json','META/m4/script-wait/clean/clean-checkout.json'])
        for old in ledger['records']:
            if old['original_va']==owner['address'] and not old.get('historical_variant'):
                assert old['implementation'] in ('not_started','wip'),old['id'];old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records'].append(record)
    count=len({r['original_va'] for r in active(ledger)});assert count==BASE_FUNCTIONS+len(owners)
    for row in active(ledger):assert real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
    sources=dict(real['sources'])
    for p in ['tools/m0.py','tools/record_script_wait.py','tools/check_script_wait.py','META/native-input-dependencies.json']:sources[p]=sha(ROOT/p)
    prior=read(ROOT/'META/m4/rail-wagon/validation.json')
    snapshots={}
    for name,digest in sources.items():
        previous=prior.get('source_snapshots',{}).get(name)
        if previous and prior.get('sources',{}).get(name)==digest and sha(ROOT/previous)==digest:
            snapshots[name]=previous;continue
        target=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');target.parent.mkdir(exist_ok=True)
        if not target.exists():shutil.copyfile(ROOT/name,target)
        snapshots[name]=target.relative_to(ROOT).as_posix()
    focused={s for o in owners for s in o['scenarios']}|{'script-timers'}
    validation=dict(passed=True,new_complete_functions=len(owners),complete_new_functions=len(owners),active_original_functions=count,baseline_functions=BASE_FUNCTIONS,baseline_translation_units=BASE_TUS,timer_variants=timer_variants,translation_units=sum(c['label'].startswith('compile-') for c in real['commands']),missing_module_controls=sum(k.startswith('missing_') for k in real),sources=sources,source_snapshots=snapshots,real_total=real['test'],clean_total=clean['test'],real_focused={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},clean_focused={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},original_code_executed=False,game_launches=0,gameplay_executed=False,native_input_capture_calls=0,native_input_recenter_calls=0,desktop_capture=False,notes=['M3 skipped by user instruction; no game execution.','Whole wait query pair plus current-source variant refresh of six existing timer owners; getter is count-zero and no byte-match evidence transfers.','Identical prior source snapshot text may be reused only after exact name+hash verification; all runtime/scenario evidence is current.'])
    # Commit evidence/ledger only after all validation and source snapshots succeed.
    write(HERE/'owners.json',owners);write(HERE/'integration-real.json',real);shutil.copyfile(maps[0],HERE/'link-map.txt');write(HERE/'validation.json',validation)
    ledger['updated']='2026-10-25';write(ROOT/'META/function-status.json',ledger)
    print(json.dumps({k:validation[k] for k in ('passed','complete_new_functions','active_original_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()





