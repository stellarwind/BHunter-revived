"""Audit source scene orchestration and bindings, without executing original code."""
import csv
import hashlib
import json
import struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FOLDER=ROOT/'META/m4/scene-owner'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def pixels(p):
 data=p.read_bytes();assert data[:2]==b'BM'
 offset=struct.unpack_from('<I',data,10)[0];width,height=struct.unpack_from('<ii',data,18)
 bits=struct.unpack_from('<H',data,28)[0];assert bits in (24,32) and width>0
 size=bits//8;stride=(width*size+3)&~3
 return sum(any(data[offset+y*stride+x*size:offset+y*stride+x*size+3]) for y in range(abs(height)) for x in range(width))
def main():
 v=read(FOLDER/'validation.json');real=read(FOLDER/'integration-real.json')
 cr=read(FOLDER/'clean/clean-checkout.json');clean=cr['result']
 assert v['passed'] and real['passed'] and clean['passed']
 assert not cr['original_assets_present'] and not cr['compiler_in_checkout']
 for name,digest in v['sources'].items():
  assert sha(ROOT/name)==digest,name
  assert sha(ROOT/v['source_snapshots'][name])==digest,name
  for b in (real,clean):
   if name in b['sources']:assert b['sources'][name]==digest,name
 for b,key in ((real,'real'),(clean,'clean')):
  assert b['test']==v[key+'_total'] and not b['test']['failures'] and not b['original_code_required']
  assert sum(c['label'].startswith('compile-') for c in b['commands'])==v['translation_units']
  controls=[val for name,val in b.items() if name.startswith('missing_')]
  assert len(controls)==v['missing_module_controls'] and all('LNK2001' in val for val in controls)
  for scenario,expected in v[key+'_focused'].items():
   assert next(r for r in b['scenarios'] if r.get('scenario')==scenario)==expected
 o=read(FOLDER/'owner.json');rows=list(csv.DictReader((ROOT/o['original']).open(),delimiter='\t'))
 at=int(o['address'],16);body=bytearray()
 for row in rows:
  assert int(row['address'],16)==at
  d=bytes.fromhex(row['bytes']);body.extend(d);at+=len(d)
 assert len(body)==o['bytes'] and len(rows)==o['instructions'] and hashlib.sha256(body).hexdigest()==o['body_sha256']
 exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
 table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
 sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
 def original(va,size):
  rva=va-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
  return exe[offset+rva-start:offset+rva-start+size]
 assert original(int(o['address'],16),len(body))==body
 assert original(0x489440,8)==struct.pack('<d',0.017453292519943295)
 assert original(0x489288,4)==struct.pack('<f',.5)
 assert original(0x48c030,48)==struct.pack('<12f',1,0,0,0,0,1,0,0,0,0,1,0)
 assert original(0x48c080,12)==struct.pack('<3f',0,0,1)
 ledger=read(ROOT/'META/function-status.json')
 for record_id in [o['address']+'-functional-c']+v['updated_variants']:
  r=next(r for r in ledger['records'] if r['id']==record_id)
  assert r['implementation']=='func' and r['integration']=='linked_harness' and not r['game_admitted']
  assert r['source_sha256']==sha(ROOT/r['source']) and not r['validation']['raw_byte_match']
  for path in ('link-map.txt','clean/link-map.txt'):assert ('_'+r['function']).encode() in (FOLDER/path).read_bytes()
 for record_id in v['historical_variants']:
  r=next(r for r in ledger['records'] if r['id']==record_id)
  assert r['historical_variant'] and sha(ROOT/r['source'])==r['source_sha256']
 a=next(r for r in ledger['support_implementations'] if r['id']=='bh_scene_bindings-adapter')
 assert a['implementation']=='adapter' and a['source_sha256']==sha(ROOT/a['source'])
 for name,digest in v['native_images'].items():assert sha(FOLDER/name)==digest
 assert v['native_images']['scene-owner-native-first.bmp']==v['native_images']['scene-owner-native-cached.bmp']==v['native_images']['scene-owner-native-after-gate.bmp']
 assert v['native_images']['scene-owner-native-first.bmp']!=v['native_images']['scene-owner-native-second.bmp']
 assert pixels(FOLDER/'scene-owner-native-first.bmp')>0 and pixels(FOLDER/'scene-owner-native-second.bmp')>0
 assert pixels(FOLDER/'scene-owner-native-blank.bmp')==0
 for row in read(ROOT/'META/reference-manifest.json')['files']:
  if row['root']=='repo':assert sha(ROOT/row['path'])==row['sha256'],row['path']
 assert not v['original_code_executed'] and not v['game_launches']
 print(json.dumps(dict(passed=True,complete_new_functions=1,updated_source_variants=len(v['updated_variants']),real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],native_checks=v['real_focused']['scene-owner-native']['checks'],game_launches=0)))
if __name__=='__main__':main()
