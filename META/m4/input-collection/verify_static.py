"""Pin collector instructions, referenced floating widths and evaluator pairs."""
import hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();sha=hashlib.sha256(exe).hexdigest()
assert sha=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,size):
 rva=va-0x400000
 _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
p=subprocess.run(['ghidra','disasm','0x004744f0','-n','4421','--project','Bhunter','--program','BHUNTER.EXE','--projects-dir',(root/'ghidra-projects').as_posix()],capture_output=True,text=True,check=True)
rows=[r for r in json.loads(p.stdout) if 0x4744f0<=int(r['address'],16)<0x475635]
cursor=0x4744f0;body=bytearray();lines=['address\tbytes\tinstruction'];constants={}
for r in rows:
 assert int(r['address'],16)==cursor
 b=bytes.fromhex(r['bytes']);assert raw(cursor,len(b))==b;body+=b;cursor+=len(b)
 lines.append(r['address']+'\t'+r['bytes']+'\t'+r['mnemonic']+' '+','.join(r['operands']))
 for operand in r['operands']:
  m=re.search(r'(float|double) ptr \[0x(0048(?:9ff[048]|a0[0-5][0-9a-f]))\]',operand)
  if not m:continue
  kind,address=m.groups();size=4 if kind=='float' else 8;value=raw(int(address,16),size)
  key=address+'-'+kind
  entry=constants.setdefault(key,dict(address=address,kind=kind,width=size,bytes=value.hex(),value=struct.unpack('<f' if size==4 else '<d',value)[0],references=[]))
  entry['references'].append(r['address'])
assert cursor==0x475635 and len(body)==4421 and len(rows)==1107
(here/'original/004744f0.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
(here/'original/004744f0.tsv').write_text('\n'.join(lines)+'\n')
(here/'constants.json').write_text(json.dumps(constants,indent=2,sort_keys=True)+'\n')
owner=dict(address='004744f0',end='00475634',bytes=len(body),instructions=len(rows),sha256=hashlib.sha256(body).hexdigest(),original_sha256=sha,role='engine_candidate',provenance='input collection owner; unidentified engine provenance; normal RET-ended body with WinMM/User32 imports',last_instruction=rows[-1],original_code_executed=False)
(here/'owner.json').write_text(json.dumps(owner,indent=2)+'\n')
print(json.dumps(dict(owner=owner,constants=constants),indent=2))
