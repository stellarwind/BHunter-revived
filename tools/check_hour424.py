"""Read-only audit of resume400 admissions and exact source/build provenance."""
import json
from record_hour424 import ROOT,HERE,read,sha,active,reference,owners,validate_builds,CONFIG,STAGE

def main():
    ledger,raw=reference();v=read(HERE/'validation.json');items=owners();real=read(HERE/'integration-real.json');report=read(HERE/'clean/clean-checkout.json')
    clean=validate_builds(real,report,items,raw)
    assert v['passed'] and v['complete_new_functions']==CONFIG['complete_new_functions'] and v['active_original_functions']==CONFIG['active_original_functions']
    assert read(HERE/'owners.json')==items
    for name,digest in v['sources'].items():assert sha(ROOT/name)==sha(ROOT/v['source_snapshots'][name])==digest,name
    rows=active(ledger);assert len(rows)==len({r['original_va'] for r in rows})==CONFIG['active_original_functions']
    for r in rows:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    for o in items:
        r=next(r for r in rows if r['id']==o['address']+'-functional-c-hour424-'+STAGE)
        assert r['implementation']=='func' and r['integration']=='linked_harness' and not r['game_admitted']
        assert r['validation']['boundary']==o['ranges'] and not r['validation']['raw_byte_match'] and not r['validation']['original_code_executed']
        for label,build in (('fixtures',real),('clean_fixtures',clean)):
            assert r['validation'][label]=={s:next(v for v in build['scenarios'] if v.get('scenario')==s) for s in o['scenarios']}
    refreshed=read(HERE/'refreshed-variants.json');assert len(refreshed)==CONFIG.get('refreshed_variants',0)
    for item in refreshed:
        new=next(r for r in rows if r['id']==item['id']);old=next(r for r in ledger['records'] if r['id']==item['previous_id'])
        assert old['historical_variant'] and old['validation']['superseded_by']==new['id'] and old['source_sha256']==sha(ROOT/item['previous_snapshot'])
        assert new['source_sha256']==real['sources'][new['source']] and not new['validation']['raw_byte_match']
    assert not v['original_code_executed'] and not v['game_launches'] and not v['gameplay_executed'] and not v['desktop_capture']
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    print(json.dumps(dict(passed=True,active_functions=CONFIG['active_original_functions'],complete_new_functions=CONFIG['complete_new_functions'],translation_units=CONFIG['translation_units'],missing_module_controls=CONFIG['missing_module_controls'],real_checks=real['test']['checks'],clean_checks=clean['test']['checks'])))
if __name__=='__main__':main()
