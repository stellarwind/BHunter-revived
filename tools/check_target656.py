"""Verify the completed606+50 block against real and independent clean builds."""
import hashlib,json,re
from pathlib import Path
from record_pickup_basic import ROOT,reference,read,write,sha,active,native_dependencies
from check_remaining129 import verify_runtime
HERE=ROOT/'META/m4/target656'
FAMILIES=['raster','primitive','editor','response','construct','launch','vm','parse','blocks']
COUNTS=[28,4,3,2,2,2,2,4,3]
CHECKS=[206692,1086,44,32,680,20,194,52,37]
def main():
 ledger,raw=reference();real=read(ROOT/'build/m0/result.json');clean_report=read(HERE/'clean/clean-checkout.json');clean=clean_report['result']
 assert real['passed'] and clean['passed'] and not clean_report['exit_code']
 assert not clean_report['original_assets_present'] and not clean_report['compiler_in_checkout']
 assert real['sources']==clean['sources'];native_dependencies(real,clean)
 current=active(ledger);assert len(current)==len({r['original_va'] for r in current})==656
 for row in current:assert row['source_sha256']==real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
 owners={};running=606;map_text=(ROOT/'build/m0/bhunter-integration.map').read_text(errors='replace');clean_map=(HERE/'clean/link-map.txt').read_text(errors='replace')
 for family,count,checks in zip(FAMILIES,COUNTS,CHECKS):
  folder=HERE/family;spec=read(folder/'admission.json');validation=read(folder/'validation.json');family_owners=read(folder/'owners.json')
  assert spec['baseline']==running and validation['baseline_functions']==running and len(family_owners)==count==validation['new_owners']
  running+=count;assert validation['active_original_functions']==running
  for name,digest in validation['sources'].items():assert sha(ROOT/validation['source_snapshots'][name])==digest
  for original in family_owners:
   va=original['original_va'];assert va not in owners;owners[va]=original
   row=next(r for r in current if r['original_va']==va);body=raw(int(va,16),original['bytes'])
   instructions=read(folder/'original'/(va+'.disasm.json'));assert b''.join(bytes.fromhex(x['bytes']) for x in instructions)==body and instructions[-1]['mnemonic']=='RET'
   boundary=row['validation']['boundary'];assert boundary['start']==va and boundary['bytes']==len(body) and int(boundary['end_exclusive'],16)==int(va,16)+len(body)
   assert boundary['sha256']==hashlib.sha256(body).hexdigest()
   assert row['implementation']=='func' and not row['validation']['raw_byte_match'] and not row['validation']['original_code_executed'] and not row['game_admitted']
   for text in (map_text,clean_map):assert re.search(r'(?<!\w)_'+re.escape(original['function'])+r'(?=\s|$)',text),original['function']
   latest=read(ROOT/row['evidence'][0]);assert sha(ROOT/latest['source_snapshots'][row['source']])==row['source_sha256']
  for build in (real,clean):
   fixture=next(x for x in build['scenarios'] if x.get('scenario')==spec['scenario']);assert fixture['checks']==checks and not fixture['failures']
   assert build['missing_'+spec['module']+'_control']=='rejected with LNK2001'
 assert running==656 and len(owners)==50
 for build,total in ((real,5527805),(clean,5021857)):
  assert not build['original_code_required'] and build['test']['checks']==total and not build['test']['failures']
  assert len([x for x in build['commands'] if x['label'].startswith('compile-')])==568
  controls=[v for k,v in build.items() if k.startswith('missing_')];assert len(controls)==278 and all('LNK2001' in v for v in controls)
 for item in read(HERE/'constants.json'):assert raw(int(item['address'],16),item['bytes']).hex()==item['hex']
 table=read(HERE/'vm/original/switch.json');assert hashlib.sha256(raw(int(table['address'],16),table['bytes'])).hexdigest()==table['sha256'] and table['count']==0
 variants=read(HERE/'primitive/refreshed-variants.json');assert len(variants)==28
 for item in variants:
  old=next(r for r in ledger['records'] if r['id']==item['old']);new=next(r for r in current if r['id']==item['new']);assert old['historical_variant'] and old['validation']['superseded_by']==new['id'];assert sha(ROOT/item['snapshot'])==old['source_sha256']
 capture=read(HERE/'capture/provenance.json');assert sha(ROOT/capture['image'])==capture['image_sha256'] and sha(ROOT/capture['tool'])==capture['tool_sha256'];assert not capture['game_launches'] and not capture['original_code_executed']
 exclusions=verify_runtime(raw);assert exclusions==50
 previous=read(ROOT/'META/m4/remaining129/remaining.json');remaining=[r for r in previous if r['address'] not in owners];assert len(remaining)==24
 write(HERE/'remaining-catalog.json',remaining)
 report=dict(passed=True,baseline_original_owners=606,new_whole_source_owners=50,admitted_original_owners=656,translation_units=568,missing_module_controls=278,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],compiled_source_inputs_identical=True,active_source_hashes_verified=656,original_owner_bytes_verified=50,typed_constants_verified=len(read(HERE/'constants.json')),historical_raster_variants_preserved=28,runtime_or_import_exclusions_separate=exclusions,prior_catalogue_entries_remaining=24,whole_vm_previously_outside_catalogue='00443ff0',reference_files_unchanged=True,game_launches=0,original_code_executed=False,automated_gameplay=False,delegated=False,requested50_complete=True,M3='skipped',M4='incomplete')
 write(HERE/'final-audit.json',report);write(HERE/'validation.json',dict(report=report,sources=real['sources'],families={f:read(HERE/f/'validation.json') for f in FAMILIES}));print(json.dumps(report))
if __name__=='__main__':main()
