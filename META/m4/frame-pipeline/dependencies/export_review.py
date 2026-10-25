"""Read-only complete-body and constant evidence for display dependencies."""
import csv,hashlib,json,struct,subprocess,re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),'--project','Bhunter','--program','BHUNTER.EXE']
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
 rva=va-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[offset+rva-start:offset+rva-start+n]
mapping={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
owners=[]
for address,symbol,source in [('0045b810','bh_capture_software','src/bh_capture.c'),('0045bc40','bh_capture_write','src/bh_capture.c'),('0045cdf0','bh_present_begin','src/bh_present_begin.c'),('0045d750','bh_display_release','src/bh_display_shutdown.c'),('004738a0','bh_display_shutdown','src/bh_display_shutdown.c')]:
 m=mapping[address];rows=json.loads(subprocess.check_output(base+['disasm','0x'+address,'-n',m['instructions']],text=True))
 (HERE/f'{address}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
 at=int(address,16);body=bytearray()
 for r in rows:
  assert int(r['address'],16)==at;b=bytes.fromhex(r['bytes']);body.extend(b);at+=len(b)
 assert len(body)==int(m['body_bytes']) and body==raw(int(address,16),len(body)) and rows[-1]['mnemonic']=='RET'
 with (HERE/f'{address}.tsv').open('w',newline='') as f:
  writer=csv.writer(f,delimiter='\t');writer.writerow(['address','bytes','mnemonic','operands']);writer.writerows((r['address'],r['bytes'],r['mnemonic'],','.join(r['operands'])) for r in rows)
 for command,name in [('decompile','decompile'),('function xrefs','xrefs'),('function get','function')]:
  (HERE/f'{address}.{name}.json').write_text(subprocess.check_output(base+command.split()+['0x'+address],text=True))
 owners.append(dict(address=address,symbol=symbol,source=source,role='engine_candidate',provenance='ordinary complete display/capture owner with application renderer globals; historical engine attribution unresolved; no import/CRT/compiler/shared-tail signature',bytes=len(body),instructions=len(rows),end=f'{at-1:08x}',body_sha256=hashlib.sha256(body).hexdigest()))
constants={}
accesses={}
for r in rows:
 for op in r['operands']:
  match=re.search(r'(float|double) ptr \[0x(00489[0-9a-f]+)\]',op)
  if match:accesses[int(match[2],16)]=8 if match[1]=='double' else 4
for va,size in sorted(accesses.items()):
 b=raw(va,size);constants[f'{va:08x}']=dict(bytes=b.hex(),value=struct.unpack('<d' if size==8 else '<f',b)[0])
constants['00491790']=dict(bytes=raw(0x491790,54).hex(),meaning='Original fixed BMP header; writers alter only width/height low two bytes, retain fixed size fields and omit padding')
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n');(HERE/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
print(json.dumps(dict(owners=owners,constants=constants,immutable_bytes_verified=True,original_code_executed=False)))
