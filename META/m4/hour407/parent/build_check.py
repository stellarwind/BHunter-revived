"""Pinned real native sound startup source check; no game/original execution."""
import os,subprocess,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent;OUT=ROOT/'build/hour407-sound';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=ROOT/'analysis/toolchain-sp3/bin'
env=dict(os.environ)
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=str(compiler)+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH']
env['INCLUDE']=str(sp3/'include')+';'+str(vc/'include');env['LIB']=str(sp3/'lib')+';'+str(vc/'lib')
sources=['src/bh_sound_startup.c','src/bh_sound_queue.c','src/bh_round.c','src/bh_fp.c','tests/sound_startup.c','META/m4/hour407/parent/driver.c'];commands=[];objects=[]
def run(args,success=True):
    p=subprocess.run(list(map(str,args)),cwd=OUT,env=env,text=True,capture_output=True);commands.append(dict(argv=list(map(str,args)),exit_code=p.returncode,output=p.stdout+p.stderr))
    if success and p.returncode:print(p.stdout+p.stderr);raise RuntimeError(str(args[0]))
    return p
for i,source in enumerate(sources):
    obj=OUT/('module%d.obj'%i);objects.append(obj);run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(obj),ROOT/source])
exe=OUT/'bhunter-sound-startup-check.exe';link=vc/'bin/LINK.EXE';libs=['kernel32.lib','user32.lib','dsound.lib']
run([link,'/nologo','/subsystem:console','/opt:ref','/map:'+str(OUT/'sound-startup.map'),'/out:'+str(exe),*objects,*libs])
p=run([exe]);fixture=json.loads(p.stdout.splitlines()[-1]);assert fixture['checks']>0 and not fixture['failures']
n=run([link,'/nologo','/subsystem:console','/opt:ref','/out:'+str(OUT/'missing.exe'),*objects[1:],*libs],False);assert n.returncode and 'LNK2001' in n.stdout+n.stderr and '_bh_sound_startup' in n.stdout+n.stderr
r=dict(passed=True,fixture=fixture,target=str(exe),commands=commands,sources={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources+['src/bh_sound_startup.h']},original_code_executed=False,game_launches=0)
(HERE/'standalone-build.json').write_bytes((json.dumps(r,indent=2)+'\n').encode());print(json.dumps(fixture))
