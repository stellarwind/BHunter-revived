"""Audit the timed hour's19 new owners against their snapshots and final build."""
import hashlib,json
from pathlib import Path
from record_pickup_basic import ROOT,read,sha,active,reference,native_dependencies
HERE=ROOT/'META/m4/hour424'
STAGES=('rendering','lifetimes')
def main():
    ledger,raw=reference();final=read(HERE/'stages/lifetimes/validation.json')
    real=read(HERE/'stages/lifetimes/integration-real.json');report=read(HERE/'stages/lifetimes/clean/clean-checkout.json');clean=report['result']
    assert real['passed'] and clean['passed'] and real['sources']==clean['sources'];native_dependencies(real,clean)
    lobby=read(ROOT/'META/native-lobby-dependencies.json')
    assert real['native_lobby_dependencies']==clean['native_lobby_dependencies']
    dependency=real['native_lobby_dependencies'];assert dependency['manifest_sha256']==sha(ROOT/'META/native-lobby-dependencies.json') and dependency['files']==lobby['files']
    for item in dependency['files']:
        p=Path(dependency['root'])/item['path'];assert p.stat().st_size==item['bytes'] and sha(p)==item['sha256']
    assert not report['original_assets_present'] and not report['compiler_in_checkout'] and not report['exit_code']
    for p,digest in real['sources'].items():assert sha(ROOT/p)==digest,p
    admitted=active(ledger);assert len(admitted)==len({r['original_va'] for r in admitted})==443
    for r in admitted:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    block=[r for r in admitted if '-functional-c-hour424-' in r['id'] and not r['id'].endswith('-context')];assert len(block)==19
    addresses=set();focused=set();previous=424;stage_reports=[]
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
            r=next(r for r in block if r['id']==address+'-functional-c-hour424-'+name)
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
    assert previous==443 and len(addresses)==19 and len(focused)==8
    for build in (real,clean):
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==409
        controls=[v for k,v in build.items() if k.startswith('missing_')];assert len(controls)==198 and all('LNK2001' in v for v in controls)
        assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
    refreshed=read(HERE/'stages/rendering/refreshed-variants.json');assert len(refreshed)==9
    for item in refreshed:
        new=next(r for r in admitted if r['id']==item['id']);old=next(r for r in ledger['records'] if r['id']==item['previous_id'])
        assert old['historical_variant'] and old['validation']['superseded_by']==new['id']
        assert old['source']==item['previous_snapshot'] and old['source_sha256']==sha(ROOT/old['source'])
        assert new['validation']['supersedes']==old['id'] and not new['validation']['raw_byte_match']
        assert new['source_sha256']==real['sources'][new['source']]
    for support in ledger['support_implementations']:
        if support.get('historical_variant') or (isinstance(support.get('validation'),dict) and support['validation'].get('superseded_by')):continue
        if support.get('source') in real['sources'] and support.get('source_sha256'):
            assert support['source_sha256']==real['sources'][support['source']]
    root=read(HERE/'validation.json')
    assert root['passed'] and root['baseline_functions']==424 and root['complete_new_functions']==19 and root['active_original_functions']==443
    assert root['real_total']==real['test'] and root['clean_total']==clean['test']
    assert root['translation_units']==409 and root['missing_module_controls']==198
    assert read(HERE/'owners.json')==sum((read(HERE/'stages'/name/'owners.json') for name in STAGES),[])
    for p,digest in root['sources'].items():assert sha(ROOT/p)==sha(ROOT/root['source_snapshots'][p])==digest,p
    for p,digest in final['sources'].items():assert root['sources'][p]==digest and root['source_snapshots'][p]==final['source_snapshots'][p]
    for label in ('real_focused','clean_focused'):
        expected={}
        for name in STAGES:expected.update(read(HERE/'stages'/name/'validation.json')[label])
        assert root[label]==expected
    for row in block:
        assert row['evidence'][0]=='META/m4/hour424/validation.json'
    assert root['refreshed_original_variants']==9 and not root['original_code_executed'] and not root['game_launches'] and not root['gameplay_executed']
    result=dict(passed=True,baseline_functions=424,complete_new_functions=19,active_original_functions=443,translation_units=409,missing_module_controls=198,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],focused_scenarios=sorted(focused),stages=stage_reports,original_code_executed=False,game_launches=0,gameplay_executed=False,desktop_capture=False)
    print(json.dumps(result))
if __name__=='__main__':main()
