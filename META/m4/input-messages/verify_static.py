"""Verify complete instruction bytes and jump-table data against immutable PE."""
import hashlib,json,struct
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
rows=json.loads((here/'original/00473c40.disasm.json').read_text());cursor=0x473c40;body=bytearray();tsv=['address\tbytes\tinstruction']
returns=[]
for row in rows:
 assert int(row['address'],16)==cursor
 b=bytes.fromhex(row['bytes']);assert raw(cursor,len(b))==b;body+=b;cursor+=len(b)
 tsv.append(row['address']+'\t'+row['bytes']+'\t'+row['mnemonic']+' '+','.join(row['operands']))
 if row['mnemonic']=='RET':returns.append(dict(address=row['address'],bytes=row['bytes']));assert b==b'\xc3'
assert cursor==0x473d25 and len(body)==229 and len(rows)==60
tables={}
for va,count in [(0x473d28,6),(0x473d40,7)]:
 b=raw(va,count*4);dest=struct.unpack('<'+'I'*count,b)
 assert all(0x473c40<=p<0x473d25 for p in dest)
 tables[f'{va:08x}']=dict(bytes=b.hex(),destinations=[f'{p:08x}' for p in dest])
owner=dict(address='00473c40',end='00473d24',bytes=len(body),instructions=len(rows),body_sha256=hashlib.sha256(body).hexdigest(),original_sha256=sha,returns=returns,abi='cdecl four arguments; every return C3, no callee pop',role='engine_candidate',provenance='input message processor; engine attribution unresolved; no verified library/CRT match',padding=dict(address='00473d25',bytes=raw(0x473d25,3).hex()),jump_tables=tables,original_code_executed=False)
(here/'original/00473c40.tsv').write_text('\n'.join(tsv)+'\n')
(here/'owner.json').write_text(json.dumps(owner,indent=2)+'\n')
print(json.dumps(owner,indent=2))
