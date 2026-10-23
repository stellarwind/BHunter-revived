"""Compare recovered functions to their original bytes; mask COFF relocations."""
import itertools,json,os,struct,subprocess
from pathlib import Path

repo=Path(__file__).resolve().parents[2]
out=repo/'analysis/math/builds';out.mkdir(exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
sp=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
tools=repo/'analysis/toolchain-sp3/bin'
env=os.environ.copy();env.pop('CL',None);env.pop('_CL_',None)
env['PATH']=str(tools)+os.pathsep+env['PATH']
env['INCLUDE']=str(sp/'include')+';'+str(vc/'include')
exe=(repo/'GAME_DATA/BHUNTER.EXE').read_bytes()
targets={
 '_bh_mat34_multiply':(0x401000,408), '_bh_vec3_cross':(0x4011a0,67),
 '_bh_vec3_normalize':(0x4011f0,113),'_bh_vec3_normalize_to':(0x401270,111),
 '_bh_mat34_transform_point':(0x4012e0,116),'_bh_mat34_transform_direction':(0x401360,107),
 '_bh_mat34_inverse_transform_point':(0x4013d0,140),'_bh_mat34_translate_world':(0x401460,45),
 '_bh_mat34_translate_local':(0x401490,123),'_bh_mat34_inverse_rigid':(0x401a70,179)}
results=[]
opts=['/Od','/O1','/O2','/Ox','/Og','/Og /Ot','/Og /Os','/Ot','/O2 /Og-','/O2 /Oi-','/O2 /Oy-','/O2 /Os']
for opt,cpu,extra in itertools.product(opts,['/G3','/G4','/G5','/G6','/GB'],[[],['/Op'],['/Oa'],['/Ow']]):
 flags=[*opt.split(),cpu,*extra];name='_'.join(s[1:] for s in flags);obj=out/(name+'.obj')
 command=[str(tools/'CL.EXE'),'/nologo','/c','/ML','/Gy',*flags,'/Fo'+str(obj),str(repo/'src/bh_math.c')]
 p=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True)
 if p.returncode:raise RuntimeError(p.stdout+p.stderr)
 b=obj.read_bytes();symptr,nsym=struct.unpack_from('<II',b,8);st=symptr+nsym*18;i=0
 functions=[]
 while i<nsym:
  at=symptr+18*i;raw=b[at:at+8]
  if raw[:4]==b'\0'*4:
   z=st+struct.unpack_from('<I',raw,4)[0];symbol=b[z:].split(b'\0')[0].decode()
  else:symbol=raw.split(b'\0')[0].decode()
  value,section,typ,storage,aux=struct.unpack_from('<IhHBB',b,at+8)
  if symbol in targets and section>0:
   sh=20+(section-1)*40;size,rawptr,relptr=struct.unpack_from('<III',b,sh+16);nr=struct.unpack_from('<H',b,sh+32)[0]
   code=b[rawptr+value:rawptr+size];mask=bytearray(b'\1'*len(code))
   for j in range(nr):
    addr,sym,kind=struct.unpack_from('<IIH',b,relptr+10*j)
    assert kind in (6,20),(symbol,kind)
    mask[addr-value:addr-value+4]=b'\0'*4
   va,length=targets[symbol];original=exe[va-0x400c00:va-0x400c00+length]
   same_length=len(code)>=length and all(c in (0x90,0xcc,0) for c in code[length:])
   equal=sum(code[k]==original[k] or not mask[k] for k in range(min(length,len(code))))
   prefix=0
   for k in range(min(length,len(code))):
    if mask[k] and code[k]!=original[k]:break
    prefix+=1
   functions.append(dict(name=symbol,address=f'{va:08x}',bytes=len(code),target_bytes=length,relocations=nr,prefix_bytes=prefix,equal_positions=equal,exact_match=same_length and equal==length))
  i+=1+aux
 row=dict(flags=flags,functions=functions,compiler_output=p.stdout+p.stderr)
 results.append(row)
 for f in functions:
  if f['exact_match']:print('MATCH',flags,f['name'])
(repo/'analysis/math/build-comparison.json').write_text(json.dumps(results,indent=2))
for symbol in targets:
 ranking=sorted([(f['prefix_bytes'],f['equal_positions'],row['flags'],f['bytes']) for row in results for f in row['functions'] if f['name']==symbol],reverse=True)
 print('BEST',symbol,ranking[0])
