import os,subprocess,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[4]
out=root/'build/target400-c';out.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=root/'analysis/toolchain-sp3/bin'
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=str(compiler)+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH']
env['INCLUDE']=str(sp3/'include')+';'+str(vc/'include');env['LIB']=str(sp3/'lib')+';'+str(vc/'lib')
commands=[]
def run(args,expected=0):
 p=subprocess.run(list(map(str,args)),cwd=out,env=env,text=True,capture_output=True);commands.append(dict(command=list(map(str,args)),exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr));assert (p.returncode==0)==(expected==0),commands[-1];return p
sources=['src/bh_serial_options.c','tests/serial_options.c','META/m4/target400/c/driver.c'];objects=[]
for i,source in enumerate(sources):
 obj=out/('module%d.obj'%i);objects.append(obj);run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(obj),root/source])
link=vc/'bin/LINK.EXE';exe=out/'serial-options.exe'
run([link,'/nologo','/subsystem:console','/machine:ix86','/out:'+str(exe),*objects,'kernel32.lib'])
p=run([exe]);result=json.loads(p.stdout)
run([link,'/nologo','/subsystem:console','/machine:ix86','/out:'+str(out/'negative.exe'),*objects[1:],'kernel32.lib'],expected=1)
r=dict(passed=result['failures']==0,target=str(exe),fixture=result,commands=commands,sources={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources+['src/bh_serial_options.h']},game_launches=0,original_code_executed=False,implementation='candidate pending full integration',new_whole_functions=10)
(root/'META/m4/target400/c/focused-validation.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(result))
