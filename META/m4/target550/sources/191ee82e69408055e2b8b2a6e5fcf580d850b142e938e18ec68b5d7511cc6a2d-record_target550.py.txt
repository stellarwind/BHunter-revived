"""Admit the requested 550-owner checkpoint after fresh real/clean builds."""
import copy,json,hashlib,shutil
from pathlib import Path
from record_pickup_basic import ROOT,read,write,sha,active,reference,native_dependencies
HERE=ROOT/'META/m4/target550'
FAMILIES={'a':'player-query','a2':'projectile-create','a3':'menu-actions','b':'render-startup','b2':'remote-prediction','b3':'segment-owner','b4':'trail','c':'script-actor-path','c2':'script-actor-route','c3':'script-actor-tick','c4':'script-actor-draw','c5':'script-actor-query','c6':'script-actor-construct','c7':'mission-progress','parent':'network-game','transition':'menu-transition','d':'menu-runtime'}

def all_owners():
 result=[]
 for family,scenario in FAMILIES.items():
  for row in read(HERE/family/'owners.json'):
   row=copy.deepcopy(row);row['family']=family;row['scenario']=scenario
   result.append(row)
 assert len(result)==len({r['original_va'] for r in result})==50
 return result

def verify_span(raw,span):
 body=raw(int(span['start'],16),span['bytes'])
 assert len(body)==int(span['end_exclusive'],16)-int(span['start'],16)
 assert hashlib.sha256(body).hexdigest()==span['sha256']
 if 'raw' in span:assert body.hex()==span['raw']

def main():
 ledger,raw=reference();items=all_owners();real=read(ROOT/'build/m0/result.json')
 report=read(HERE/'clean/clean-checkout.json');clean=report['result']
 assert real['passed'] and clean['passed'] and report['exit_code']==0
 assert not report['original_assets_present'] and not report['compiler_in_checkout']
 assert real['sources']==clean['sources'];native_dependencies(real,clean)
 for name,digest in real['sources'].items():assert sha(ROOT/name)==digest,name
 for build in (real,clean):
  assert not build['original_code_required'] and build['test']['failures']==0
  assert sum(s['checks'] for s in build['scenarios'] if 'checks' in s and s.get('scenario')!='m2-foundations')==build['test']['checks']
  controls=[v for k,v in build.items() if k.startswith('missing_')]
  assert all('LNK2001' in v for v in controls)
  compiled=[c for c in build['commands'] if c['label'].startswith('compile-')]
  for c in compiled:
   rel='/'.join(Path(c['argv'][-1]).parts[-2:]);assert rel in build['sources'],rel
  for scenario in FAMILIES.values():
   rows=[s for s in build['scenarios'] if s.get('scenario')==scenario]
   assert len(rows)==1 and rows[0]['checks']>0 and rows[0]['failures']==0,scenario
 maps=[ROOT/'build/m0/bhunter-integration.map',HERE/'clean/link-map.txt']
 for item in items:
  assert item['role'] in ('game','engine_candidate')
  spans=item['boundary'] if isinstance(item['boundary'],list) else [item['boundary']]
  for span in spans:verify_span(raw,span)
  if item.get('owned_data'):
   data=read(ROOT/item['owned_data']);verify_span(raw,data);assert data['independent_function_count']==0
  for path in maps:assert ('_'+item['function']).encode() in path.read_bytes(),item['function']
 before=active(ledger);assert len(before)==500
 prior=read(ROOT/'META/m4/target500/validation.json');refreshed=[];affected=set()
 for old in before:
  source=old['source']
  if old['source_sha256']==real['sources'][source]:continue
  affected.add(source)
  # A changed admitted owner always becomes a new independently tested variant.
  snapshot=prior['source_snapshots'][source];assert sha(ROOT/snapshot)==old['source_sha256']
  new=copy.deepcopy(old);identifier=old['original_va']+'-functional-c-target550-context'
  old['historical_variant']=True;old['historical_source_path']=source;old['source']=snapshot
  old.setdefault('validation',{})['superseded_by']=identifier
  new['id']=identifier;new['source_sha256']=real['sources'][source]
  new['validation']['supersedes']=old['id'];new['validation']['raw_byte_match']=False
  previous_fixtures=new['validation'].get('fixtures',{});scenarios=list(previous_fixtures) if isinstance(previous_fixtures,dict) else ['texture-ownership','texture-controls','texture-policy','texture-image-owner','texture-draw-queue'];
  if not isinstance(previous_fixtures,dict):new['validation']['historical_fixture_summary']=previous_fixtures
  for key,build in [('fixtures',real),('clean_fixtures',clean)]:
   new['validation'][key]={s:next(v for v in build['scenarios'] if v.get('scenario')==s) for s in scenarios}
  new['validation']['limits'].append('Fresh same-owner source context: config owning profile deletion, painted text capture and/or real software texture initialization. Full current fixtures independently validate source; historical bytes/evidence remain separate, no byte-match transfer.')
  new['evidence']=['META/m4/target550/validation.json']+new['evidence']
  for path in maps:assert ('_'+new['function']).encode() in path.read_bytes()
  ledger['records'].append(new);refreshed.append(dict(original_va=new['original_va'],old=old['id'],new=identifier,snapshot=snapshot))
 for old in list(ledger['support_implementations']):
  if old.get('source') not in affected or old.get('historical_variant') or (isinstance(old.get('validation'),dict) and old['validation'].get('superseded_by')):continue
  source=old['source']
  if old.get('source_sha256')==real['sources'][source]:continue
  snapshot=prior['source_snapshots'][source];assert sha(ROOT/snapshot)==old['source_sha256']
  new=copy.deepcopy(old);identifier=old['id']+'-target550'
  old['historical_variant']=True;old['historical_source_path']=source;old['source']=snapshot;old.setdefault('validation',{})['superseded_by']=identifier
  new['id']=identifier;new['source_sha256']=real['sources'][source];new.setdefault('validation',{})['supersedes']=old['id'];new['validation']['original_code_executed']=False;new['evidence']=['META/m4/target550/validation.json']+new.get('evidence',[]);ledger['support_implementations'].append(new)
 for item in items:
  address=item['original_va'];assert address not in {r['original_va'] for r in before}
  identifier=address+'-functional-c-target550';v=copy.deepcopy(item.get('validation',{}));scenario=item['scenario']
  v.update(boundary=item['boundary'],raw_byte_match=False,original_code_executed=False,
   fixtures={scenario:next(s for s in real['scenarios'] if s.get('scenario')==scenario)},
   clean_fixtures={scenario:next(s for s in clean['scenarios'] if s.get('scenario')==scenario)})
  if item.get('owned_data'):v['owned_data']=item['owned_data']
  v.setdefault('limits',[]).extend(['Linked source harness; M3 skipped and M4/playable source game incomplete.','No original game code or gameplay automation; physical CD, PC24 and actual peer gameplay remain unvalidated.'])
  record=dict(id=identifier,original_va=address,function=item['function'],source=item['source'],source_sha256=real['sources'][item['source']],role=item['role'],provenance_confidence=item['provenance_confidence'],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=v,evidence=item.get('evidence',[])+['META/m4/target550/'+item['family']+'/README.md','META/m4/target550/validation.json'])
  for old in ledger['records']:
   if old['original_va']==address and not old.get('historical_variant'):
    assert old['implementation'] in ('wip','not_started');old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
  ledger['records'].append(record)
 admitted=active(ledger);assert len(admitted)==len({r['original_va'] for r in admitted})==550
 for r in admitted:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
 # Public context/lifetime adapters are separately recorded, never counted
 # as original functions or allowed to inherit original byte-match evidence.
 for source in sorted({item['source'] for item in items}):
  identifier='target550-'+Path(source).stem+'-source-bindings'
  assert not any(s['id']==identifier for s in ledger['support_implementations'])
  ledger['support_implementations'].append(dict(id=identifier,source=source,source_sha256=real['sources'][source],role='unknown',provenance_confidence='Explicit source context/lifetime adaptation; no claimed original function identity',implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,original_function_count=0,validation=dict(original_code_executed=False,contract='Typed borrowed state, registry/lifetime bindings and safe malformed-input guards documented in the corresponding family README; no independent original-function count.'),evidence=['META/m4/target550/validation.json']))
 sources=dict(real['sources'])
 for name in ['tools/m0.py','tools/record_target550.py','tools/check_target550.py','META/native-input-dependencies.json','META/native-render-dependencies.json','META/native-directplay-dependencies.json','META/native-lobby-dependencies.json']:sources[name]=sha(ROOT/name)
 snapshots={}
 for name,digest in sources.items():
  previous=prior['source_snapshots'].get(name)
  if previous and prior['sources'].get(name)==digest and sha(ROOT/previous)==digest:snapshots[name]=previous;continue
  target=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');target.parent.mkdir(exist_ok=True);shutil.copyfile(ROOT/name,target);snapshots[name]=target.relative_to(ROOT).as_posix()
 validation=dict(passed=True,baseline_functions=500,complete_new_functions=50,active_original_functions=550,
  translation_units=len([c for c in real['commands'] if c['label'].startswith('compile-')]),
  missing_module_controls=len([k for k in real if k.startswith('missing_')]),real_total=real['test'],clean_total=clean['test'],sources=sources,source_snapshots=snapshots,
  focused={family:{'real':next(s for s in real['scenarios'] if s.get('scenario')==scenario),'clean':next(s for s in clean['scenarios'] if s.get('scenario')==scenario)} for family,scenario in FAMILIES.items()},
  original_code_executed=False,game_launches=0,gameplay_executed=False,physical_input=False,desktop_capture=False,
  notes=['Distinct complete original code owners only; source bindings and switch/name data add zero.','550 source owners is not playable-game completion or a stable binary denominator. The727 inventory omitted callbacks and retained independently verified runtime bodies.','No byte-match evidence transferred. Existing silent native sound fixtures remain explicitly separate from audible gameplay.'])
 write(HERE/'owners.json',items);write(HERE/'refreshed-variants.json',refreshed);write(HERE/'integration-real.json',real);shutil.copyfile(maps[0],HERE/'link-map.txt');write(HERE/'validation.json',validation)
 ledger['updated']='2026-10-26';ledger['current_source_frontier']='META/m4/target550/NEXT.md';ledger['latest_source_session']='META/m4/target550/session.json'
 write(ROOT/'META/function-status.json',ledger)
 print(json.dumps({k:validation[k] for k in ('passed','active_original_functions','complete_new_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()
