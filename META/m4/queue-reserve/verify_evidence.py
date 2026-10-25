"""Verify complete original owners, external jump-table data and linked variant."""
import hashlib,json,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
b=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(b).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
p=struct.unpack_from('<I',b,60)[0];t=p+24+struct.unpack_from('<H',b,p+20)[0];sections=[struct.unpack_from('<IIII',b,t+40*i+8) for i in range(struct.unpack_from('<H',b,p+6)[0])]
def raw(a,n):
 r=a-0x400000;_,v,_,o=next(s for s in sections if s[1]<=r and r+n<=s[1]+s[2]);return b[o+r-v:o+r-v+n]
owners=[]
for a,size,symbol in [('00477530',155,'bh_queue_reserve'),('00477360',22,'bh_queue_mode_execute')]:
 rows=json.loads((here/'original'/(a+'.json')).read_text());at=int(a,16);body=bytearray()
 for r in rows:
  assert int(r['address'],16)==at;code=bytes.fromhex(r['bytes']);assert code==raw(at,len(code));at+=len(code);body.extend(code)
 assert len(body)==size and rows[-1]['mnemonic']=='RET'
 owners.append(dict(address=a,end_inclusive=f'{at-1:08x}',bytes=size,instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',implementation='func',source='src/bh_queue_reserve.c',symbol=symbol,integration='focused real TU linked harness; parent integration admission required',provenance='ordinary queue reservation/render callback; engine attribution unknown, not CRT/compiler/EH/import/shared tail'))
table=json.loads((here/'jump-table.json').read_text());assert bytes.fromhex(table['bytes'])==raw(0x4775cc,36)
assert table['targets']==['00477559','00477560','00477567','0047756e','00477575','00477555','0047757c','00477583','0047758a']
d=json.loads((here/'validation.json').read_text());assert d['passed']
for path,digest in d['source_sha256'].items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,path
evidence=dict(schema_version=1,owners=owners,jump_table=table,header_mapping=[1,2,3,4,5,0,6,7,8],default_header=0,source_sha256=d['source_sha256'],focused_output=d['commands'][-1]['stdout'],adapter=dict(symbol='bh_queue_mode',source='src/bh_queue_reserve.c',implementation='adapter',original_function_claim=False),original_code_executed=False,game_launches=0,limits=['bounded source queue representation; malformed original pointer behavior not claimed','not a game/render worker implementation','parent integration and clean-check admission separately required','no instruction matching'])
(here/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n');print(json.dumps(dict(complete_owners=2,verified_instructions=sum(x['instructions'] for x in owners),external_jump_table_bytes=36,focused_output=d['commands'][-1]['stdout'])))
