"""Admit the requested 600-owner checkpoint after fresh real/clean builds."""
import copy,json,hashlib,shutil,re
from pathlib import Path
from record_pickup_basic import ROOT,read,write,sha,active,reference,native_dependencies
HERE=ROOT/'META/m4/target600'
FAMILIES={'a': 'area-projectile', 'a2': 'weapon-path', 'a3': 'pulse-visual', 'a4': 'pulse-projectile', 'a5': 'weapon-fire', 'a6': 'weapon-homing-track', 'a7': 'weapon-pulse-launch', 'a8': 'weapon-arc-fire', 'a9': 'effect-split', 'a10': 'weapon-effect-fire', 'b': 'actor-vehicle-draw', 'b2': 'actor-vehicle-lifetime', 'b3': 'actor-damage', 'b4': 'actor-pursuit-route', 'b5': 'editor-numeric', 'b6': 'effect-orbit-tick', 'b7': 'actor-attack', 'c': 'actor-basis-control', 'c2': 'network-scores', 'c3': 'player-force-control', 'c4': 'player-force-control', 'c5': 'network-debug', 'c6': 'flyby-construct', 'c7': 'player-aim', 'c8': 'network-effect-receive', 'parent': 'effect-projectile', 'parent-create': 'effect-create', 'parent-puff': 'effect-puff'}

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

def verify_constants(raw):
 count=0
 for family in FAMILIES:
  path=HERE/family/'constants.json'
  if not path.exists():continue
  data=read(path);assert isinstance(data,list),str(path)
  for item in data:
   address=item.get('original_va',item.get('address'));hex_body=item.get('raw',item.get('raw_hex'))
   assert address and hex_body is not None,(str(path),item)
   body=bytes.fromhex(hex_body);assert raw(int(address,16),len(body))==body,(str(path),address)
   if item.get('sha256'):assert hashlib.sha256(body).hexdigest()==item['sha256'],address
   count+=1
 return count

def verify_full_build_gate(raw,real,clean,maps):
 plan=read(HERE/'integration-plan.json');prior=read(ROOT/'META/m4/target550/integration-real.json')
 relative=lambda command:'/'.join(Path(command['argv'][-1]).parts[-2:])
 baseline=[relative(c) for c in prior['commands'] if c['label'].startswith('compile-')]
 expected=baseline+[row[0] for row in plan['modules']]
 assert len(expected)==len(set(expected))==540
 expected_controls={k for k in prior if k.startswith('missing_')}|{'missing_'+name+'_control' for name,symbol in plan['guards']}
 assert len(expected_controls)==264
 expected_scenarios={t['scenario'] for t in plan['tests']};assert len(expected_scenarios)==27 and expected_scenarios==set(FAMILIES.values())
 for build,path in zip((real,clean),maps):
  compiled=[relative(c) for c in build['commands'] if c['label'].startswith('compile-')]
  assert len(compiled)==len(set(compiled))==540 and set(compiled)==set(expected),'exact compile-unit set'
  controls={k for k in build if k.startswith('missing_')};assert controls==expected_controls,'exact omission-control set'
  assert all('LNK2001' in build[k] for k in controls)
  for scenario in expected_scenarios:
   found=[x for x in build['scenarios'] if x.get('scenario')==scenario];assert len(found)==1 and found[0]['checks']>0 and not found[0]['failures'],scenario
  assert sha(path)==build['map_sha256'],str(path)
 return verify_constants(raw)

def map_has(path,function):
 return re.search(r'(?<![A-Za-z0-9_])_'+re.escape(function)+r'(?:@[0-9]+)?(?=\s|$)',path.read_text(errors='replace')) is not None


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
 verify_full_build_gate(raw,real,clean,maps)
 for item in items:
  assert item['role'] in ('game','engine_candidate')
  spans=item['boundary'] if isinstance(item['boundary'],list) else [item['boundary']]
  for span in spans:verify_span(raw,span)
  if item.get('owned_data'):
   data=read(ROOT/item['owned_data']);verify_span(raw,data);assert data['independent_function_count']==0
  for path in maps:assert map_has(path,item['function']),item['function']
 before=active(ledger);assert len(before)==550
 prior=read(ROOT/'META/m4/target550/validation.json');refreshed=[];affected=set()
 for old in before:
  source=old['source']
  if old['source_sha256']==real['sources'][source]:continue
  affected.add(source)
  # A changed admitted owner always becomes a new independently tested variant.
  snapshot=prior['source_snapshots'][source];assert sha(ROOT/snapshot)==old['source_sha256']
  new=copy.deepcopy(old);identifier=old['original_va']+'-functional-c-target600-context'
  old['historical_variant']=True;old['historical_source_path']=source;old['source']=snapshot
  old.setdefault('validation',{})['superseded_by']=identifier
  new['id']=identifier;new['source_sha256']=real['sources'][source]
  new['validation']['supersedes']=old['id'];new['validation']['raw_byte_match']=False
  previous_fixtures=new['validation'].get('fixtures',{});scenarios=list(previous_fixtures) if isinstance(previous_fixtures,dict) else ['texture-ownership','texture-controls','texture-policy','texture-image-owner','texture-draw-queue'];
  if not isinstance(previous_fixtures,dict):new['validation']['historical_fixture_summary']=previous_fixtures
  for key,build in [('fixtures',real),('clean_fixtures',clean)]:
   new['validation'][key]={s:next(v for v in build['scenarios'] if v.get('scenario')==s) for s in scenarios}
  new['validation']['limits'].append('Fresh independently linked source context for the current whole-owner families. Full current fixtures independently validate source; historical bytes/evidence remain separate, no byte-match transfer.')
  new['evidence']=['META/m4/target600/validation.json']+new['evidence']
  for path in maps:assert map_has(path,new['function'])
  ledger['records'].append(new);refreshed.append(dict(original_va=new['original_va'],old=old['id'],new=identifier,snapshot=snapshot))
 for old in list(ledger['support_implementations']):
  if old.get('source') not in affected or old.get('historical_variant') or (isinstance(old.get('validation'),dict) and old['validation'].get('superseded_by')):continue
  source=old['source']
  if old.get('source_sha256')==real['sources'][source]:continue
  snapshot=prior['source_snapshots'][source];assert sha(ROOT/snapshot)==old['source_sha256']
  new=copy.deepcopy(old);identifier=old['id']+'-target600'
  old['historical_variant']=True;old['historical_source_path']=source;old['source']=snapshot;old.setdefault('validation',{})['superseded_by']=identifier
  new['id']=identifier;new['source_sha256']=real['sources'][source];new.setdefault('validation',{})['supersedes']=old['id'];new['validation']['original_code_executed']=False;new['evidence']=['META/m4/target600/validation.json']+new.get('evidence',[]);ledger['support_implementations'].append(new)
 for item in items:
  address=item['original_va'];assert address not in {r['original_va'] for r in before}
  identifier=address+'-functional-c-target600';v=copy.deepcopy(item.get('validation',{}));scenario=item['scenario']
  v.update(boundary=item['boundary'],raw_byte_match=False,original_code_executed=False,
   fixtures={scenario:next(s for s in real['scenarios'] if s.get('scenario')==scenario)},
   clean_fixtures={scenario:next(s for s in clean['scenarios'] if s.get('scenario')==scenario)})
  if item.get('owned_data'):v['owned_data']=item['owned_data']
  v.setdefault('limits',[]).extend(['Linked source harness; M3 skipped and M4/playable source game incomplete.','No original game code or gameplay automation; physical CD, PC24 and actual peer gameplay remain unvalidated.'])
  record=dict(id=identifier,original_va=address,function=item['function'],source=item['source'],source_sha256=real['sources'][item['source']],role=item['role'],provenance_confidence=item['provenance_confidence'],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=v,evidence=item.get('evidence',[])+['META/m4/target600/'+item['family']+'/README.md','META/m4/target600/validation.json'])
  for old in ledger['records']:
   if old['original_va']==address and not old.get('historical_variant'):
    assert old['implementation'] in ('wip','not_started');old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
  ledger['records'].append(record)
 admitted=active(ledger);assert len(admitted)==len({r['original_va'] for r in admitted})==600
 for r in admitted:assert r['source_sha256']==real['sources'][r['source']]==sha(ROOT/r['source']),r['id']
 # Public context/lifetime adapters are separately recorded, never counted
 # as original functions or allowed to inherit original byte-match evidence.
 for source in sorted({item['source'] for item in items}):
  identifier='target600-'+Path(source).stem+'-source-bindings'
  assert not any(s['id']==identifier for s in ledger['support_implementations'])
  ledger['support_implementations'].append(dict(id=identifier,source=source,source_sha256=real['sources'][source],role='unknown',provenance_confidence='Explicit source context/lifetime adaptation; no claimed original function identity',implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,original_function_count=0,validation=dict(original_code_executed=False,contract='Typed borrowed state, registry/lifetime bindings and safe malformed-input guards documented in the corresponding family README; no independent original-function count.'),evidence=['META/m4/target600/validation.json']))
 sources=dict(real['sources'])
 for name in ['tools/m0.py','tools/record_target600.py','tools/check_target600.py','META/native-input-dependencies.json','META/native-render-dependencies.json','META/native-directplay-dependencies.json','META/native-lobby-dependencies.json']:sources[name]=sha(ROOT/name)
 snapshots={}
 for name,digest in sources.items():
  previous=prior['source_snapshots'].get(name)
  if previous and prior['sources'].get(name)==digest and sha(ROOT/previous)==digest:snapshots[name]=previous;continue
  target=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');target.parent.mkdir(exist_ok=True);shutil.copyfile(ROOT/name,target);snapshots[name]=target.relative_to(ROOT).as_posix()
 validation=dict(passed=True,baseline_functions=550,complete_new_functions=50,active_original_functions=600,
  translation_units=len([c for c in real['commands'] if c['label'].startswith('compile-')]),
  missing_module_controls=len([k for k in real if k.startswith('missing_')]),real_total=real['test'],clean_total=clean['test'],sources=sources,source_snapshots=snapshots,
  focused={family:{'real':next(s for s in real['scenarios'] if s.get('scenario')==scenario),'clean':next(s for s in clean['scenarios'] if s.get('scenario')==scenario)} for family,scenario in FAMILIES.items()},
  original_code_executed=False,game_launches=0,gameplay_executed=False,physical_input=False,desktop_capture=False,
  notes=['Distinct complete original code owners only; source bindings and switch/name data add zero.','600 source owners is not playable-game completion or a stable binary denominator. The727 inventory omitted callbacks and retained independently verified runtime bodies.','No byte-match evidence transferred. Existing silent native sound fixtures remain explicitly separate from audible gameplay.'])
 write(HERE/'owners.json',items);write(HERE/'refreshed-variants.json',refreshed);write(HERE/'integration-real.json',real);shutil.copyfile(maps[0],HERE/'link-map.txt');write(HERE/'validation.json',validation)
 ledger['updated']='2026-10-26';ledger['current_source_frontier']='META/m4/target600/NEXT.md';ledger['latest_source_session']='META/m4/target600/session.json'
 write(ROOT/'META/function-status.json',ledger)
 print(json.dumps({k:validation[k] for k in ('passed','active_original_functions','complete_new_functions','translation_units','missing_module_controls','real_total','clean_total')}))
if __name__=='__main__':main()
