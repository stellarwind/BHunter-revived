"""Read-only PE/Ghidra evidence for the two identical +12c methods."""
from pathlib import Path
import hashlib,json,struct,subprocess
ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
EXE=ROOT/'GAME_DATA/BHUNTER.EXE'
image=EXE.read_bytes()
assert hashlib.sha256(image).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',image,60)[0]
section_table=pe+24+struct.unpack_from('<H',image,pe+20)[0]
sections=[struct.unpack_from('<IIII',image,section_table+i*40+8)
          for i in range(struct.unpack_from('<H',image,pe+6)[0])]
def raw(va,size):
    rva=va-0x400000
    _,start,_,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
    return image[offset+rva-start:offset+rva-start+size]
base=['ghidra','--projects-dir',(ROOT/'ghidra-projects').as_posix(),
      '--project','Bhunter','--program','BHUNTER.EXE']
records=[];bodies=[]
for address,index in [(0x00433820,8),(0x00433b00,7)]:
    rows=json.loads(subprocess.check_output(base+['disasm',f'0x{address:08x}','-n','10'],text=True))
    body=b''.join(bytes.fromhex(row['bytes']) for row in rows)
    at=address
    for row in rows:
        assert int(row['address'],16)==at
        at+=len(bytes.fromhex(row['bytes']))
    assert len(body)==42 and at==address+42 and rows[-1]['mnemonic']=='RET'
    assert body==raw(address,42)
    assert not any(row['mnemonic']=='CALL' for row in rows)
    (HERE/f'{address:08x}.disasm.json').write_text(json.dumps(rows,indent=2)+'\n')
    bodies.append(body)
    records.append(dict(address=f'{address:08x}',end_exclusive=f'{at:08x}',
                        bytes=len(body),instructions=len(rows),index=index,
                        table_method_offset='12c',sha256=hashlib.sha256(body).hexdigest(),
                        role='game',historical_engine_provenance='unresolved',
                        direct_calls=[]))
assert bodies[0]==bodies[1]
assert raw(0x00489200,4)==bytes.fromhex('00000000')
assert raw(0x004891f8,8)==bytes.fromhex('111111111111a13f')
out=dict(original_exe_sha256=hashlib.sha256(image).hexdigest(),records=records,
         same_body=True,zero_float='00000000',double_step='111111111111a13f',
         double_step_value=struct.unpack('<d',raw(0x004891f8,8))[0],
         boundary='whole RET-ended table methods, not compiler, CRT, import or shared tail',
         original_code_executed=False)
(HERE/'timer-review.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(out))
