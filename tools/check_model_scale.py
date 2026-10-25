"""Audit model scale and winding source evidence without executing a game or changing ledger."""
import json
from pathlib import Path
from record_model_scale import ROOT,HERE,SPECS,BASE_FUNCTIONS,active,read,sha,reference,boundaries,native_dependencies,EXPECTED_CONTROLS

def main():
    v=read(HERE/'validation.json');real=read(HERE/'integration-real.json');report=read(HERE/'clean/clean-checkout.json');clean=report['result']
    owners=read(HERE/'owners.json');ledger,raw=reference()
    assert v['passed'] and real['passed'] and clean['passed'] and real['sources']==clean['sources']
    native_dependencies(real,clean)
    assert 'META/native-input-dependencies.json' in v['sources']
    assert not report['original_assets_present'] and not report['compiler_in_checkout']
    assert owners==boundaries(raw)
    assert len(owners)==v['new_complete_functions']==v['complete_new_functions']==len(SPECS)
    for name,digest in v['sources'].items():assert sha(ROOT/name)==sha(ROOT/v['source_snapshots'][name])==digest,name
    assert all(v['sources'][name]==digest for name,digest in real['sources'].items())
    for build,prefix in ((real,'real'),(clean,'clean')):
        assert build['test']==v[prefix+'_total'] and not build['test']['failures'] and not build['original_code_required']
        assert sum(c['label'].startswith('compile-') for c in build['commands'])==v['translation_units']
        controls=[r for k,r in build.items() if k.startswith('missing_')]
        assert len(controls)==v['missing_module_controls']==EXPECTED_CONTROLS and all('LNK2001' in r for r in controls)
        assert sum(r['checks'] for r in build['scenarios'] if 'checks' in r and r.get('scenario')!='m2-foundations')==build['test']['checks']
        for scenario,row in v[prefix+'_focused'].items():
            assert next(r for r in build['scenarios'] if r.get('scenario')==scenario)==row
            assert not row['failures'] and row['checks']>0
    for owner in owners:
        record=next(r for r in ledger['records'] if r['id']==owner['address']+'-functional-c-model-scale')
        assert record['source_sha256']==real['sources'][record['source']]==v['sources'][owner['source']]
        assert record['implementation']=='func' and record['integration']=='linked_harness' and not record['game_admitted']
        assert not record['validation']['raw_byte_match'] and not record['validation']['original_code_executed']
        assert record['validation']['boundary']==owner['ranges']
        for name in ('link-map.txt','clean/link-map.txt'):assert ('_'+record['function']).encode() in (HERE/name).read_bytes()
        for prefix in ('real','clean'):
            expected={s:v[prefix+'_focused'][s] for s in owner['scenarios']}
            assert record['validation']['fixtures' if prefix=='real' else 'clean_fixtures']==expected
    admitted=active(ledger)
    assert len({r['original_va'] for r in admitted})==v['active_original_functions']==BASE_FUNCTIONS+len(SPECS)
    assert len(admitted)==len({r['original_va'] for r in admitted})
    for row in admitted:assert real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
    assert not v['original_code_executed'] and not v['game_launches'] and not v['gameplay_executed'] and not v['desktop_capture']
    print(json.dumps(dict(passed=True,complete_new_owners=len(owners),active_functions=v['active_original_functions'],translation_units=v['translation_units'],missing_module_controls=v['missing_module_controls'],real_checks=real['test']['checks'],clean_checks=clean['test']['checks'])))
if __name__=='__main__':main()

