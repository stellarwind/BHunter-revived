"""Read-only original PE range evidence for discontiguous renderer callback."""
from pathlib import Path
import hashlib,json,struct

ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
ORIGINAL=HERE.parent/'original'
ORIGINAL.mkdir(parents=True,exist_ok=True)
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
exe_hash=hashlib.sha256(exe).hexdigest()
assert exe_hash=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',exe,60)[0]
table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
sections=[struct.unpack_from('<IIII',exe,table+i*40+8)
          for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(address,size):
    rva=address-0x400000
    section=next((s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2]),None)
    assert section is not None
    return exe[section[3]+rva-section[1]:section[3]+rva-section[1]+size]
assembly=(ROOT/'META/m1/review/00479090.asm.txt').read_text()
rows=[line for line in assembly.splitlines() if line.startswith('0045') or line.startswith('0047')]
assert len(rows)==64
entry=[line for line in rows if line.startswith(('004790','004791'))]
tail=[line for line in rows if line.startswith('004509')]
assert len(entry)==38 and len(tail)==26
assert entry[0].startswith('00479090 ') and entry[-1]=='00479111 JMP 0x00450930'
assert tail[0].startswith('00450930 ') and tail[-1]=='00450978 RET'
assert sum('CALL 0x0045cbc0' in line for line in entry)==1
assert sum('CALL 0x0045ca10' in line for line in entry)==1
assert sum('CALL 0x004508a0' in line for line in tail)==1
assert raw(0x00479111,5)[0]==0xe9
offset=struct.unpack('<i',raw(0x00479112,4))[0]
assert 0x00479116+offset==0x00450930
ranges=[]
for start,end,role,instructions in ((0x00479090,0x00479116,'entry',38),
                                    (0x00450930,0x00450979,'owned_tail',26)):
    body=raw(start,end-start)
    ranges.append(dict(start=f'{start:08x}',end_exclusive=f'{end:08x}',
        role=role,bytes=len(body),instructions=instructions,
        sha256=hashlib.sha256(body).hexdigest(),hex=body.hex()))
(ORIGINAL/'00479090.asm.txt').write_text(assembly)
(ORIGINAL/'00479090.ranges.json').write_text(json.dumps(dict(
    original_exe_sha256=exe_hash,function='00479090',ranges=ranges,
    source_disasm='META/m1/review/00479090.asm.txt',
    tail_jump='00479111 -> 00450930',
    direct_calls=['0045cbc0','0045ca10','004508a0'],
    provenance='engine renderer callback; historical authorship unresolved',
    source_owner='src/bh_render_execute.c',
    source_selector_binding='typed indexes and borrowed contexts replace original function pointers',
    original_code_executed=False),indent=2)+'\n')
print(json.dumps(dict(ranges=len(ranges),bytes=sum(r['bytes'] for r in ranges))))
