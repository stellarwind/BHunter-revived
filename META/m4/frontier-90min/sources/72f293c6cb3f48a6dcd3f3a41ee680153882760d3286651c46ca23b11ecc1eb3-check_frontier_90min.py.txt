"""Audit whole-owner source restoration; never launches game/original code."""
import collections,csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];HERE=ROOT/'META/m4/frontier-90min'
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 v=read(HERE/'validation.json');owners=read(HERE/'owners.json');real=read(HERE/'integration-real.json');report=read(HERE/'clean/clean-checkout.json');clean=report['result'];ledger=read(ROOT/'META/function-status.json')
 assert v['passed'] and real['passed'] and clean['passed'] and not report['original_assets_present'] and not report['compiler_in_checkout']
 assert len(owners)==v['complete_new_functions'] and len({r['address'] for r in owners})==len(owners)
 assert real['sources']==clean['sources']
 for name,digest in v['sources'].items():
  assert sha(ROOT/name)==digest,name
  assert sha(ROOT/v['source_snapshots'][name])==digest,name
 for build,prefix in [(real,'real'),(clean,'clean')]:
  assert build['test']==v[prefix+'_total'] and not build['test']['failures'] and not build['original_code_required']
  assert sum(c['label'].startswith('compile-') for c in build['commands'])==v['translation_units']
  assert sum(k.startswith('missing_') for k in build)==v['missing_module_controls']
  assert all('LNK2001' in value for k,value in build.items() if k.startswith('missing_'))
  assert sum(r['checks'] for r in build['scenarios'] if 'checks' in r and r.get('scenario')!='m2-foundations')==build['test']['checks']
  for scenario,row in v[prefix+'_focused'].items():assert next(r for r in build['scenarios'] if r.get('scenario')==scenario)==row and not row['failures']
  for scenario in ['input-collection','input-devices']:
   row=next(r for r in build['scenarios'] if r.get('scenario')==scenario);assert row['native_recenter_calls']==0
  assert next(r for r in build['scenarios'] if r.get('scenario')=='input-devices')['native_capture_calls']==0
 exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
 sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
 def raw(a,n):
  r=a-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=r and r+n<=s[1]+s[2]);return exe[offset+r-start:offset+r-start+n]
 for owner in owners:
  at=int(owner['address'],16);body=bytearray();rows=list(csv.DictReader((ROOT/owner['original']).open(),delimiter='\t'))
  for row in rows:
   assert int(row['address'],16)==at;b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
  assert len(body)==owner['bytes'] and len(rows)==owner['instructions'] and hashlib.sha256(body).hexdigest()==owner['body_sha256']
  assert body==raw(int(owner['address'],16),len(body))
  record=next(r for r in ledger['records'] if r['id']==owner['address']+'-functional-c')
  assert record['implementation']=='func' and record['integration']=='linked_harness' and not record['game_admitted'] and not record['validation']['raw_byte_match']
  assert record['source_sha256']==sha(ROOT/record['source'])
  for name in ['link-map.txt','clean/link-map.txt']:assert ('_'+record['function']).encode() in (HERE/name).read_bytes(),record['function']
 for identifier in v['corrected_variants']:
  row=next(r for r in ledger['records'] if r['id']==identifier);assert row['source_sha256']==sha(ROOT/row['source']) and not row.get('historical_variant')
  old=next(r for r in ledger['records'] if r.get('validation',{}).get('superseded_by')==identifier);assert old['historical_variant'] and old['source_sha256']!=row['source_sha256']
 assert next(r for r in ledger['records'] if r['id']=='00454fc0-rect-uv-corrected-c')['implementation']=='wip'
 ids=[r['id'] for r in ledger['records']];assert len(ids)==len(set(ids))
 active=[r for r in ledger['records'] if r.get('implementation') in ['func','bm'] and r.get('integration')=='linked_harness' and not r.get('historical_variant') and not r.get('validation',{}).get('superseded_by')]
 for row in active:
  assert row['source'] in real['sources'] and real['sources'][row['source']]==sha(ROOT/row['source'])
  if row.get('source_sha256'):assert row['source_sha256']==sha(ROOT/row['source']),row['id']
 for address in v['unimplemented_callers']:
  rows=[r for r in ledger['records'] if r.get('original_va')==address and not r.get('historical_variant')];assert rows and all(r['implementation'] in ['not_started','wip'] for r in rows)
 for name,digest in v['native_images'].items():assert sha(HERE/name)==digest
 for item in read(ROOT/'META/reference-manifest.json')['files']:
  if item['root']=='repo':assert sha(ROOT/item['path'])==item['sha256'],item['path']
 assert not v['original_code_executed'] and not v['game_launches'] and not v['gameplay_executed'] and not v['native_input_capture_calls'] and not v['native_input_recenter_calls']
 print(json.dumps(dict(passed=True,whole_new_owners=len(owners),active_unique_source_functions=len({r['original_va'] for r in active}),translation_units=v['translation_units'],real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],missing_module_controls=v['missing_module_controls'],original_code_executed=False,game_launches=0)))
if __name__=='__main__':main()
