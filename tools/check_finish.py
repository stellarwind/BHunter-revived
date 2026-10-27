"""Verify final inventoried source ownership, immutable references and both actual builds."""
import hashlib,json,re,sys,struct,argparse
from pathlib import Path
from record_pickup_basic import ROOT,reference,read,write,sha,active,native_dependencies
sys.path.insert(0,str(ROOT/'analysis/astra-review'))
from targeted_probes import parse_coff
HERE=ROOT/'META/m4/finish'
FAMILIES=['reset','editor-draw','editor-control','world-load','screen-region','mission-story','weapon-dispatch','remote-fire','remote-tick','remote-packets','remote-position','remote-messages','player','game-bootstrap','startup-bootstrap']

def runtime_proof(raw,catalog):
 report=read(ROOT/'META/m4/remaining129/runtime-review.json');library=Path(report['library']);body=library.read_bytes();assert sha(library)==report['library_sha256'];objects={};pos=8
 while pos+60<=len(body):
  n=int(body[pos+48:pos+58].strip(b'\0 '));obj=body[pos+60:pos+60+n];pos+=60+n+(n&1)
  if obj[:2]==b'\x4c\x01':objects[hashlib.sha256(obj).hexdigest()]=parse_coff(obj)
 def mask(section,off,n):
  out=bytearray(b'\xff'*n)
  for relocation in section['relocations']:
   at=relocation['offset']-off
   if at>=n or at+4<=0:continue
   assert relocation['kind'] in (6,20) and 0<=at<=n-4
   out[at:at+4]=b'\0'*4
  return bytes(out)
 for address,hits in report['matches'].items():
  for hit in hits:
   symbols,sections=objects[hit['object_sha256']];section=sections[hit['section']]
   assert any(s['name']==hit['symbol'] and s['section']==hit['section'] and s['value']==hit['offset'] for s in symbols.values())
   for span in hit['ranges']:
    lo=int(span['start'],16);n=int(span['end_exclusive'],16)-lo;off=hit['offset']+lo-int(address,16);original=raw(lo,n);control=mask(section,off,n)
    assert section['data'][off:off+n]==bytes.fromhex(span['library_hex']) and original.hex()==span['original_hex'] and control.hex()==span['mask_hex']
    assert all((a&m)==(b&m) for a,b,m in zip(original,section['data'][off:off+n],control)) and hashlib.sha256(original).hexdigest()==span['sha256']
 for entry in report['verified_sections']:
  symbols,sections=objects[entry['object_sha256']];code=bytes.fromhex(entry['library_hex']);control=bytes.fromhex(entry['mask_hex']);original=raw(int(entry['start'],16),len(code))
  assert any(s['data']==code and mask(s,0,len(code))==control for s in sections.values())
  assert original.hex()==entry['original_hex'] and all((a&m)==(b&m) for a,b,m in zip(original,code,control))
 classification=read(ROOT/'META/m4/remaining129/runtime-classification.json');assert len(classification['entries'])==50
 for entry in classification['entries']:
  assert catalog[entry['address']]['role']==entry['role']
  if entry['address']!='0047e3ca':assert entry['proof']==catalog[entry['address']]['behavior']
 assert raw(0x4882d0,6)==bytes.fromhex('ff25f0904800')
 # Refined exact alias proof supersedes the historical unresolved alias label.
 alias=read(HERE/'remote-position/math-alias.json');assert alias['passed'] and alias['symbol']=='__CIfmod' and alias['descriptor_name']=='fmod'
 symbols,sections=objects[alias['object_sha256']]
 assert any(s['name']=='__CIfmod' for s in symbols.values()) and any(s['name']=='__OP_FMODjmptab' for s in symbols.values())
 for span in alias['sections']:
  original=raw(int(span['start'],16),span['bytes']);code=bytes.fromhex(span['library']);control=bytes.fromhex(span['mask'])
  assert original.hex()==span['original'] and any(s['data']==code and mask(s,0,len(code))==control for s in sections.values())
  assert all((a&m)==(b&m) for a,b,m in zip(original,code,control))
  test=bytearray(code);at=next(i for i,m in enumerate(control) if m);test[at]^=1;assert not all((a&m)==(b&m) for a,b,m in zip(original,test,control))
 return len(classification['entries'])

def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--clean-evidence',type=Path,default=HERE/'clean');args=parser.parse_args();clean_dir=args.clean_evidence.resolve()
 ledger,raw=reference();current=active(ledger);by_va={r['original_va']:r for r in current};assert len(current)==len(by_va)==679
 real=read(ROOT/'build/m0/result.json');clean_report=read(clean_dir/'clean-checkout.json');clean=clean_report['result']
 assert real['passed'] and clean['passed'] and not clean_report['exit_code'] and not clean_report['original_assets_present'] and not clean_report['compiler_in_checkout']
 assert real['sources']==clean['sources'];native_dependencies(real,clean)
 for r in current:
  assert r['source_sha256']==sha(ROOT/r['source'])==real['sources'][r['source']],r['id']
  path=ROOT/r['evidence'][0]
  if path.suffix=='.json':
   evidence=read(path);snapshot=evidence.get('source_snapshots',{}).get(r['source']) if isinstance(evidence,dict) else None
   if snapshot:assert sha(ROOT/snapshot)==r['source_sha256']
 maps=[(ROOT/'build/m0/bhunter-integration.map').read_text(errors='replace'),(clean_dir/'link-map.txt').read_text(errors='replace')]
 owners={};running=656
 for family in FAMILIES:
  folder=HERE/family;spec=read(folder/'admission.json');validation=read(folder/'validation.json');items=read(folder/'owners.json')
  assert spec['baseline']==running==validation['baseline_functions'] and validation['new_owners']==len(items)
  running+=len(items);assert validation['active_original_functions']==running
  for source,digest in validation['sources'].items():assert sha(ROOT/validation['source_snapshots'][source])==digest
  for o in items:
   va=o['original_va'];assert va not in owners;owners[va]=o;instructions=read(folder/'original'/(va+'.disasm.json'));body=bytearray();at=int(va,16)
   for instruction in instructions:
    assert int(instruction['address'],16)==at;chunk=bytes.fromhex(instruction['bytes']);assert raw(at,len(chunk))==chunk;body.extend(chunk);at+=len(chunk)
   assert len(body)==o['bytes'] and bytes(body)==raw(int(va,16),len(body))
   if instructions[-1]['mnemonic']!='RET':
    assert o.get('terminal_interior_jump') and instructions[-1]['mnemonic']=='JMP' and int(va,16)<=int(instructions[-1]['operands'][0],16)<at
    assert any(i['mnemonic']=='RET' for i in instructions)
   r=by_va[va];boundary=r['validation']['boundary'];assert boundary['start']==va and boundary['bytes']==len(body) and int(boundary['end_exclusive'],16)==at and boundary['sha256']==hashlib.sha256(body).hexdigest()
   assert not r['validation']['raw_byte_match'] and not r['validation']['original_code_executed'] and not r['game_admitted']
   for text in maps:assert re.search(r'(?<!\w)_'+re.escape(o['function'])+r'(?=\s|$)',text),o['function']
  for build in (real,clean):
   scenario=next(r for r in build['scenarios'] if r.get('scenario')==spec['scenario']);assert scenario['checks'] and not scenario['failures']
   assert build['missing_'+spec['module']+'_control']=='rejected with LNK2001'
 assert running==679 and len(owners)==23
 for family in ('source-normalization','player-binding'):
  folder=HERE/family;v=read(folder/'validation.json');assert v['baseline_functions']==679 and v['active_original_functions']==679 and v['new_owners']==0
  for source,digest in v['sources'].items():assert sha(ROOT/v['source_snapshots'][source])==digest
  for item in read(folder/'refreshed-variants.json'):
   old=next(r for r in ledger['records'] if r['id']==item['old']);new=next(r for r in ledger['records'] if r['id']==item['new']);assert old['historical_variant'] and old['validation']['superseded_by']==new['id'];assert sha(ROOT/item['snapshot'])==old['source_sha256']
 catalog={r['address']:r for r in read(ROOT/'META/m1/role-catalog.json')['records']};assert len(catalog)==891
 # Pure forwarding alias to existing config save has no independent behavior.
 alias=read(HERE/'aliases/0042f350.json');code=raw(0x42f350,5);assert code.hex()==alias['hex'] and code[0]==0xe9 and 0x42f355+struct.unpack('<i',code[1:])[0]==0x402370
 assert catalog['0042f350']['alias_of']=='00402370' and catalog['0042f350']['role']=='shared_tail' and alias['original_function_count']==0 and '00402370' in by_va
 entries=[r for r in ledger['records'] if r['original_va']=='0042f350' and not r.get('historical_variant')];assert len(entries)==1 and entries[0]['implementation']=='reused' and entries[0]['original_function_count']==0
 excluded={'library_verified','compiler_support','import_thunk','shared_tail'};candidates={va for va,r in catalog.items() if r['role'] not in excluded};missing=candidates-set(by_va);outside=set(by_va)-set(catalog)
 assert len(candidates)==679 and not missing and not outside and catalog['00443ff0']['added_after_original_catalog']
 prior=read(ROOT/'META/m4/target656/remaining-catalog.json');assert len(prior)==24 and all(r['address'] in owners or r['address']=='0042f350' for r in prior)
 data=read(HERE/'game-bootstrap/data-audit.json');assert data['passed'] and data['exact_reproduction'] and sha(ROOT/data['source'])==data['source_sha256'] and sha(ROOT/data['generator'])==data['generator_sha256']
 vm=read(ROOT/'META/m4/target656/vm/original/00443ff0.disasm.json');assert b''.join(bytes.fromhex(r['bytes']) for r in vm)==raw(0x443ff0,5875) and vm[-1]['mnemonic']=='RET'
 libraries=runtime_proof(raw,catalog)
 for build in (real,clean):
  assert not build['original_code_required'] and not build['test']['failures']
  assert sum(r.get('checks',0) for r in build['scenarios'] if r.get('scenario')!='m2-foundations')==build['test']['checks']
  assert len([r for r in build['commands'] if r['label'].startswith('compile-')])==601
  controls=[v for k,v in build.items() if k.startswith('missing_')];assert len(controls)==295 and all('LNK2001' in v for v in controls)
 report=dict(passed=True,baseline_original_owners=656,new_whole_source_owners=23,admitted_original_owners=679,translation_units=601,missing_module_controls=295,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],compiled_source_inputs_identical=True,active_source_hashes_verified=679,new_original_owner_bytes_verified=23,catalogue_records=891,catalogue_distinct_source_owners=679,source_owner_added_to_catalogue='00443ff0',remaining_inventoried_owners=[],forwarding_aliases_counted_zero=['0042f350'],prior_runtime_exclusions_verified=libraries,exact_fmod_alias_proven=True,current_player_binding='Explicit checked source registry, no pointer inside original player plume words',reference_files_unchanged=True,game_launches=0,original_code_executed=False,automated_gameplay=False,screenshots=0,delegated=False,inventoried_source_recovery_complete=True,positive_full_native_bootstrap_executed=False,shipping_game_target_complete=False,M3='skipped',M4='incomplete')
 write(HERE/'remaining-catalog.json',[]);write(HERE/'final-audit.json',report);write(HERE/'validation.json',dict(passed=True,real_total=real['test'],clean_total=clean['test'],translation_units=601,missing_module_controls=295,complete_new_functions=23,report=report,sources=real['sources'],families={f:read(HERE/f/'validation.json') for f in FAMILIES},corrections={f:read(HERE/f/'validation.json') for f in ('source-normalization','player-binding')}));print(json.dumps(report))
if __name__=='__main__':main()
