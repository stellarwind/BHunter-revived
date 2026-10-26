"""Read-only audit of the hour443 source stages and final linked variants."""
import hashlib,json,struct
from pathlib import Path
from record_pickup_basic import ROOT,read,sha,active,reference,native_dependencies
HERE=ROOT/'META/m4/hour443'

def main():
    session=read(HERE/'session.json');names=[s['stage'] for s in session['admissions']]
    assert names and len(names)==len(set(names))
    final_dir=HERE/'stages'/names[-1];config=read(final_dir/'assignment.json');final=read(final_dir/'validation.json')
    real=read(final_dir/'integration-real.json');report=read(final_dir/'clean/clean-checkout.json');clean=report['result']
    ledger,raw=reference();rows=active(ledger)
    assert real['passed'] and clean['passed'] and real['sources']==clean['sources'];native_dependencies(real,clean)
    assert not report['original_assets_present'] and not report['compiler_in_checkout'] and report['exit_code']==0
    lobby=read(ROOT/'META/native-lobby-dependencies.json');dep=real['native_lobby_dependencies']
    assert dep==clean['native_lobby_dependencies'] and dep['manifest_sha256']==sha(ROOT/'META/native-lobby-dependencies.json') and dep['files']==lobby['files']
    for item in dep['files']:
        p=Path(dep['root'])/item['path'];assert p.stat().st_size==item['bytes'] and sha(p)==item['sha256']
    for p,digest in real['sources'].items():assert sha(ROOT/p)==digest,p
    current=config['active_original_functions'];assert len(rows)==len({r['original_va'] for r in rows})==current
    for r in rows:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    expected_new=current-443
    new_addresses={o['address'] for n in names for o in read(HERE/'stages'/n/'owners.json')}
    block=[r for r in rows if r['original_va'] in new_addresses];assert len(block)==expected_new
    addresses=set();focused=set();previous=443;stage_reports=[];all_owners=[];refreshed=[]
    for name in names:
        directory=HERE/'stages'/name;c=read(directory/'assignment.json');v=read(directory/'validation.json');owners=read(directory/'owners.json');all_owners+=owners
        assert v['passed'] and v['baseline_functions']==c['baseline_functions']==previous
        assert len(owners)==v['complete_new_functions']==c['complete_new_functions']
        previous=v['active_original_functions'];assert previous==c['active_original_functions']==v['baseline_functions']+len(owners)
        sr=read(directory/'integration-real.json');sc=read(directory/'clean/clean-checkout.json')['result']
        assert sr['passed'] and sc['passed'] and sr['sources']==sc['sources']
        assert v['real_total']==sr['test'] and v['clean_total']==sc['test']
        for p,digest in v['sources'].items():assert sha(ROOT/v['source_snapshots'][p])==digest,p
        focused.update(c['families'].values())
        for owner in owners:
            address=owner['address'];assert address not in addresses;addresses.add(address)
            r=next(r for r in ledger['records'] if r['id']==address+'-functional-c-hour443-'+name)
            assert r['implementation']=='func' and r['integration']=='linked_harness' and not r['game_admitted']
            assert not r['validation']['raw_byte_match'] and not r['validation']['original_code_executed']
            assert r['validation']['boundary']==owner['ranges']
            source=owner['source']
            assert sr['sources'][source]==r['source_sha256']==sha(ROOT/r['source'])
            latest=next(t for t in rows if t['original_va']==address)
            assert latest['source']==source and latest['source_sha256']==real['sources'][source]
            if r.get('historical_variant'):
                assert r['validation']['superseded_by']==latest['id'] and r['historical_source_path']==source
                assert r['validation'].get('known_defect')==latest['validation'].get('corrects_previous_defect')
            if owner.get('owned_data'):
                data=read(ROOT/owner['owned_data']);data_bytes=raw(int(data['start'],16),data['bytes'])
                assert hashlib.sha256(data_bytes).hexdigest()==data['sha256'] and data_bytes.hex()==data['raw'] and data['independent_function_count']==0
                assert r['validation']['owned_data']==owner['owned_data']
            for span in owner['ranges']:
                body=raw(int(span['start'],16),span['bytes']);assert len(body)==int(span['end_exclusive'],16)-int(span['start'],16)
                assert hashlib.sha256(body).hexdigest()==span['sha256']
            for mapname in ('link-map.txt','clean/link-map.txt'):
                assert ('_'+r['function']).encode() in (directory/mapname).read_bytes(),r['function']
            for key,build in (('fixtures',sr),('clean_fixtures',sc)):
                assert r['validation'][key]=={s:next(t for t in build['scenarios'] if t.get('scenario')==s) for s in owner['scenarios']}
        rr=read(directory/'refreshed-variants.json');assert len(rr)==c.get('refreshed_variants',0);refreshed+=rr
        stage_reports.append(dict(stage=name,new_functions=len(owners),active_functions=previous,validation_sha256=sha(directory/'validation.json')))
    assert previous==current and len(addresses)==expected_new
    for build in (real,clean):
        assert build['test']['failures']==0 and not build['original_code_required']
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==config['translation_units']
        for command in build['commands']:
            if command['label'].startswith('compile-'):
                source=Path(command['argv'][-1]);assert '/'.join(source.parts[-2:]) in build['sources']
        controls=[v for k,v in build.items() if k.startswith('missing_')]
        assert len(controls)==config['missing_module_controls'] and all('LNK2001' in v for v in controls)
        assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
        for scenario in focused:
            hits=[s for s in build['scenarios'] if s.get('scenario')==scenario];assert len(hits)==1 and hits[0]['checks']>0 and hits[0]['failures']==0
            for key in ('play_calls','native_send_calls','physical_acquires','effect_starts','game_launches','gameplay_executed','desktop_capture'):
                assert not hits[0].get(key,0),(scenario,key)
    for item in refreshed:
        new=next(r for r in rows if r['id']==item['id']);old=next(r for r in ledger['records'] if r['id']==item['previous_id'])
        assert old['historical_variant'] and old['validation']['superseded_by']==new['id'] and old['original_va']==new['original_va']
        assert old['source']==item['previous_snapshot'] and old['source_sha256']==sha(ROOT/old['source'])
        assert old['historical_source_path']==new['source'] and new['validation']['supersedes']==old['id'] and not new['validation']['raw_byte_match']
        assert new['source_sha256']==real['sources'][new['source']]
    support_refreshes=0
    for support in ledger['support_implementations']:
        v=support.get('validation',{});v=v if isinstance(v,dict) else {}
        if support.get('historical_variant') or v.get('superseded_by'):continue
        if support.get('source') in real['sources'] and support.get('source_sha256'):assert support['source_sha256']==real['sources'][support['source']]
        if '-hour443-' in support['id'] and v.get('supersedes'):
            old=next(r for r in ledger['support_implementations'] if r['id']==v['supersedes']);assert old['historical_variant'] and old['validation']['superseded_by']==support['id']
            assert old['source_sha256']==sha(ROOT/old['source']);support_refreshes+=1
    assert len(refreshed)==sum(read(HERE/'stages'/n/'assignment.json').get('refreshed_variants',0) for n in names) and support_refreshes==3
    assert raw(0x4892c8,8)==struct.pack('<d',-1.0/32768.0)
    corrected=[r for r in rows if r['validation'].get('corrects_previous_defect')]
    assert len(corrected)==4 and {r['source'] for r in corrected}=={'src/bh_script_explosion.c'}
    for r in corrected:
        old=next(t for t in ledger['records'] if t['id']==r['validation']['supersedes'])
        assert old['validation']['known_defect']==r['validation']['corrects_previous_defect'] and old['source_sha256']!=r['source_sha256']
        assert not r['validation'].get('known_defect')
        for label in ('fixtures','clean_fixtures'):
            assert r['validation'][label]['script-explosion']['checks']==297 and not r['validation'][label]['script-explosion']['failures']
    root=read(HERE/'validation.json')
    assert root['passed'] and root['baseline_functions']==443 and root['complete_new_functions']==expected_new and root['active_original_functions']==current
    assert root['real_total']==real['test'] and root['clean_total']==clean['test']
    assert root['translation_units']==config['translation_units'] and root['missing_module_controls']==config['missing_module_controls']
    assert read(HERE/'owners.json')==all_owners
    for p,digest in root['sources'].items():assert sha(ROOT/p)==sha(ROOT/root['source_snapshots'][p])==digest,p
    for p,digest in final['sources'].items():assert root['sources'][p]==digest and root['source_snapshots'][p]==final['source_snapshots'][p]
    for label in ('real_focused','clean_focused'):
        expected={}
        for name in names:expected.update(read(HERE/'stages'/name/'validation.json')[label])
        assert root[label]==expected
    for row in block:
        assert row['evidence'][0]=='META/m4/hour443/validation.json'
        for p in row['evidence']:assert (ROOT/p).is_file(),p
    assert root['refreshed_original_variants']==len(refreshed) and not root['original_code_executed'] and not root['game_launches'] and not root['gameplay_executed']
    result=dict(passed=True,baseline_functions=443,complete_new_functions=expected_new,active_original_functions=current,translation_units=config['translation_units'],missing_module_controls=config['missing_module_controls'],real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],focused_scenarios=sorted(focused),refreshed_original_variants=len(refreshed),refreshed_support_variants=support_refreshes,stages=stage_reports,original_code_executed=False,game_launches=0,gameplay_executed=False,desktop_capture=False)
    print(json.dumps(result))
if __name__=='__main__':main()
