import os,subprocess,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent;OUT=ROOT/'build/hour443-physics-effects';OUT.mkdir(exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=ROOT/'analysis/toolchain-sp3/bin'
env=dict(os.environ)
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=str(compiler)+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH'];env['INCLUDE']=str(sp3/'include')+';'+str(vc/'include');env['LIB']=str(sp3/'lib')+';'+str(vc/'lib')
sources=['src/bh_physics_effects.c','tests/physics_effects.c','src/bh_entities.c','src/bh_draw_queue.c','src/bh_contact_effects.c','src/bh_sound_queue.c','src/bh_network_transport.c','src/bh_network_codec.c','src/bh_position.c','src/bh_math.c','src/bh_fp.c','src/bh_round.c'];commands=[];objects=[]
def run(cmd,ok=True):
 p=subprocess.run(list(map(str,cmd)),cwd=OUT,env=env,text=True,capture_output=True);commands.append(dict(argv=list(map(str,cmd)),exit_code=p.returncode,output=p.stdout+p.stderr))
 if ok and p.returncode:print(p.stdout+p.stderr);raise RuntimeError(str(cmd[0]))
 return p
for i,source in enumerate(sources):
 obj=OUT/('module%d.obj'%i);objects.append(obj);run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECTDRAW_VERSION=0x0500','/DDIRECT3D_VERSION=0x0500','/DBH_PHYSICS_EFFECTS_STANDALONE','/Fo'+str(obj),ROOT/source])
link=[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref'];libs=['kernel32.lib','user32.lib','winmm.lib']
exe=OUT/'bhunter-physics-effects.exe';run(link+['/out:'+str(exe),'/map:'+str(OUT/'effects.map'),*objects,*libs]);p=run([exe]);print(p.stdout)
r=json.loads(p.stdout.splitlines()[-1]);assert r['checks']>0 and not r['failures']
n=run(link+['/out:'+str(OUT/'missing.exe'),*objects[1:],*libs],False);assert n.returncode and 'LNK2001' in n.stdout+n.stderr and '_bh_physics_effects_run' in n.stdout+n.stderr
result=dict(passed=True,target=str(exe),fixture=r,commands=commands,sources={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources+['src/bh_physics_effects.h']},original_code_executed=False,missing_module_link_control=True)
(HERE/'standalone-build.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
