"""Compile new real TUs and link verified existing real source dependency objects."""
import hashlib,json,os,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3];OUT=ROOT/'build/flash-flare';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN');sdk=Path('D:/Decomp/dx7sdk')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_flash_flare.c','tests/flash_flare.c','src/bh_render_execute.c','src/bh_message_panel.c','src/bh_atlas_wide.c'];base=json.loads((ROOT/'build/m0/result.json').read_text());dependencies=[]
for source,sha in base['sources'].items():
 if source.startswith('src/') and source.endswith('.c') and source not in sources:
  obj=ROOT/'build/m0'/(Path(source).stem+'.obj')
  if obj.exists():
   assert hashlib.sha256((ROOT/source).read_bytes()).hexdigest()==sha,source
   dependencies.append(dict(source=source,source_sha256=sha,object=str(obj),object_sha256=hashlib.sha256(obj.read_bytes()).hexdigest()))
result=dict(target='bhunter-flash-flare-check.exe',passed=False,original_code_executed=False,game_executed=False,commands=[],sources={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources+['src/bh_flash_flare.h']},real_dependency_objects=dependencies)
def run(cmd,ok=True):
 p=subprocess.run([str(x) for x in cmd],env=env,cwd=OUT,capture_output=True,text=True);result['commands'].append(dict(command=[str(x) for x in cmd],exit_code=p.returncode,output=p.stdout+p.stderr))
 if ok and p.returncode:(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for s in sources:
 o=OUT/(Path(s).stem+('_test' if s.startswith('tests/') else '')+'.obj');objects.append(o)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DDIRECTINPUT_VERSION=0x0500','/DBH_FLASH_FLARE_STANDALONE','/Fo'+str(o),ROOT/s])
link=[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref'];libraries=[sdk/'lib/ddraw.lib',sdk/'lib/dinput.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'];deps=[x['object'] for x in dependencies]
run(link+['/out:'+str(OUT/'bhunter-flash-flare-check.exe'),'/map:'+str(OUT/'flash-flare.map'),*objects,*deps,*libraries]);p=run([OUT/'bhunter-flash-flare-check.exe']);result['stdout']=p.stdout;print(p.stdout)
n=run(link+['/out:'+str(OUT/'missing-flare.exe'),objects[1],*deps,*libraries],False);assert n.returncode and 'bh_flare_execute' in n.stdout;result['missing_module_link_control']=True;result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n')
