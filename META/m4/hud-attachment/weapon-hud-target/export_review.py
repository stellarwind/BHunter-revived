"""Read-only complete weapon reticle owner export and immutable raw-byte audit."""
from pathlib import Path
import hashlib,json,re,struct,subprocess
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),'--project','Bhunter','--program','BHUNTER.EXE']
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(exe).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(va,n):
    _,start,_,offset=next(s for s in sections if s[1]<=va-0x400000 and va-0x400000+n<=s[1]+s[2])
    return exe[offset+va-0x400000-start:offset+va-0x400000-start+n]
for name,argv in [('function',['function','get']),('xrefs',['function','xrefs']),('decompile',['decompile']),('disasm',['disasm'])]:
    args=base+argv+['0x004314f0']+(['-n','235'] if name=='disasm' else [])
    rows=json.loads(subprocess.check_output(args,text=True))
    (HERE/'original'/('004314f0.'+name+'.json')).write_text(json.dumps(rows,indent=2)+'\n')
rows=json.loads((HERE/'original/004314f0.disasm.json').read_text());at=0x4314f0;constants={}
for row in rows:
    b=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and b==raw(at,len(b));at+=len(b)
    for operand in row['operands']:
        match=re.match(r'(float|double) ptr \[(0x00489[0-9a-f]+)\]',operand)
        if match:
            kind,va=match.groups();va=int(va,16);fmt='f' if kind=='float' else 'd';b=raw(va,struct.calcsize(fmt))
            constants[f'{va:08x}']=dict(kind=kind,raw=b.hex(),value=struct.unpack('<'+fmt,b)[0])
assert at==0x43180f and len(rows)==235 and rows[-1]['mnemonic']=='RET'
result=dict(address='004314f0',bytes=799,instructions=235,end_exclusive=f'{at:08x}',
    raw_sha256=hashlib.sha256(raw(0x4314f0,799)).hexdigest(),whole_raw_body_verified=True,
    constants=constants,original_code_executed=False)
(HERE/'raw-review.json').write_text(json.dumps(result,indent=2)+'\n')
with (HERE/'original/004314f0.tsv').open('w') as f:
    for row in rows:f.write(row['address']+'\t'+row['mnemonic']+' '+', '.join(row['operands'])+'\n')
print('Verified whole799-byte/235-instruction weapon reticle owner and',len(constants),'constants')
