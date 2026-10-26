import os,json,hashlib,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[4]
here=Path(__file__).resolve().parent
out=root/'build/target600-c';out.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
env=os.environ.copy();env['INCLUDE']=str(vc/'include');env['LIB']=str(vc/'lib');env['PATH']=str(root/'analysis/toolchain-sp3/bin')+';'+str(vc/'bin')+';'+env['PATH']
commands=[]
def run(cmd,ok=True):
 p=subprocess.run(list(map(str,cmd)),env=env,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 commands.append(dict(command=list(map(str,cmd)),exit_code=p.returncode,output=p.stdout))
 if ok:assert p.returncode==0,p.stdout
 return p
sources=['src/bh_actor_basis_control.c','src/bh_actor_basis.c','src/bh_math.c','src/bh_round.c','src/bh_fp.c','tests/actor_basis_control.c']
objects=[]
for source in sources:
 obj=out/(Path(source).stem+'.obj');objects.append(obj)
 run([root/'analysis/toolchain-sp3/bin/CL.EXE','/nologo','/c','/ML','/O2','/G5','/Gy','/DBH_ACTOR_BASIS_CONTROL_STANDALONE','/Fo'+str(obj),source])
link=[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref']
exe=out/'bhunter-actor-basis-control-check.exe';run(link+['/out:'+str(exe)]+objects+['kernel32.lib']);p=run([exe]);result=json.loads(p.stdout)
control=run(link+['/out:'+str(out/'missing.exe')]+objects[1:]+['kernel32.lib'],False);assert control.returncode!=0
result.update(target=exe.name,commands=commands,missing_module_link_control=True,original_code_executed=False,sources={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources})
(here/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout)
