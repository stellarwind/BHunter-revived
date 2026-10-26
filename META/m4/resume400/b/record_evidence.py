import csv,hashlib,json,struct,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
data=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(data).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe=struct.unpack_from('<I',data,0x3c)[0];count=struct.unpack_from('<H',data,pe+6)[0];opts=struct.unpack_from('<H',data,pe+20)[0];sections=[]
for i in range(count):
 off=pe+24+opts+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',data,off+8);sections.append((va,rs,rp))
def raw(va,size):
 rva=va-0x400000
 for start,length,pos in sections:
  if start<=rva and rva+size<=start+length:return data[pos+rva-start:pos+rva-start+size]
 raise ValueError(hex(va))
rows={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
owners=[]
for address,name in [('00443800','bh_script_translate_tick'),('004439a0','bh_script_rotate_tick')]:
 row=rows[address];va=int(address,16);size=int(row['body_bytes'])
 asm=subprocess.check_output(['objdump','-d','-Mintel',f'--start-address=0x{va:x}',f'--stop-address=0x{va+size:x}','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
 (HERE/(address+'.asm.txt')).write_text(asm)
 assert raw(va,size)[-1]==0xc3
 owners.append(dict(original_va=address,function=name,source='src/bh_script_motion.c',role='engine_candidate',
  provenance_confidence='Ordinary complete script target movement/rotation callback; parser installs both reviewed entrypoints; proprietary game/engine authorship unresolved; no verified CRT/import/compiler helper/shared tail counted',
  boundary=dict(start=address,end_exclusive=f'{va+size:08x}',bytes=size,instructions=int(row['instructions']),sha256=hashlib.sha256(raw(va,size)).hexdigest()),
  validation=dict(abi='32-bit x86 cdecl BHEntityStep; actual entity fields and source pool ownership, PC53 nearest/masked',
   limits=['Functional reconstruction, not byte match; complete script VM and movement target construction remain unrecovered.',
    'Source explicit binding guards reject dead/foreign/self target and malformed/cyclic attachment chains before mutation; target/context ownership must stay stable while bound.',
    'Movable target matrices/bounds and origin preinitialization are explicit caller state, not guessed globals or autonomous route actor layout.',
    'Finite moderate matrix geometry and current PC53 numerical profile; PC24 and NaN payload/sign are not validated.',
    'No game/original code execution, automated gameplay, physical input, native audio or Play calls.']),
  evidence=['META/m4/resume400/b/'+address+'.asm.txt','META/m4/resume400/b/constants.json','META/m4/resume400/b/parser-install.asm.txt','META/m4/resume400/b/README.md','META/m4/resume400/b/standalone-build.json']))
(HERE/'owners.json').write_text(json.dumps(owners,indent=2)+'\n')
constants=[]
for a in [0x489498,0x4894a0,0x4894a8,0x4894b0,0x4891f8]:
 b=raw(a,8);constants.append(dict(address=f'{a:08x}',bytes=b.hex(),binary64=struct.unpack('<d',b)[0]))
(HERE/'constants.json').write_text(json.dumps(constants,indent=2)+'\n')
p=subprocess.check_output(['objdump','-d','-Mintel','--start-address=0x444a07','--stop-address=0x444ae2','GAME_DATA/BHUNTER.EXE'],cwd=ROOT,text=True)
(HERE/'parser-install.asm.txt').write_text(p)
