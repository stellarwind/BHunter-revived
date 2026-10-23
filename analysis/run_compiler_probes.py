"""Compile small reconstructed math routines under candidate VC5 options."""
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

root = Path(__file__).resolve().parent
base = Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO')
patch = Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3')
bindir = root/'toolchain-sp3'/'bin'
mode = sys.argv[1] if len(sys.argv)>1 else 'sp3'
if mode not in ('sp3','stock','vc41'):
    raise ValueError(mode)
bindir = root/('toolchain-'+mode)/'bin'
bindir.mkdir(parents=True, exist_ok=True)
if mode=='vc41':
    old=Path('D:/Decomp/MSCVC/MSVC41/VC41/MSDEV/BIN')
    # Keep this older toolset's own support DLLs together.
    for f in old.iterdir():
        if f.is_file() and f.suffix.lower() in ('.dll','.exe'): shutil.copy2(f,bindir/f.name)
else:
    shutil.copy2(base/'VC/BIN/CL.EXE', bindir/'CL.EXE')
    for f in ['c1.dll','c1xx.dll','c2.exe','link.exe']:
        shutil.copy2((patch/'enu/vc/bin' if mode=='sp3' else base/'VC/BIN')/f, bindir/f)
    for f in ['MSPDB50.DLL','MSDIS100.DLL','MSVCP50.DLL']:
        shutil.copy2(base/'SHAREDIDE/BIN'/f, bindir/f)
env = os.environ.copy()
env.pop('CL',None)
env.pop('_CL_',None)
env['PATH'] = str(bindir)+os.pathsep+env['PATH']
exe = (root.parent/'GAME_DATA/BHUNTER.EXE').read_bytes()
# File offset differs from VA by 0x400c00 in this PE's .text section.
targets = {'_probe_translate': exe[0x1460-0xc00:0x1460-0xc00+45],
           '_probe_cross': exe[0x11a0-0xc00:0x11a0-0xc00+67]}
# Correct .text RVA 0x1000 -> file offset 0x400.
assert targets['_probe_translate'].startswith(bytes.fromhex('d9442404'))
outdir = root/('compiler-probes-'+mode)
outdir.mkdir(exist_ok=True)
results=[]
source=(root/'compiler_probes.c').read_text()
source+='\ntypedef struct Vec3 {float x,y,z;} Vec3;\ntypedef struct Mat34 {float m00,m01,m02,m03,m10,m11,m12,m13,m20,m21,m22,m23;} Mat34;\n'
for idx, choices in enumerate(itertools.product([False,True],repeat=3)):
    statements=[]
    for offset,arg,flip in zip([3,7,11],['x','y','z'],choices):
        rhs=f'{arg} + m[{offset}]' if flip else f'm[{offset}] + {arg}'
        statements.append(f'm[{offset}] = {rhs};')
    source+=f'\nvoid probe_translate_v{idx}(float x,float y,float z,float *m) {{'+''.join(statements)+'}\n'
    typed=''.join(statements)
    for offset,field in [(3,'m03'),(7,'m13'),(11,'m23')]:typed=typed.replace(f'm[{offset}]',f'm->{field}')
    source+=f'\nvoid probe_translate_struct_v{idx}(float x,float y,float z,Mat34 *m) {{'+typed+'}\n'
products=[('a[1]','b[2]'),('b[1]','a[2]'),('a[2]','b[0]'),('b[2]','a[0]'),('b[1]','a[0]'),('a[1]','b[0]')]
for idx,choices in enumerate(itertools.product([False,True],repeat=6)):
    expressions=[]
    for (a,b),flip in zip(products,choices):
        expressions.append(f'{b} * {a}' if flip else f'{a} * {b}')
    source+=f'\nvoid probe_cross_v{idx}(float *a,float *b,float *out) {{'+''.join(f'out[{i}] = {expressions[2*i]} - {expressions[2*i+1]};' for i in range(3))+'}\n'
    typed=''.join(f'out[{i}] = {expressions[2*i]} - {expressions[2*i+1]};' for i in range(3))
    for pointer in ['a','b','out']:
        for i,field in enumerate(['x','y','z']):typed=typed.replace(f'{pointer}[{i}]',f'{pointer}->{field}')
    source+=f'\nvoid probe_cross_struct_v{idx}(Vec3 *a,Vec3 *b,Vec3 *out) {{'+typed+'}\n'
sourcepath=outdir/'generated-probes.c'
sourcepath.write_text(source)
for opt, cpu, fp in itertools.product(['/Od','/O1','/O2','/Ox','/Og','/Og /Ot','/Og /Os','/Ot','/O2 /Og-','/O2 /Oi-','/O2 /Oy-','/O2 /Os'],['/G3','/G4','/G5','/G6','/GB'],[[],['/Op'],['/Oa'],['/Ow']]):
    flags=[*opt.split(),cpu,*fp]
    name='_'.join(f.strip('/') for f in flags)
    obj=outdir/(name+'.obj')
    command=[str(bindir/'CL.EXE'),'/nologo','/c','/ML','/Gy',*flags,'/Fo'+str(obj),str(sourcepath)]
    process=subprocess.run(command,env=env,cwd=outdir,capture_output=True,text=True)
    row={'flags':flags,'exit_code':process.returncode,'output':process.stdout+process.stderr,'functions':[]}
    if process.returncode:
        results.append(row); continue
    b=obj.read_bytes()
    symptr, nsym=struct.unpack_from('<II',b,8)
    strtab=symptr+nsym*18
    si=0
    while si<nsym:
        so=symptr+si*18; raw=b[so:so+8]
        if raw[:4]==b'\0'*4:
            at=strtab+struct.unpack_from('<I',raw,4)[0];symbol=b[at:].split(b'\0')[0].decode()
        else:symbol=raw.split(b'\0')[0].decode()
        val,sec,typ,storage,aux=struct.unpack_from('<IhHBB',b,so+8)
        target_name = next((t for t in targets if symbol==t or symbol.startswith(t+'_')),None)
        if target_name in targets and sec>0:
            sh=20+(sec-1)*40; size,rawptr=struct.unpack_from('<II',b,sh+16)
            code=b[rawptr+val:rawptr+size]
            target=targets[target_name]
            # Strip only trailing object section alignment padding.
            exact=code[:len(target)]==target and all(v in (0x90,0xcc,0) for v in code[len(target):])
            row['functions'].append({'name':symbol,'code_hex':code.hex(),'length':len(code),'target_length':len(target),'exact_match':exact})
        si+=1+aux
    results.append(row)
    matches=[f['name'] for f in row['functions'] if f['exact_match']]
    if matches: print(name,matches)
(root/('compiler-probes-'+mode+'.json')).write_text(json.dumps(results,indent=2))
assert all(r['exit_code']==0 for r in results), 'Inspect compiler output for failed variants'
