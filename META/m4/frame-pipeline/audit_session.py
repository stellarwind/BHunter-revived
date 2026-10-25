"""Audit the timed source session's three immutable admission checkpoints."""
import hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'tools'))
import record_frame_pipeline as frame
import record_window_dispatch as window
import record_sound_loader as sound
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
ledger,raw=sound.reference();active=sound.active(ledger)
functions,tus,controls=322,310,148
owners=[];checkpoints=[]
for module,delta,tu_delta,control_delta in [(frame,12,18,9),(window,4,6,3),(sound,1,2,1)]:
 v=read(module.HERE/'validation.json');saved=read(module.HERE/'owners.json')
 assert saved==module.boundaries(raw)
 assert v['passed'] and v['baseline_functions']==functions and v['baseline_translation_units']==tus
 functions+=delta;tus+=tu_delta;controls+=control_delta
 assert v['complete_new_functions']==delta and v['active_original_functions']==functions
 assert v['translation_units']==tus and v['missing_module_controls']==controls
 assert not v['original_code_executed'] and not v['game_launches'] and not v['gameplay_executed']
 for name,digest in v['sources'].items():assert sha(ROOT/v['source_snapshots'][name])==digest,name
 for o in saved:
  records=[r for r in active if r['original_va']==o['address']]
  assert len(records)==1 and records[0]['source']==o['source']
  assert records[0]['source_sha256']==sha(ROOT/o['source'])
  assert records[0]['validation']['boundary']==o['ranges']
  for name in ['link-map.txt','clean/link-map.txt']:
   assert ('_'+o['function']).encode() in (module.HERE/name).read_bytes()
 owners+=saved;checkpoints.append(dict(folder=module.HERE.relative_to(ROOT).as_posix(),new_owners=delta,functions=functions,translation_units=tus,controls=controls,real=v['real_total']['checks'],clean=v['clean_total']['checks']))
assert len(owners)==len({o['address'] for o in owners})==17
assert len({r['original_va'] for r in active})==339
prior=read(ROOT/'META/m4/hud-continuation/validation.json')
assert checkpoints[-1]['real']-prior['real_total']['checks']==915
assert checkpoints[-1]['clean']-prior['clean_total']['checks']==552
old=next(r for r in ledger['records'] if r['id']=='0047b950-wave-data-subset-c')
assert old['historical_variant'] and old['validation']['superseded_by']=='0047b950-functional-c-sound-loader'
out=dict(passed=True,baseline_functions=322,new_complete_owners=17,active_functions=339,translation_units=336,missing_module_controls=161,
 original_body_bytes=sum(o['bytes'] for o in owners),original_instructions=sum(o['instructions'] for o in owners),
 real_checks=checkpoints[-1]['real'],clean_checks=checkpoints[-1]['clean'],additional_real_checks=915,additional_clean_checks=552,
 checkpoints=checkpoints,original_code_executed=False,game_launches=0,gameplay_executed=False)
(Path(__file__).parent/'session-audit.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8',newline='\n')
print(json.dumps(out))
