"""Pin init/shutdown and generate exact255-pointer key-name defaults."""
import hashlib,json,struct,subprocess
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
owners=[]
for address,size,count in [('00473d60',627,143),('00473fe0',55,16)]:
 start=int(address,16);rows=[r for r in query('disasm','0x'+address,'-n',str(size)) if start<=int(r['address'],16)<start+size]
 cursor=start;body=bytearray();lines=['address\tbytes\tinstruction']
 for r in rows:
  assert int(r['address'],16)==cursor
  b=bytes.fromhex(r['bytes']);assert raw(cursor,len(b))==b;body+=b;cursor+=len(b)
  lines.append(r['address']+'\t'+r['bytes']+'\t'+r['mnemonic']+' '+','.join(r['operands']))
 assert cursor==start+size and len(rows)==count
 (here/'original'/f'{address}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
 (here/'original'/f'{address}.tsv').write_text('\n'.join(lines)+'\n')
 (here/'original'/f'{address}.decompile.json').write_text(json.dumps(query('decompile','0x'+address),indent=2)+'\n')
 owners.append(dict(address=address,end=f'{cursor-1:08x}',bytes=size,instructions=count,sha256=hashlib.sha256(body).hexdigest(),role='engine_candidate',provenance='ordinary input device/key-name lifetime owner, engine attribution unresolved; not verified CRT/import/compiler helper',last_instruction=rows[-1]))
(here/'device-owners.json').write_text(json.dumps(owners,indent=2)+'\n')
data=raw(0x491d10,255*4);entries=[];lines=['/* Generated from immutable BHUNTER.EXE table00491d10..0049210b. */','#ifndef BH_INPUT_KEY_NAMES_H','#define BH_INPUT_KEY_NAMES_H','#include <stddef.h>','static const char *const bh_input_key_defaults[255]={']
for vk,pointer in enumerate(struct.unpack('<255I',data)):
 if pointer:
  value=bytearray()
  for offset in range(256):
   byte=raw(pointer+offset,1)[0]
   if byte==0:break
   assert byte<128;value.append(byte)
  else:raise AssertionError('unterminated string')
  # Fixed-width octal escapes preserve every byte without hex escape extension.
  literal='"'+''.join('\\%03o'%b for b in value)+'"'
  entry=dict(vk=vk,slot=f'{0x491d10+vk*4:08x}',pointer=f'{pointer:08x}',bytes=bytes(value).hex(),value=bytes(value).decode('ascii'))
 else:literal='NULL';entry=dict(vk=vk,slot=f'{0x491d10+vk*4:08x}',pointer='00000000',bytes=None,value=None)
 lines.append('    '+literal+(', /* VK%u */'%vk))
 entries.append(entry)
lines+=['};','#endif','']
(root/'src/bh_input_key_names.h').write_text('\n'.join(lines))
(here/'key-names.json').write_text(json.dumps(dict(address='00491d10',count=255,bytes=data.hex(),sha256=hashlib.sha256(data).hexdigest(),entries=entries),indent=2)+'\n')
print(json.dumps(dict(owners=owners,keys=len(entries),nonzero=sum(e['pointer']!='00000000' for e in entries)),indent=2))
