"""Read-only whole-body/reference/source-variant validation."""
import hashlib,json,re,struct
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
b=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(b).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
p=struct.unpack_from('<I',b,60)[0];t=p+24+struct.unpack_from('<H',b,p+20)[0];sections=[struct.unpack_from('<IIII',b,t+40*i+8) for i in range(struct.unpack_from('<H',b,p+6)[0])]
def raw(a,n):
 r=a-0x400000;_,v,_,o=next(s for s in sections if s[1]<=r and r+n<=s[1]+s[2]);return b[o+r-v:o+r-v+n]
owners=[];constants={}
for a,size,symbol in [('00455410',1059,'bh_screen_quad_draw'),('00477680',50,'bh_screen_quad_execute'),('00478e50',227,'bh_screen_quad_queue')]:
 rows=json.loads((here/'original'/(a+'.disasm.json')).read_text());at=int(a,16);body=bytearray()
 for r in rows:
  assert int(r['address'],16)==at;code=bytes.fromhex(r['bytes']);assert raw(at,len(code))==code;body.extend(code);at+=len(code)
  for operand in r['operands']:
   m=re.search(r'(float|double) ptr \[0x(00489[0-9a-f]+)\]',operand)
   if m:
    kind,address=m.groups();n=4 if kind=='float' else 8;data=raw(int(address,16),n);constants[address]=dict(width=n,bytes=data.hex(),value=struct.unpack('<f' if n==4 else '<d',data)[0])
 assert len(body)==size and rows[-1]['mnemonic']=='RET'
 owners.append(dict(address=a,end_inclusive=f'{at-1:08x}',bytes=size,instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance='full screen primitive/queue engine role; engine authorship unresolved; not CRT/compiler/import/shared tail',implementation='func',source='src/bh_screen_quad.c',symbol=symbol,integration='focused real-TU linked harness; parent integration separately required'))
d=json.loads((here/'validation.json').read_text());assert d['passed']
for path,digest in d['source_sha256'].items():assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,path
evidence=dict(schema_version=1,owners=owners,constants=constants,source_sha256=d['source_sha256'],focused_output=d['commands'][-1]['stdout'],original_code_executed=False,game_launches=0,contract='complete screen full-texture owner and producer/consumer with explicit primitive emitter; native source binding tested, software dispatcher still unfinished',live_near_z='00451720 writes0049175c argument2; screen depth is near_z minus double -0.01, NOT constant1 or far_z',limitations=['no instruction matching','PC24/huge-angle FSIN/FCOS not established','software dispatch/raster backend remains unfinished','no full HUD/game target admission'])
(here/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n');print(json.dumps(dict(complete_owners=len(owners),verified_instructions=sum(x['instructions'] for x in owners),focused_output=d['commands'][-1]['stdout'])))
