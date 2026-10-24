"""Complete immutable-byte and focused-source audit for cached atlas owner."""
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
path=FOLDER/'original/0046f490.tsv';rows=list(csv.DictReader(path.open(),delimiter='\t'));at=0x46f490;body=bytearray()
for row in rows:
 data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data;body.extend(data);at+=len(data)
assert len(rows)==1087 and at==0x470372
assert set(r['instruction'] for r in rows if r['instruction'].startswith('CALL 0x'))=={'CALL 0x0044f1c0','CALL 0x0047cbd0','CALL 0x0047ce50','CALL 0x0047cec0','CALL 0x0047cf60','CALL 0x0047dae0','CALL 0x0047db90','CALL 0x004882e0'}
result=json.loads((FOLDER/'focused-build.json').read_text());assert result['passed'] and result['test']['checks']==3439 and not result['test']['failures']
for name,value in result['sources'].items():assert sha(ROOT/name)==value
record=dict(address='0046f490',symbol='bh_atlas_image_load',role='engine_candidate',provenance_confidence='medium',classification='Complete ordinary native cached atlas/glyph-sheet loader, directly called by00470380. Unknown proprietary engine authorship. Not software raster kernel, CRT/compiler helper, import or shared tail.',original=path.relative_to(ROOT).as_posix(),boundary='0046f490-00470371',instructions=len(rows),bytes=len(body),body_sha256=hashlib.sha256(body).hexdigest(),implementation='func',integration='focused_linked_harness; parent integration pending')
review=dict(records=[record],target=result['target'],source_sha256=result['sources'],focused_checks=result['test'],dependencies=[dict(original='0047cbd0/0047ce50/0047cec0/0047cf60',binding='real admitted BHVFile through BHPcxReader'),dict(original='inline0044efa0-equivalent RLE/header0044f0a0 contract',binding='real bh_pcx_reader_open/next/close in src/bh_pcx_reader.c'),dict(original='0044f1c0',binding='real bh_pcx_palette in src/bh_pcx.c'),dict(original='0047dae0/0047db90/004882e0',role='library_verified',binding='real allocator/free and case-insensitive CRT comparison; excluded from recovered count'),dict(original='004e9c44',binding='BHTextureContext.draw real IDirectDraw'),dict(original='00491750/00491754',binding='BHTextureContext.palette_mode/palette_color'),dict(original='004ededc',binding='BHAtlasContext.images'),dict(original='004e9750',binding='BHAtlasContext.scan_step, explicit active-screen pitch; deliberately independent of atlas pitch')],original_code_executed=False,game_launches=0,limits=['Original publishes owner before decode and can retain partially initialized cache entry after failed Lock; source failure is transactional and cleans resources instead','Source guards max4096 dimensions, max127 name bytes, standard contiguous/disjoint8/16/24/32formats with RGB bit31 rejected, max512rectangles and scan reads within declared index storage','Original independent scan step preserved including0; arbitrary nonmatching step can read native padding as original, meaningful-content fixtures bind matching pitch or0','No original destructor claim: explicit source context dispose releases actual surfaces and owned cache records','No source game/font/page caller integration or instruction match claimed; focused target links current real m0 dependency objects and parent rebuild remains required'])
(FOLDER/'review.json').write_text(json.dumps(review,indent=2)+'\n',newline='\n')
print(json.dumps(dict(passed=True,record=record,focused_checks=result['test']['checks'])))
