"""Admit one configured hour443 source stage only after complete real/clean source checks."""
import hashlib,json,shutil,sys,copy
from pathlib import Path
from record_pickup_basic import ROOT,read,write,sha,active,reference,native_dependencies
BASE=ROOT/'META/m4/hour443'
STAGE=sys.argv[1] if len(sys.argv)>1 else 'final'
HERE=BASE/'stages'/STAGE
CONFIG=read(HERE/'assignment.json')
SCENARIOS=CONFIG['families']

def owners():
    result=[]
    for family,scenario in SCENARIOS.items():
        rows=read(BASE/family/'owners.json')
        for row in rows:
            boundary=row.get('boundary') or row.get('ranges') or row.get('validation',{}).get('boundary')
            if isinstance(boundary,dict):boundary=[boundary]
            result.append(dict(address=row.get('original_va',row.get('address')),function=row['function'],source=row['source'],role=row['role'],family=family,scenarios=[scenario],ranges=boundary,provenance_confidence=row.get('provenance_confidence','Ordinary entity callback/numeric behavior with complete reviewed code boundaries; engine authorship unresolved'),abi=row.get('validation',{}).get('abi','x86 cdecl with explicit borrowed source contexts; callback ABI as documented in family README'),limits=[limit for limit in row.get('validation',{}).get('limits',[]) if not limit.startswith('Pending parent')]))
            if row.get('owned_data'):result[-1]['owned_data']=row['owned_data']
    assert len(result)==len({o['address'] for o in result})==CONFIG['complete_new_functions']
    return result

def validate_builds(real,report,items,raw):
    clean=report['result'];assert real['passed'] and clean['passed']
    assert real['sources']==clean['sources']
    native_dependencies(real,clean)
    assert not report['original_assets_present'] and not report['compiler_in_checkout'] and report['exit_code']==0
    for name,digest in real['sources'].items():assert sha(ROOT/name)==digest,name
    for build in (real,clean):
        assert build['test']['failures']==0 and not build['original_code_required']
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==CONFIG['translation_units']
        for command in build['commands']:
            if command['label'].startswith('compile-'):
                source=Path(command['argv'][-1]);key='/'.join(source.parts[-2:])
                assert key in build['sources'],key
        controls=[v for k,v in build.items() if k.startswith('missing_')]
        assert len(controls)==CONFIG['missing_module_controls'] and all('LNK2001' in v for v in controls)
        assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
        for scenario in SCENARIOS.values():
            rows=[s for s in build['scenarios'] if s.get('scenario')==scenario]
            assert len(rows)==1 and rows[0]['checks']>0 and rows[0]['failures']==0
    for o in items:
        if o.get('owned_data'):
            data=read(ROOT/o['owned_data']);body=raw(int(data['start'],16),data['bytes'])
            assert len(body)==int(data['end_exclusive'],16)-int(data['start'],16)
            assert hashlib.sha256(body).hexdigest()==data['sha256'] and body.hex()==data['raw']
            assert data['independent_function_count']==0
        for span in o['ranges']:
            b=raw(int(span['start'],16),span['bytes'])
            assert len(b)==int(span['end_exclusive'],16)-int(span['start'],16)
            assert hashlib.sha256(b).hexdigest()==span['sha256'],o['address']
        for path in (ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt'):
            assert ('_'+o['function']).encode() in path.read_bytes(),o['function']
    return clean


def refresh_variants(ledger,real,clean):
    affected=CONFIG.get('refreshed_sources',[]);prior=read(ROOT/CONFIG['prior_validation']);rows=[]
    for old in list(active(ledger)):
        if old['source'] not in affected:continue
        source=old['source'];snapshot=prior['source_snapshots'][source]
        assert old['implementation']=='func' and old['source_sha256']==prior['sources'][source]==sha(ROOT/snapshot)
        new=copy.deepcopy(old);identifier=old['original_va']+'-functional-c-hour443-'+STAGE+'-context'
        assert not any(r['id']==identifier for r in ledger['records'])
        old_id=old['id'];old['historical_variant']=True;old['historical_source_path']=source;old['source']=snapshot;old['validation']['superseded_by']=identifier
        new['id']=identifier;new['source_sha256']=real['sources'][source];new['validation']['raw_byte_match']=False
        new['validation']['supersedes']=old_id
        if source in CONFIG.get('corrected_sources',{}):
            defect=CONFIG['corrected_sources'][source]
            old['validation']['known_defect']=defect
            old['historical_reason']='Incorrect functional source variant; correction receives independent current linked evidence.'
            new['validation'].pop('known_defect',None)
            new['validation']['corrects_previous_defect']=defect
        scenarios=list(new['validation'].get('fixtures') or CONFIG.get('refresh_existing_scenarios',[]));assert scenarios
        for key,build in (('fixtures',real),('clean_fixtures',clean)):
            new['validation'][key]={s:next(v for v in build['scenarios'] if v.get('scenario')==s) for s in scenarios}
        new['validation']['tu_dispatch_fixtures']={s:next(v for v in real['scenarios'] if v.get('scenario')==s) for s in CONFIG.get('refresh_scenarios',[])}
        new['validation']['original_code_executed']=False
        new['validation']['limits'].append(CONFIG.get('refresh_note','Fresh same-owner source variant; current real/clean checks and frozen historical source. No byte-match transfer or extra original function count.'))
        new['evidence']=['META/m4/hour443/stages/'+STAGE+'/validation.json','META/m4/hour443/stages/'+STAGE+'/refreshed-variants.json']+new['evidence']
        ledger['records'].append(new);rows.append(dict(original_va=new['original_va'],function=new['function'],source=source,previous_id=old_id,previous_snapshot=snapshot,id=identifier))
        for path in (ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt'):assert ('_'+new['function']).encode() in path.read_bytes()
    assert len(rows)==CONFIG.get('refreshed_variants',0)
    for old in list(ledger.get('support_implementations',[])):
        if old.get('source') not in affected or old.get('historical_variant') or old.get('validation',{}).get('superseded_by'):continue
        source=old['source'];snapshot=prior['source_snapshots'][source];assert old['source_sha256']==sha(ROOT/snapshot)
        new=copy.deepcopy(old);identifier=old['id']+'-hour443-'+STAGE
        old['historical_variant']=True;old['historical_source_path']=source;old['source']=snapshot;old.setdefault('validation',{})['superseded_by']=identifier
        new['id']=identifier;new['source_sha256']=real['sources'][source];new.setdefault('validation',{})['supersedes']=old['id'];new['evidence']=['META/m4/hour443/stages/'+STAGE+'/validation.json']+new.get('evidence',[])
        ledger['support_implementations'].append(new)
    return rows

def main():
    ledger,raw=reference();items=owners();real=read(ROOT/'build/m0/result.json');report=read(HERE/'clean/clean-checkout.json')
    clean=validate_builds(real,report,items,raw)
    current=active(ledger);assert len(current)==len({r['original_va'] for r in current})==CONFIG['baseline_functions']
    for o in items:
        assert o['address'] not in {r['original_va'] for r in current}
        identifier=o['address']+'-functional-c-hour443-'+STAGE
        assert not any(r['id']==identifier for r in ledger['records'])
        record=dict(id=identifier,original_va=o['address'],function=o['function'],role=o['role'],provenance_confidence=o['provenance_confidence'],source=o['source'],source_sha256=real['sources'][o['source']],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=o['ranges'],abi=o['abi'],fixtures={s:next(v for v in real['scenarios'] if v.get('scenario')==s) for s in o['scenarios']},clean_fixtures={s:next(v for v in clean['scenarios'] if v.get('scenario')==s) for s in o['scenarios']},raw_byte_match=False,original_code_executed=False,limits=o['limits']+['Full contract, source context adaptations and outstanding ownership/precision requirements in family README.','Linked source harness only; live world/menu/script construction and playable game are incomplete; M3 skipped.','No game/original code, gameplay automation, physical input or UI capture.']),evidence=['META/m4/hour443/'+o['family']+'/README.md','META/m4/hour443/stages/'+STAGE+'/owners.json','META/m4/hour443/stages/'+STAGE+'/validation.json','META/m4/hour443/stages/'+STAGE+'/integration-real.json','META/m4/hour443/stages/'+STAGE+'/clean/clean-checkout.json'])
        if o.get('owned_data'):record['validation']['owned_data']=o['owned_data'];record['evidence'].append(o['owned_data'])
        for old in ledger['records']:
            if old['original_va']==o['address'] and not old.get('historical_variant'):
                assert old['implementation'] in ('not_started','wip');old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records'].append(record)
    refreshed=refresh_variants(ledger,real,clean)
    for support in CONFIG.get('support',[]):
        assert not any(r['id']==support['id'] for r in ledger['support_implementations'])
        source=support['source'];assert source in real['sources']
        ledger['support_implementations'].append(dict(support,source_sha256=real['sources'][source],implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,evidence=['META/m4/hour443/stages/'+STAGE+'/validation.json','META/m4/hour443/'+support['family']+'/README.md']))
    admitted=active(ledger);assert len(admitted)==len({r['original_va'] for r in admitted})==CONFIG['active_original_functions']
    for r in admitted:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    sources=dict(real['sources'])
    for path in ['tools/m0.py','tools/record_hour443.py','tools/check_hour443.py','META/native-input-dependencies.json','META/native-render-dependencies.json','META/native-directplay-dependencies.json','META/native-lobby-dependencies.json']:
        sources[path]=sha(ROOT/path)
    prior=read(ROOT/CONFIG['prior_validation']);snapshots={}
    for name,digest in sources.items():
        previous=prior.get('source_snapshots',{}).get(name)
        if previous and prior.get('sources',{}).get(name)==digest and sha(ROOT/previous)==digest:snapshots[name]=previous;continue
        path=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');path.parent.mkdir(exist_ok=True)
        shutil.copyfile(ROOT/name,path);snapshots[name]=path.relative_to(ROOT).as_posix()
    validation=dict(passed=True,baseline_functions=CONFIG['baseline_functions'],complete_new_functions=CONFIG['complete_new_functions'],active_original_functions=CONFIG['active_original_functions'],translation_units=CONFIG['translation_units'],missing_module_controls=CONFIG['missing_module_controls'],real_total=real['test'],clean_total=clean['test'],sources=sources,source_snapshots=snapshots,real_focused={s:next(v for v in real['scenarios'] if v.get('scenario')==s) for s in SCENARIOS.values()},clean_focused={s:next(v for v in clean['scenarios'] if v.get('scenario')==s) for s in SCENARIOS.values()},original_code_executed=False,game_launches=0,gameplay_executed=False,desktop_capture=False,native_input_capture_calls=0,native_input_recenter_calls=0,notes=['Whole assigned owners only; source context bindings/guards are count-zero adapters.','No byte-match evidence transferred. Existing full-suite sound-tick fixture retains12 silent-PCM native Play calls; Play calls are explicitly recorded per family, including actual native primary startup; no samples uploaded by that fixture.','M3 skipped; M4 incomplete; PC24, CD audio and DirectPlay gameplay requirements remain open.'])
    write(HERE/'refreshed-variants.json',refreshed);write(HERE/'owners.json',items);write(HERE/'integration-real.json',real);shutil.copyfile(ROOT/'build/m0/bhunter-integration.map',HERE/'link-map.txt');write(HERE/'validation.json',validation)
    ledger['updated']='2026-10-26';write(ROOT/'META/function-status.json',ledger)
    print(json.dumps({k:validation[k] for k in ('passed','complete_new_functions','active_original_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()
