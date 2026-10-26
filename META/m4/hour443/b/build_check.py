import hashlib,json,os,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];OUT=ROOT/'build/physics-step';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
baseline=json.loads((ROOT/'build/m0/result.json').read_text());assert baseline['passed'] and not baseline['original_code_required']
result=dict(target='bhunter-physics-step-check.exe',passed=False,original_code_executed=False,commands=[],sources={})
def run(argv,expect=True):
 argv=[str(x) for x in argv];invocation=argv
 if Path(argv[0]).name.lower()=='link.exe':
  response=OUT/('link.rsp' if expect else 'missing.rsp');response.write_text('\n'.join(subprocess.list2cmdline([a]) for a in argv[1:])+'\n',encoding='mbcs');invocation=[argv[0],'@'+str(response)]
 p=subprocess.run(invocation,env=env,cwd=OUT,capture_output=True,text=True);result['commands'].append(dict(argv=argv,exit_code=p.returncode,output=p.stdout+p.stderr))
 if expect and p.returncode:(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for command in baseline['commands']:
 argv=command['argv'];source=Path(argv[-1])
 if not command['label'].startswith('compile-') or source.parent!=ROOT/'src':continue
 rel=source.relative_to(ROOT).as_posix();
 if rel in ['src/bh_physics_effects.c','src/bh_physics_step.c']:continue
 sha=hashlib.sha256(source.read_bytes()).hexdigest();assert sha==baseline['sources'][rel]
 result['sources'][rel]=sha;obj=OUT/(source.stem+'.obj');objects.append(obj)
 run([('/Fo'+str(obj)) if a.startswith('/Fo') else a for a in argv])
for rel in ['src/bh_physics_effects.c','src/bh_physics_step.c','tests/physics_step.c']:
 source=ROOT/rel;result['sources'][rel]=hashlib.sha256(source.read_bytes()).hexdigest();obj=OUT/(source.stem+('_test' if rel.startswith('tests/') else '')+'.obj');objects.append(obj)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DDIRECTINPUT_VERSION=0x0500','/DBH_PHYSICS_STEP_STANDALONE','/Fo'+str(obj),source])
for source in (ROOT/'src').glob('*.h'):result['sources'][source.relative_to(ROOT).as_posix()]=hashlib.sha256(source.read_bytes()).hexdigest()
libs=[a for a in next(c['argv'] for c in baseline['commands'] if c['label']=='link') if a.lower().endswith('.lib')]
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/map:'+str(OUT/'physics-step.map'),'/out:'+str(OUT/'bhunter-physics-step-check.exe'),*objects,*libs])
p=run([OUT/'bhunter-physics-step-check.exe']);result['stdout']=p.stdout;print(p.stdout)
n=run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/out:'+str(OUT/'missing-target.exe'),*[o for o in objects if o.name!='bh_physics_step.obj'],*libs],False)
assert n.returncode and 'bh_physics_step' in n.stdout
result['missing_module_link_control']=True;result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n')
