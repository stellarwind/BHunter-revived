"""Read-only audit of the exact admitted 550-owner source checkpoint."""
import json,hashlib
from record_pickup_basic import ROOT,read,write,sha,active,reference,native_dependencies
from record_target550 import HERE,FAMILIES,verify_span

def main():
 ledger,raw=reference();validation=read(HERE/'validation.json');real=read(HERE/'integration-real.json');report=read(HERE/'clean/clean-checkout.json');clean=report['result'];owners=read(HERE/'owners.json')
 admitted=active(ledger)
 assert validation['passed'] and len(admitted)==len({r['original_va'] for r in admitted})==550
 assert len(owners)==len({r['original_va'] for r in owners})==50
 assert validation['baseline_functions']==500 and validation['complete_new_functions']==50
 assert real['passed'] and clean['passed'] and not report['original_assets_present'] and not report['compiler_in_checkout'] and report['exit_code']==0
 assert real['sources']==clean['sources'];native_dependencies(real,clean)
 for name,digest in validation['sources'].items():
  assert sha(ROOT/name)==digest,name
  assert sha(ROOT/validation['source_snapshots'][name])==digest,name
 for row in admitted:
  assert row['source_sha256']==real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
  assert row['role'] in ('game','engine_candidate'),row['id']
 for row in owners:
  span=row['boundary'];spans=span if isinstance(span,list) else [span]
  for span in spans:verify_span(raw,span)
  if row.get('owned_data'):
   data=read(ROOT/row['owned_data']);verify_span(raw,data);assert data['independent_function_count']==0
  for p in [HERE/'link-map.txt',HERE/'clean/link-map.txt']:assert ('_'+row['function']).encode() in p.read_bytes()
 for build,key in [(real,'real_total'),(clean,'clean_total')]:
  assert build['test']==validation[key] and build['test']['failures']==0
  assert not build['original_code_required']
  assert len([c for c in build['commands'] if c['label'].startswith('compile-')])==validation['translation_units']
  controls=[v for k,v in build.items() if k.startswith('missing_')]
  assert len(controls)==validation['missing_module_controls'] and all('LNK2001' in v for v in controls)
  assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
  for scenario in FAMILIES.values():
   rows=[s for s in build['scenarios'] if s.get('scenario')==scenario]
   assert len(rows)==1 and rows[0]['checks'] and rows[0]['failures']==0
 # Every retained constant annotation independently agrees with immutable PE.
 constants=0
 for family in FAMILIES:
  path=HERE/family/'constants.json'
  if not path.exists():continue
  data=read(path)
  if isinstance(data,list):
   for item in data:
    if 'raw' in item and 'original_va' in item:
     body=bytes.fromhex(item['raw']);assert raw(int(item['original_va'],16),len(body))==body;constants+=1
 assert not validation['original_code_executed'] and validation['game_launches']==0 and not validation['gameplay_executed']
 result=dict(passed=True,admitted_original_owners=550,new_whole_owners=50,translation_units=validation['translation_units'],missing_module_controls=validation['missing_module_controls'],real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],verified_constant_records=constants,reference_files_unchanged=True,original_code_executed=False,game_launches=0)
 write(HERE/'final-audit.json',result);print(json.dumps(result))
if __name__=='__main__':main()
