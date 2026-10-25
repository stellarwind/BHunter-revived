"""Read-only callback registration frontier audit; no implementation/execution."""
import csv,hashlib,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent/'next-review';HERE.mkdir(exist_ok=True)
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
 rva=va-0x400000;_,start,_,offset=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[offset+rva-start:offset+rva-start+n]
mapping={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
ledger=json.loads((ROOT/'META/function-status.json').read_text())['records']
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),'--project','Bhunter','--program','BHUNTER.EXE']
report=[]
for address,role in [('00449e50','engine_candidate'),('0044a160','game'),('0044f1b0','engine_candidate')]:
 m=mapping[address];rows=json.loads(subprocess.check_output(base+['disasm','0x'+address,'-n',m['instructions']],text=True))
 at=int(address,16);body=bytearray()
 for r in rows:
  assert int(r['address'],16)==at;b=bytes.fromhex(r['bytes']);body.extend(b);at+=len(b)
 assert len(body)==int(m['body_bytes']) and body==raw(int(address,16),len(body)) and rows[-1]['mnemonic']=='RET'
 (HERE/f'{address}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
 decompile=subprocess.run(base+['decompile','0x'+address],capture_output=True,text=True)
 (HERE/f'{address}.decompile.json').write_text(decompile.stdout if decompile.returncode==0 else json.dumps(dict(error=decompile.stderr,exit_code=decompile.returncode,historical_listing='META/m1/review/'+address+'.c.txt'))+'\n')
 report.append(dict(address=address,role=role,classification='ordinary complete owner; no CRT/import/compiler/shared-tail signature; engine provenance unresolved unless application-specific installer',bytes=len(body),instructions=len(rows),end=f'{at-1:08x}',sha256=hashlib.sha256(body).hexdigest(),ledger=[dict(id=r['id'],implementation=r['implementation'],source=r.get('source')) for r in ledger if r.get('original_va')==address],direct_calls=m['calls']))
(HERE/'review.json').write_text(json.dumps(dict(owners=report,original_code_executed=False,source_implementation_created=False),indent=2)+'\n')
print(json.dumps(report))
