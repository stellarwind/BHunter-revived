"""Pin complete original owners, not source byte-match evidence."""
import csv,hashlib,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),'--project','Bhunter','--program','BHUNTER.EXE']
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
 rva=va-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[offset+rva-start:offset+rva-start+n]
mapping={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
owners=[]
for address,function,source in [('00457590','bh_sprite_quad_draw','bh_sprite_quad'),('00478730','bh_quad_queue','bh_quad_queue'),('00477760','bh_quad_queue_execute','bh_quad_queue')]:
 m=mapping[address];path=HERE/'original'/f'{address}.disasm.json'
 rows=json.loads(subprocess.check_output(base+['disasm','0x'+address,'-n',m['instructions']],text=True))
 path.write_text(json.dumps(rows,indent=2)+'\n');at=int(address,16);body=bytearray()
 for r in rows:
  assert int(r['address'],16)==at;b=bytes.fromhex(r['bytes']);body.extend(b);at+=len(b)
 assert len(rows)==int(m['instructions']) and len(body)==int(m['body_bytes']) and body==raw(int(address,16),len(body)) and rows[-1]['mnemonic']=='RET'
 with (HERE/'original'/f'{address}.tsv').open('w',newline='') as f:
  writer=csv.writer(f,delimiter='\t');writer.writerow(['address','bytes','mnemonic','operands']);writer.writerows((r['address'],r['bytes'],r['mnemonic'],','.join(r['operands'])) for r in rows)
 for command,name in [('decompile','decompile'),('function xrefs','xrefs'),('function get','function')]:
  out=subprocess.check_output(base+command.split()+['0x'+address],text=True);(HERE/'original'/f'{address}.{name}.json').write_text(out)
 owners.append(dict(address=address,function=function,source='src/'+source+'.c',role='engine_candidate',bytes=len(body),instructions=len(rows),end=f'{at-1:08x}',body_sha256=hashlib.sha256(body).hexdigest(),original=f'META/m4/sprite-quad/original/{address}.tsv'))
constants={}
for va in [0x489ed0,0x489ef8,0x489ee8,0x489f00,0x489f18]:
 b=raw(va,4);constants[f'{va:08x}']=dict(bytes=b.hex(),value=struct.unpack('<f',b)[0])
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n');(HERE/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
print(json.dumps(dict(owners=len(owners),immutable_bytes_verified=True,original_code_executed=False)))
