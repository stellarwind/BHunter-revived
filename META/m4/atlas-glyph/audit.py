"""Complete instruction and native focused evidence audit for clear/cursor."""
import csv,hashlib,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];FOLDER=Path(__file__).resolve().parent
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def original(address,size):
 rva=address-0x400000;_,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
records=[]
for address,symbol,count,end in [(0x4705c0,'bh_atlas_page_clear',292,0x4708ce),(0x4708d0,'bh_atlas_page_cursor',10,0x4708f1)]:
 path=FOLDER/('original/%08x.tsv'%address);rows=list(csv.DictReader(path.open(),delimiter='\t'));at=address;body=bytearray()
 for row in rows:
  data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
 assert len(rows)==count and at==end and not any(r['instruction'].startswith('CALL 0x') for r in rows)
 records.append(dict(address='%08x'%address,symbol=symbol,role='engine_candidate',provenance_confidence='medium',classification='Complete ordinary page clear/color conversion/residency dirty owner or coupled raw cursor control. Unknown proprietary engine authorship; not glyph copy, import, CRT/compiler helper or shared tail.',original=path.relative_to(ROOT).as_posix(),boundary='%08x-%08x'%(address,end-1),instructions=len(rows),bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest(),implementation='func',integration='focused_linked_harness; parent integration pending'))
result=json.loads((FOLDER/'focused-build.json').read_text());assert result['passed'] and result['test']['checks']==10313 and not result['test']['failures']
for name,value in result['sources'].items():assert sha(ROOT/name)==value
native=next(c for c in result['commands'] if c['label']=='native-checks');busy=[json.loads(line) for line in native['stdout'].splitlines() if line.startswith('{') and 'native_busy_hresult' in line]
assert busy==[{'native_busy_hresult':-2005532242,'pixel_bits':8},{'native_busy_hresult':-2005532242,'pixel_bits':16}]
review=dict(records=records,target=result['target'],source_sha256=result['sources'],focused_checks=result['test'],native_busy_control=busy,dependencies=[dict(source='src/bh_atlas_page.c/h',binding='real40bytepage and owning native surface/image'),dict(source='src/bh_atlas_image.c/h',binding='borrowed palette/pixel format from complete atlas owner'),dict(source='src/bh_texture.c',binding='real resident slot pixel dirty flag, preallocation/residency positive controls'),dict(original='SDK vtables',binding='real native IDirectDrawSurface Lock/Unlock; no fake COM')],original_code_executed=False,game_launches=0,limits=['00470900 glyph blits/text wrapping not assigned or implemented; these owners only clear backing and set cursor','Source int/q.error/last_hresult expose native failure ignored by original void clear; invalid bindings/dimensions/formats reject safely','Original packed-color cache sentinel collision on first0xffffffff remains tied to page constructor undefined packed_color field; source constructor deterministic0 is an explicit extension','Native8/16/24/32 actual clears verified on RGB backend; broader native/HAL errors not claimed','Signed bit31 RGB mask conversion edge verified by original atlas format binding applied to actual standard native32 surface, without claiming driver support for creating that mask format'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',newline='\n')
print(json.dumps(dict(passed=True,records=records,focused_checks=result['test']['checks'],native_busy_control=busy)))
