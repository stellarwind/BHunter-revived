"""Real-TU source-only private offscreen display/capture dependency checks."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/model-scale';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_model_scale.c','tests/model_scale.c','src/bh_round.c','src/bh_fp.c','src/bh_math.c','src/bh_model_draw.c']
inputs=sources+['src/bh_model_scale.h']
result=dict(target='bhunter-model-scale-check.exe',original_code_executed=False,passed=False,commands=[],sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs})
def run(command,expected_success=True):
 p=subprocess.run([str(x) for x in command],env=env,cwd=OUT,capture_output=True,text=True)
 result['commands'].append(dict(command=[str(x) for x in command],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode and expected_success:
  (HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for source in sources:
 obj=OUT/(Path(source).stem+('_test' if source.startswith('tests/') else '')+'.obj');objects.append(obj)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DBH_MODEL_SCALE_STANDALONE','/DDIRECTINPUT_VERSION=0x0500','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/Fo'+str(obj),ROOT/source])
own_names={p.name for p in objects}
dependencies=sorted(p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.name not in own_names and 'agent' not in p.name)
result['borrowed_real_integration_objects']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in dependencies}
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/map:'+str(OUT/'model-scale.map'),'/out:'+str(OUT/'bhunter-model-scale-check.exe'),*objects,*dependencies,'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
p=run([OUT/'bhunter-model-scale-check.exe']);result['clean_stdout']=p.stdout
p=run([OUT/'bhunter-model-scale-check.exe',ROOT/'GAME_DATA/Bhunter.bin']);result['real_stdout']=p.stdout;result['passed']=True
control=run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/out:'+str(OUT/'missing-model-scale.exe'),*objects[1:],*dependencies,'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'],False)
assert control.returncode and 'bh_model_scale' in control.stdout and 'bh_model_reverse_edges' in control.stdout
result['missing_module_link_control']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n');print(result['clean_stdout']+result['real_stdout'])
