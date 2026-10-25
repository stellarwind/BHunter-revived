"""Admit the35 target400 owners only after complete real/clean source checks."""
import hashlib,json,shutil
from pathlib import Path
from record_pickup_basic import ROOT,read,write,sha,active,reference,native_dependencies
HERE=ROOT/'META/m4/target400'
SCENARIOS={'a':'menu-settings','b':'script-lifetime','c':'serial-options','parent':'callback-helpers'}

def owners():
    result=[]
    for family,scenario in SCENARIOS.items():
        rows=read(HERE/family/('records-candidate.json' if family=='c' else 'owners.json'))
        for row in rows:
            boundary=row.get('boundary') or row.get('ranges') or row.get('validation',{}).get('boundary')
            if isinstance(boundary,dict):boundary=[boundary]
            result.append(dict(address=row.get('original_va',row.get('address')),function=row['function'],source=row['source'],role=row['role'],family=family,scenarios=[scenario],ranges=boundary,provenance_confidence=row.get('provenance_confidence','Ordinary entity callback/numeric behavior with complete reviewed code boundaries; engine authorship unresolved'),abi=row.get('validation',{}).get('abi','x86 cdecl with explicit borrowed source contexts; callback ABI as documented in family README'),limits=row.get('validation',{}).get('limits',[])))
    assert len(result)==len({o['address'] for o in result})==35
    return result

def validate_builds(real,report,items,raw):
    clean=report['result'];assert real['passed'] and clean['passed']
    assert real['sources']==clean['sources']
    native_dependencies(real,clean)
    assert not report['original_assets_present'] and not report['compiler_in_checkout'] and report['exit_code']==0
    for name,digest in real['sources'].items():assert sha(ROOT/name)==digest,name
    for build in (real,clean):
        assert build['test']['failures']==0 and not build['original_code_required']
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==366
        for command in build['commands']:
            if command['label'].startswith('compile-'):
                source=Path(command['argv'][-1]);key='/'.join(source.parts[-2:])
                assert key in build['sources'],key
        controls=[v for k,v in build.items() if k.startswith('missing_')]
        assert len(controls)==176 and all('LNK2001' in v for v in controls)
        assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
        for scenario in SCENARIOS.values():
            rows=[s for s in build['scenarios'] if s.get('scenario')==scenario]
            assert len(rows)==1 and rows[0]['checks']>0 and rows[0]['failures']==0
    for o in items:
        for span in o['ranges']:
            b=raw(int(span['start'],16),span['bytes'])
            assert len(b)==int(span['end_exclusive'],16)-int(span['start'],16)
            assert hashlib.sha256(b).hexdigest()==span['sha256'],o['address']
        for path in (ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt'):
            assert ('_'+o['function']).encode() in path.read_bytes(),o['function']
    return clean

def main():
    ledger,raw=reference();items=owners();real=read(ROOT/'build/m0/result.json');report=read(HERE/'clean/clean-checkout.json')
    clean=validate_builds(real,report,items,raw)
    current=active(ledger);assert len(current)==len({r['original_va'] for r in current})==365
    for o in items:
        assert o['address'] not in {r['original_va'] for r in current}
        identifier=o['address']+'-functional-c-target400'
        assert not any(r['id']==identifier for r in ledger['records'])
        record=dict(id=identifier,original_va=o['address'],function=o['function'],role=o['role'],provenance_confidence=o['provenance_confidence'],source=o['source'],source_sha256=real['sources'][o['source']],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=o['ranges'],abi=o['abi'],fixtures={s:next(v for v in real['scenarios'] if v.get('scenario')==s) for s in o['scenarios']},clean_fixtures={s:next(v for v in clean['scenarios'] if v.get('scenario')==s) for s in o['scenarios']},raw_byte_match=False,original_code_executed=False,limits=o['limits']+['Full contract, source context adaptations and outstanding ownership/precision requirements in family README.','Linked source harness only; live world/menu/script construction and playable game are incomplete; M3 skipped.','No game/original code, gameplay automation, physical input or UI capture.']),evidence=['META/m4/target400/'+o['family']+'/README.md','META/m4/target400/owners.json','META/m4/target400/validation.json','META/m4/target400/integration-real.json','META/m4/target400/clean/clean-checkout.json'])
        for old in ledger['records']:
            if old['original_va']==o['address'] and not old.get('historical_variant'):
                assert old['implementation'] in ('not_started','wip');old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records'].append(record)
    admitted=active(ledger);assert len(admitted)==len({r['original_va'] for r in admitted})==400
    for r in admitted:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    sources=dict(real['sources'])
    for path in ['tools/m0.py','tools/record_target400.py','tools/check_target400.py','META/native-input-dependencies.json','META/native-render-dependencies.json','META/native-directplay-dependencies.json']:
        sources[path]=sha(ROOT/path)
    prior=read(ROOT/'META/m4/pickup-basic/validation.json');snapshots={}
    for name,digest in sources.items():
        previous=prior.get('source_snapshots',{}).get(name)
        if previous and prior.get('sources',{}).get(name)==digest and sha(ROOT/previous)==digest:snapshots[name]=previous;continue
        path=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');path.parent.mkdir(exist_ok=True)
        shutil.copyfile(ROOT/name,path);snapshots[name]=path.relative_to(ROOT).as_posix()
    validation=dict(passed=True,baseline_functions=365,complete_new_functions=35,active_original_functions=400,translation_units=366,missing_module_controls=176,real_total=real['test'],clean_total=clean['test'],sources=sources,source_snapshots=snapshots,real_focused={s:next(v for v in real['scenarios'] if v.get('scenario')==s) for s in SCENARIOS.values()},clean_focused={s:next(v for v in clean['scenarios'] if v.get('scenario')==s) for s in SCENARIOS.values()},original_code_executed=False,game_launches=0,gameplay_executed=False,desktop_capture=False,native_input_capture_calls=0,native_input_recenter_calls=0,notes=['Exactly35 whole owners; source context bindings/guards are count-zero adapters.','No byte-match evidence transferred. Existing full-suite sound-tick fixture retains12 silent-PCM native Play calls; new families perform no Play.','M3 skipped; M4 incomplete; PC24, CD audio and DirectPlay gameplay requirements remain open.'])
    write(HERE/'owners.json',items);write(HERE/'integration-real.json',real);shutil.copyfile(ROOT/'build/m0/bhunter-integration.map',HERE/'link-map.txt');write(HERE/'validation.json',validation)
    ledger['updated']='2026-10-25';write(ROOT/'META/function-status.json',ledger)
    print(json.dumps({k:validation[k] for k in ('passed','complete_new_functions','active_original_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()
