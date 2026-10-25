"""Read-only Ghidra export and immutable PE byte verification."""
import hashlib,json,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
out=here/'original';out.mkdir(exist_ok=True)
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
 rva=va-0x400000
 _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+n]
def query(*args):
 p=subprocess.run(['ghidra',*args,'--project','Bhunter','--program','BHUNTER.EXE','--projects-dir',(root/'ghidra-projects').as_posix()],capture_output=True,text=True,check=True)
 return json.loads(p.stdout)
owners=[]
for address in ['00478060','00477fa0','0044f510','00478730','00477760','00457590','00478930','00477f20','0045cf30']:
 info=query('function','get','0x'+address)[0];start=int(address,16);size=info['size']
 rows=query('disasm','0x'+address,'-n',str(size));rows=[r for r in rows if start<=int(r['address'],16)<start+size]
 cursor=start;body=bytearray()
 for r in rows:
  assert int(r['address'],16)==cursor
  b=bytes.fromhex(r['bytes']);body.extend(b);cursor+=len(b)
 assert cursor==start+size and raw(start,size)==body
 (out/(address+'.disasm.json')).write_text(json.dumps(rows,indent=2)+'\n')
 (out/(address+'.c.txt')).write_text(query('decompile','0x'+address)[0]['code'].strip()+'\n')
 owners.append(dict(address=address,end=f'{cursor-1:08x}',size=size,instructions=len(rows),sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance='unresolved; render queue/image policy; not verified library or CRT',complete=True))
(here/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
print(json.dumps(owners,indent=2))
