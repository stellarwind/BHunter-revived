"""Read-only full player render frontier: exports and immutable-PE body checks."""
from pathlib import Path
import hashlib,json,re,struct,subprocess
ROOT=Path(__file__).resolve().parents[4]; HERE=Path(__file__).resolve().parent
OUT=HERE
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0]; table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
    _,start,_,offset=next(s for s in sections if s[1]<=va-0x400000 and va-0x400000+n<=s[1]+s[2])
    return exe[offset+va-0x400000-start:offset+va-0x400000-start+n]
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),'--project','Bhunter','--program','BHUNTER.EXE']
report=[]
for va,size in [(0x4324a0,106),(0x432510,65),(0x432c60,900)]:
    name=f'{va:08x}'
    meta=json.loads(subprocess.check_output(base+['function','get',hex(va)],text=True))
    assert meta[0]['size']==size
    (OUT/(name+'.function.json')).write_text(json.dumps(meta,indent=2)+'\n')
    dec=json.loads(subprocess.check_output(base+['decompile',hex(va)],text=True))
    (OUT/(name+'.decompile.json')).write_text(json.dumps(dec,indent=2)+'\n')
    rows=json.loads(subprocess.check_output(base+['disasm',hex(va),'-n',str(size)],text=True))
    body=[];at=va;calls=set()
    for row in rows:
        if at>=va+size:break
        b=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and b==raw(at,len(b))
        at+=len(b);body.append(row)
        if row['mnemonic']=='CALL':calls.update(row['operands'])
    assert at==va+size and body[-1]['mnemonic']=='RET'
    (OUT/(name+'.disasm.json')).write_text(json.dumps(body,indent=2)+'\n')
    report.append(dict(address=name,bytes=size,instructions=len(body),end_exclusive=f'{at:08x}',
        raw_sha256=hashlib.sha256(raw(va,size)).hexdigest(),whole_raw_body_verified=True,
        call_operands=sorted(calls),source_admission=False,original_code_executed=False))
(OUT/'raw-review.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
