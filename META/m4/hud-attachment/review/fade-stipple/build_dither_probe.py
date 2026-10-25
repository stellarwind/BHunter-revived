import pathlib,os,json,hashlib,subprocess,struct,collections
root=pathlib.Path('D:/Decomp/Bhunter');here=root/'META/m4/hud-attachment/review/fade-stipple';src=here/'dither_probe.c';vc=pathlib.Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=pathlib.Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=root/'analysis/toolchain-sp3/bin';shared=pathlib.Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN');env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')]);deps=sorted((root/'build/m0').glob('bh_*.obj'));out={'question':'Is half-fade checkerboard caused by DITHERENABLE state?','original_code_executed':False,'gameplay_executed':False,'source_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'dependency_hashes':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in deps},'commands':[],'variants':[]}
def run(cmd,cwd):
 p=subprocess.run([str(x) for x in cmd],cwd=cwd,env=env,capture_output=True,text=True);out['commands'].append(dict(command=[str(x) for x in cmd],cwd=str(cwd),exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr));(here/'dither-result.json').write_text(json.dumps(out,indent=2));print(p.stdout+p.stderr)
 if p.returncode:raise SystemExit(p.returncode)
 return p
for name,flags in [('dither-false',['/DDIAG_DISABLE_DITHER'])]:
 work=root/'build/fade-stipple'/name;work.mkdir(parents=True,exist_ok=True);obj=work/'probe.obj';exe=work/'probe.exe'
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*flags,'/Fo'+str(obj),src],work)
 run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/out:'+str(exe),obj,*deps,'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'],work)
 p=run([exe],work);images=[]
 for im in sorted(work.glob('fade-native-*.bmp')):
  b=im.read_bytes();off=struct.unpack_from('<I',b,10)[0];w,h=struct.unpack_from('<ii',b,18);stride=(w*3+3)&~3;counts=collections.Counter(tuple(b[off+y*stride+x*3:off+y*stride+x*3+3][::-1]) for y in range(h) for x in range(w));images.append(dict(path=str(im.relative_to(root)),sha256=hashlib.sha256(b).hexdigest(),colors=counts.most_common()))
 out['variants'].append(dict(name=name,images=images));(here/'dither-result.json').write_text(json.dumps(out,indent=2))
print('DONE')
