import json,re,struct,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[3]
here=Path(__file__).resolve().parent
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes()
pe=struct.unpack_from('<I',exe,60)[0]
table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,size):
 rva=va-0x400000
 _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
 return exe[offset+rva-start:offset+rva-start+size]
owners=[]
for file in sorted((here/'original').glob('*.disasm.json')):
 rows=json.loads(file.read_text(encoding='utf-8-sig'));at=int(rows[0]['address'],16);body=bytearray()
 for row in rows:
  assert int(row['address'],16)==at
  b=bytes.fromhex(row['bytes']);body.extend(b);at+=len(b)
 assert raw(int(rows[0]['address'],16),len(body))==body
 owners.append(dict(address=rows[0]['address'],end=f'{at-1:08x}',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),last_instruction=rows[-1]['mnemonic']))
constants={}
rows=json.loads((here/'original/0041c020.disasm.json').read_text(encoding='utf-8-sig'))
for row in rows:
 for operand in row['operands']:
  match=re.search(r'(float|double) ptr \[0x(00489[0-9a-f]+)\]',operand)
  if match:
   kind,address=match.groups();size=4 if kind=='float' else 8;b=raw(int(address,16),size)
   constants[address]=dict(kind=kind,bytes=b.hex(),value=struct.unpack('<f' if size==4 else '<d',b)[0])
(here/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
(here/'background-constants.json').write_text(json.dumps(constants,indent=2,sort_keys=True)+'\n')
strings={}
for address in ('004910e4','004910d8','00492b4c','00492b3c'):
 b=raw(int(address,16),80).split(b'\0',1)[0]
 strings[address]=dict(bytes=b.hex(),value=b.decode('ascii'))
(here/'resource-strings.json').write_text(json.dumps(strings,indent=2)+'\n')
print(json.dumps(dict(owners=owners,constants=constants,resource_strings=strings),indent=2))
