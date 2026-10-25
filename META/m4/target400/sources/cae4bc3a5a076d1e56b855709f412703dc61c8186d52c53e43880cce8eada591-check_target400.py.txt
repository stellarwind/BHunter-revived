"""Read-only audit of target400 admissions and exact source/build provenance."""
import json
from record_target400 import ROOT,HERE,read,sha,active,reference,owners,validate_builds

def main():
    ledger,raw=reference();v=read(HERE/'validation.json');items=owners();real=read(HERE/'integration-real.json');report=read(HERE/'clean/clean-checkout.json')
    clean=validate_builds(real,report,items,raw)
    assert v['passed'] and v['complete_new_functions']==35 and v['active_original_functions']==400
    assert read(HERE/'owners.json')==items
    for name,digest in v['sources'].items():assert sha(ROOT/name)==sha(ROOT/v['source_snapshots'][name])==digest,name
    rows=active(ledger);assert len(rows)==len({r['original_va'] for r in rows})==400
    for r in rows:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
    for o in items:
        r=next(r for r in rows if r['id']==o['address']+'-functional-c-target400')
        assert r['implementation']=='func' and r['integration']=='linked_harness' and not r['game_admitted']
        assert r['validation']['boundary']==o['ranges'] and not r['validation']['raw_byte_match'] and not r['validation']['original_code_executed']
        for label,build in (('fixtures',real),('clean_fixtures',clean)):
            assert r['validation'][label]=={s:next(v for v in build['scenarios'] if v.get('scenario')==s) for s in o['scenarios']}
    assert not v['original_code_executed'] and not v['game_launches'] and not v['gameplay_executed'] and not v['desktop_capture']
    assert v['real_total']==real['test'] and v['clean_total']==clean['test']
    print(json.dumps(dict(passed=True,active_functions=400,complete_new_functions=35,translation_units=366,missing_module_controls=176,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'])))
if __name__=='__main__':main()
