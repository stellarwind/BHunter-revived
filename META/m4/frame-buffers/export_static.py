"""Pin complete triple-buffer owners and timing literals; read-only original."""
import hashlib,json,re,struct,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,size):
 rva=va-0x400000
 _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
def query(*args):
 p=subprocess.run(['ghidra',*args,'--project','Bhunter','--program','BHUNTER.EXE','--projects-dir',(root/'ghidra-projects').as_posix()],capture_output=True,text=True,check=True)
 return json.loads(p.stdout)
owners=[];constants={}
for address,size,count in [('00471c00',183,36),('00471cc0',168,50),('00471d70',119,32)]:
 start=int(address,16);rows=[r for r in query('disasm','0x'+address,'-n',str(size)) if start<=int(r['address'],16)<start+size]
 cursor=start;body=bytearray();lines=['address\tbytes\tinstruction']
 for r in rows:
  assert int(r['address'],16)==cursor
  b=bytes.fromhex(r['bytes']);assert raw(cursor,len(b))==b;body+=b;cursor+=len(b)
  lines.append(r['address']+'\t'+r['bytes']+'\t'+r['mnemonic']+' '+','.join(r['operands']))
  for operand in r['operands']:
   m=re.search(r'(float|double) ptr \[0x(00489f(?:c8|d0|d8))\]',operand)
   if m:
    kind,va=m.groups();width=4 if kind=='float' else 8;b=raw(int(va,16),width)
    e=constants.setdefault(va,dict(kind=kind,width=width,bytes=b.hex(),value=struct.unpack('<f' if width==4 else '<d',b)[0],references=[]));e['references'].append(r['address'])
 assert cursor==start+size and len(rows)==count
 (here/'original'/f'{address}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
 (here/'original'/f'{address}.tsv').write_text('\n'.join(lines)+'\n')
 (here/'original'/f'{address}.decompile.json').write_text(json.dumps(query('decompile','0x'+address),indent=2)+'\n')
 owners.append(dict(address=address,end=f'{cursor-1:08x}',bytes=size,instructions=count,sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance='ordinary triple-buffer owner, engine authorship unresolved; not verified CRT/EH/import/shared tail',last_instruction=rows[-1]))
rows=query('disasm','0x004798bf','-n','8');rows=[r for r in rows if int(r['address'],16)<=0x4798e4]
for r in rows:assert raw(int(r['address'],16),len(r['bytes'])//2)==bytes.fromhex(r['bytes'])
(here/'original/caller-004798bf.json').write_text(json.dumps(rows,indent=2)+'\n')
(here/'owner.json').write_text(json.dumps(owners,indent=2)+'\n')
(here/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
print(json.dumps(dict(owners=owners,constants=constants),indent=2))
