"""Real-TU source-only private offscreen display/capture dependency checks."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/frame-pipeline/dispatch-dependencies';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_force_feedback_start.c','src/bh_cd_notify.c','tests/force_feedback_start.c','tests/cd_notify.c']
inputs=sources+['src/bh_force_feedback_start.h','src/bh_cd_notify.h']
result=dict(target='bhunter-notify-check.exe',original_code_executed=False,passed=False,commands=[],sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs})
def run(command):
 p=subprocess.run([str(x) for x in command],env=env,cwd=OUT,capture_output=True,text=True)
 result['commands'].append(dict(command=[str(x) for x in command],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:
  (HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for source in sources:
 obj=OUT/(Path(source).stem+('_test' if source.startswith('tests/') else '')+'.obj');objects.append(obj)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECTINPUT_VERSION=0x0500','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/Fo'+str(obj),ROOT/source])
(OUT/'main.c').write_text('int bh_force_feedback_start_checkpoint(unsigned long*); int bh_cd_notify_checkpoint(unsigned long*); int main(void){unsigned long n;int a=bh_force_feedback_start_checkpoint(&n);return a|bh_cd_notify_checkpoint(&n);}')
run([compiler/'CL.EXE','/nologo','/c','/ML','/Fo'+str(OUT/'main.obj'),OUT/'main.c']);objects.append(OUT/'main.obj')
own_names={p.name for p in objects}
dependencies=sorted(p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.name not in own_names and 'agent' not in p.name)
result['borrowed_real_integration_objects']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/map:'+str(OUT/'display-dependencies.map'),'/out:'+str(OUT/'bhunter-notify-check.exe'),*objects,*dependencies,'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
p=run([OUT/'bhunter-notify-check.exe']);result['stdout']=p.stdout;result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout)
