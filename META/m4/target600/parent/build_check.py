import hashlib,json,os,subprocess,shutil
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];OUT=ROOT/'build/effect-projectile';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ)
for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_effect_projectile.c','src/bh_effect_accelerated.c','src/bh_effect_puff.c','src/bh_effect_split_tick.c','src/bh_effect_orbit_tick.c','src/bh_effect_network.c','tests/effect_projectile.c'];result=dict(target='bhunter-effect-projectile-check.exe',passed=False,original_code_executed=False,commands=[],sources={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources+['src/bh_effect_projectile.h']})
def run(cmd,ok=True):
 p=subprocess.run([str(x) for x in cmd],env=env,cwd=OUT,capture_output=True,text=True);result['commands'].append(dict(command=[str(x) for x in cmd],exit_code=p.returncode,output=p.stdout+p.stderr))
 if ok and p.returncode:(HERE/'query-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for s in sources:
 o=OUT/(Path(s).stem+('_test' if s.startswith('tests/') else '')+'.obj');objects.append(o);run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/DBH_EFFECT_PROJECTILE_STANDALONE','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DDIRECTINPUT_VERSION=0x0500','/Fo'+str(o),ROOT/s])
fresh={p.stem for p in objects};existing=[p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.stem not in fresh and (ROOT/'src'/(p.stem+'.c')).is_file()]
# Snapshot retained admitted objects, so an unrelated parent rebuild cannot
# change this target after its recorded link fingerprint was captured.
depdir=OUT/'admitted-dependencies';depdir.mkdir(exist_ok=True);retained=[];result['linked_existing_objects']={}
for p in existing:
 body=p.read_bytes();copy=depdir/p.name;copy.write_bytes(body);retained.append(copy);result['linked_existing_objects'][str(p.relative_to(ROOT))]=hashlib.sha256(body).hexdigest()
objects+=retained
result['retained_object_limit']='These exact admitted object bytes supply the renderer closure; parent fresh full integration remains required. No source byte-match evidence inferred.'
result['header_context']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'src').glob('*.h')}

link=[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref'];run(link+['/out:'+str(OUT/'bhunter-effect-projectile-check.exe'),'/map:'+str(OUT/'effect-projectile.map'),*objects,'kernel32.lib','user32.lib','dsound.lib','ole32.lib','winmm.lib','gdi32.lib','D:/Decomp/dx7sdk/lib/ddraw.lib','D:/Decomp/dx7sdk/lib/dinput.lib']);p=run([OUT/'bhunter-effect-projectile-check.exe']);result['stdout']=p.stdout;print(p.stdout)
n=run(link+['/out:'+str(OUT/'missing-effect-projectile.exe'),*objects[1:],'kernel32.lib','user32.lib','dsound.lib','ole32.lib','winmm.lib','gdi32.lib','D:/Decomp/dx7sdk/lib/ddraw.lib','D:/Decomp/dx7sdk/lib/dinput.lib'],False);assert n.returncode and 'bh_effect_projectile_bind' in n.stdout;result['missing_module_link_control']=True;result['passed']=True
(HERE/'query-build.json').write_text(json.dumps(result,indent=2)+'\n')
