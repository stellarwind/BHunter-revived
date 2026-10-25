import json,struct,hashlib,pathlib
root=pathlib.Path(__file__).resolve().parents[4]
exe=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,0x3c)[0];n=struct.unpack_from('<H',exe,pe+6)[0];opts=struct.unpack_from('<H',exe,pe+20)[0];base=struct.unpack_from('<I',exe,pe+24+28)[0];sections=[]
for i in range(n):
 at=pe+24+opts+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',exe,at+8);sections.append((va,max(vs,rs),rp))
def offset(rva):
 for start,length,at in sections:
  if start<=rva<start+length:return at+rva-start
 raise AssertionError(hex(rva))
def raw(va,size):
 at=offset(va-base);return exe[at:at+size]
result=[]
for address,count,size in [('004713c0',30,85),('00476030',15,56)]:
 rows=json.loads((pathlib.Path(__file__).parent/'original'/(address+'.disasm.json')).read_text(encoding='utf-8-sig'));assert len(rows)==count
 content=b''.join(bytes.fromhex(r['bytes']) for r in rows);assert len(content)==size and content==raw(int(address,16),size)
 result.append(dict(address=address,bytes=size,instructions=count,sha256=hashlib.sha256(content).hexdigest()))
import_rva=struct.unpack_from('<I',exe,pe+24+96+8)[0];at=offset(import_rva);found=None
while True:
 original,timestamp,chain,name,first=struct.unpack_from('<IIIII',exe,at)
 if not (original or name or first):break
 thunk=offset(original or first);i=0
 while True:
  value=struct.unpack_from('<I',exe,thunk+4*i)[0]
  if not value:break
  if base+first+4*i==0x4891c8:
   start=offset(value)+2;found=exe[start:exe.index(0,start)].decode('ascii')
  i+=1
 at+=20
assert found=='CoUninitialize'
output=dict(passed=True,original_sha256=hashlib.sha256(exe).hexdigest(),owners=result,tail_import=dict(address='004891c8',name=found),original_code_executed=False)
(pathlib.Path(__file__).parent/'boundary-validation.json').write_text(json.dumps(output,indent=2)+'\n')
print('PASS: immutable EXE,45instructions141bytes,CoUninitialize IAT4891c8')
