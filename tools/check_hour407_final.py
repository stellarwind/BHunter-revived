"""Audit the timed hour's17 new owners against their snapshots and final build."""
import hashlib,json
from pathlib import Path
from record_pickup_basic import ROOT,read,sha,active,reference,native_dependencies
HERE=ROOT/'META/m4/hour407'
STAGES=('foundations','sensors','startup','provider-init')
def main():
    ledger,raw=reference();final=read(HERE/'stages/provider-init/validation.json')
    real=read(HERE/'stages/provider-init/integration-real.json');report=read(HERE/'stages/provider-init/clean/clean-checkout.json');clean=report['result']
    assert real['passed'] and clean['passed'] and real['sources']==clean['sources'];native_dependencies(real,clean)
    lobby=read(ROOT/'META/native-lobby-dependencies.json')
    assert real['native_lobby_dependencies']==clean['native_lobby_dependencies']
    dependency=real['native_lobby_dependencies'];assert dependency['manifest_sha256']==sha(ROOT/'META/native-lobby-dependencies.json') and dependency['files']==lobby['files']
    for item in dependency['files']:
        p=Path(dependency['root'])/item['path'];assert p.stat().st_size==item['bytes'] and sha(p)==item['sha256']
    assert not report['original_assets_present'] and not report['compiler_in_checkout'] and not report['exit_code']
    for p,digest in real['sources'].items():assert sha(ROOT/p)==digest,p
    admitted=active(ledger);assert len(admitted)==len({r['original_va'] for r in admitted})==424
    for r in admitted:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    block=[r for r in admitted if '-functional-c-hour407-' in r['id']];assert len(block)==17
    addresses=set();focused=set();previous=407;stage_reports=[]
    for name in STAGES:
        directory=HERE/'stages'/name;config=read(directory/'assignment.json');v=read(directory/'validation.json');owners=read(directory/'owners.json')
        assert v['passed'] and v['baseline_functions']==config['baseline_functions']==previous
        assert len(owners)==v['complete_new_functions']==config['complete_new_functions']
        previous=v['active_original_functions'];assert previous==config['active_original_functions']==v['baseline_functions']+len(owners)
        stage_real=read(directory/'integration-real.json');stage_clean=read(directory/'clean/clean-checkout.json')['result']
        assert stage_real['passed'] and stage_clean['passed'] and stage_real['sources']==stage_clean['sources']
        assert v['real_total']==stage_real['test'] and v['clean_total']==stage_clean['test']
        for p,digest in v['sources'].items():assert sha(ROOT/v['source_snapshots'][p])==digest,p
        for owner in owners:
            address=owner['address'];assert address not in addresses;addresses.add(address)
            r=next(r for r in block if r['id']==address+'-functional-c-hour407-'+name)
            assert r['implementation']=='func' and r['integration']=='linked_harness' and not r['game_admitted']
            assert not r['validation']['raw_byte_match'] and not r['validation']['original_code_executed']
            assert r['validation']['boundary']==owner['ranges']
            assert stage_real['sources'][r['source']]==real['sources'][r['source']]==r['source_sha256']
            for span in owner['ranges']:
                body=raw(int(span['start'],16),span['bytes']);assert len(body)==int(span['end_exclusive'],16)-int(span['start'],16)
                assert hashlib.sha256(body).hexdigest()==span['sha256']
            for file in ('link-map.txt','clean/link-map.txt'):
                assert ('_'+r['function']).encode() in (directory/file).read_bytes(),r['function']
            for scenario in owner['scenarios']:
                focused.add(scenario)
                for build in (real,clean):
                    row=next(s for s in build['scenarios'] if s.get('scenario')==scenario);assert row['checks']>0 and not row['failures']
        stage_reports.append(dict(stage=name,new_functions=len(owners),active_functions=previous,validation_sha256=sha(directory/'validation.json')))
    assert previous==424 and len(addresses)==17 and len(focused)==9
    for build in (real,clean):
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==392
        controls=[v for k,v in build.items() if k.startswith('missing_')];assert len(controls)==189 and all('LNK2001' in v for v in controls)
        assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
    native=next(s for s in real['scenarios'] if s.get('scenario')=='provider-startup')
    assert native['sessions_opened']==0 and native['connections_initialized']==0 and native['connection_initialization_calls']==1
    assert native['address_enumerations']==0 and native['initialization_hresult']==-2005466886 and native['address_hresult']==-2005466816
    result=dict(passed=True,baseline_functions=407,complete_new_functions=17,active_original_functions=424,translation_units=392,missing_module_controls=189,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],focused_scenarios=sorted(focused),stages=stage_reports,original_code_executed=False,game_launches=0,gameplay_executed=False,desktop_capture=False)
    print(json.dumps(result))
if __name__=='__main__':main()
