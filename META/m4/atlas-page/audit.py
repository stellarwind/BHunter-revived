"""Verify complete page constructor original instructions and source evidence."""
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
path=FOLDER/'original/00470380.tsv';rows=list(csv.DictReader(path.open(),delimiter='\t'));at=0x470380;body=bytearray()
for row in rows:
 data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
assert len(rows)==196 and at==0x4705b9 and original(0x491870,11)==b'@@@PAGE@@@\0'
assert set(r['instruction'] for r in rows if r['instruction'].startswith('CALL 0x'))=={'CALL 0x0044e580','CALL 0x0044eb60','CALL 0x0046f490','CALL 0x0047dae0','CALL 0x0047db90'}
result=json.loads((FOLDER/'focused-build.json').read_text());assert result['passed'] and result['test']['checks']==135 and not result['test']['failures']
for name,value in result['sources'].items():assert sha(ROOT/name)==value
record=dict(address='00470380',symbol='bh_atlas_page_create',role='engine_candidate',provenance_confidence='medium',classification='Complete ordinary atlas text-page backing constructor called by0042c4c0/004474c0. Unknown proprietary engine authorship; not import/CRT/compiler helper/shared tail.',original=path.relative_to(ROOT).as_posix(),boundary='00470380-004705b8',instructions=len(rows),bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest(),implementation='func',integration='focused_linked_harness; parent integration pending')
review=dict(records=[record],target=result['target'],source_sha256=result['sources'],focused_checks=result['test'],dependencies=[dict(original='0044e580',symbol='bh_texture_request_format',source='src/bh_texture.c'),dict(original='0044eb60',symbol='bh_texture_palette_tables',source='src/bh_texture_tables.c'),dict(original='0046f490',symbol='bh_atlas_image_load',source='src/bh_atlas_image.c'),dict(original='0047dae0/0047db90',role='library_verified',binding='real caller allocator and CRT malloc/free, excluded from recovery count'),dict(original='004e9c44/004edc70',binding='BHTextureContext.draw/images real native/owned image bindings')],original_code_executed=False,game_launches=0,limits=['Constructor creates blank native texture backing only; glyph copying/UV generation/text drawing and font caller ownership remain separate','Original page packed_color+10 and image padding are undefined until later; source initializes deterministically0 without claiming original value','Source allocation/native HRESULT failure checks clean partial page ownership; original unchecked palette/arena failures and leaked0x28record on surface failure not reproduced','Original format request and borrowed atlas-cache side effects remain after later page failure; source dispose is an explicit lifetime adapter, not recovered destructor','No original function byte match, original execution or game/renderer integration claim; real downstream residency/GetHandle fixture is separate from constructor behavior'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',newline='\n')
print(json.dumps(dict(passed=True,record=record,focused_checks=result['test']['checks'])))
