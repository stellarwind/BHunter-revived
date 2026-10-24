"""Verify complete software-table owner against preserved original bytes."""
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
path=FOLDER/'original/0046e600.tsv';rows=list(csv.DictReader(path.open(),delimiter='\t'));at=0x46e600;body=bytearray()
for row in rows:
 data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
assert len(rows)==1122 and at==0x46f481
assert [r['instruction'] for r in rows if r['instruction'].startswith('CALL')]==['CALL 0x0047dae0','CALL 0x0044e9b0','CALL 0x0047dae0','CALL 0x0047dae0','CALL 0x0047dae0']
role=json.loads((ROOT/'META/m1/role-catalog.json').read_text());record=next(r for r in role['functions'] if r['address']=='0047dae0') if 'functions' in role else next(r for r in role['records'] if r['address']=='0047dae0')
assert record['role']=='library_verified' and record['behavior']=='malloc wrapper calling __nh_malloc'
result=json.loads((FOLDER/'focused-build.json').read_text());assert result['passed'] and result['test']['checks']==2228273 and not result['test']['failures']
for name,value in result['sources'].items():assert sha(ROOT/name)==value
record=dict(address='0046e600',symbol='bh_software_tables_initialize',role='engine_candidate',provenance_confidence='medium',classification='Complete ordinary software renderer table owner called by0045c540. Integer palette/blend initialization, unknown proprietary engine authorship; not rasterkernel, import, CRT/compiler support or shared tail.',original=path.relative_to(ROOT).as_posix(),boundary='0046e600-0046f480',instructions=len(rows),bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest(),implementation='func',integration='focused_linked_harness; parent integration pending')
review=dict(records=[record],target=result['target'],source_sha256={name:sha(ROOT/name) for name in ('src/bh_software_tables.c','src/bh_software_tables.h','tests/software_tables.c','src/bh_texture_tables.c')},dependencies=[dict(original='0044e9b0',symbol='bh_texture_tables_init',source='src/bh_texture_tables.c'),dict(original='0047dae0',role='library_verified',binding='real allocator, default CRT malloc implementation in focused fixtures',evidence='META/m1/role-catalog.json; META/m1/review/0047dae0.asm.txt; excluded from recovery count'),dict(original='004e38c4',binding='explicit bytes_per_pixel1/2'),dict(original='004e38c0/004e3890/004e3894;004e32a4/004e3260/004e3264',binding='BHTextureContext.display_right/display_left RGB'),dict(original='004e3424/004e3444/004e3448',binding='explicit RGB masks'),dict(original='004e3858/004e2df4/004e2de4/004e38c8',binding='owning shade_indexed/blend_indexed/shade_native/blend_native')],focused_checks=result['test'],original_code_executed=False,game_launches=0,limits=['Source guards reject malformed/noncontiguous/overlapping/out-of16bit masks and missing3/3/2bits instead of original potentially infinite division loops','Pixel byte counts other than1/2 explicitly rejected; original treats any non1 value as native16 branch','Allocation failure and repeated live-owner initialization safely reject; original did not check allocation or release overwritten pointers','dispose is explicit source lifetime adapter; no original destructor claim','No original execution/differential or renderer/gameplay claim; complete integer table behavior tested exhaustively on indexed8/565/555'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',newline='\n')
print(json.dumps(dict(passed=True,record=record,focused_checks=result['test']['checks'])))
