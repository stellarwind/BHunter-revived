import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
out=ROOT/'build/frame-pipeline/cd';out.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
env=dict(os.environ);env['PATH']=str(ROOT/'analysis/toolchain-sp3/bin')+os.pathsep+str(vc.parent/'SHAREDIDE/BIN')+os.pathsep+env['PATH']
env['INCLUDE']=str(sp3/'include')+os.pathsep+str(vc/'include');env['LIB']=str(sp3/'lib')+os.pathsep+str(vc/'lib')
commands=[]
def run(args):
 p=subprocess.run([str(x) for x in args],env=env,cwd=out,capture_output=True,text=True)
 commands.append(dict(argv=[str(x) for x in args],exit_code=p.returncode,output=p.stdout+p.stderr));assert not p.returncode,p.stdout+p.stderr
 return p.stdout
(out/'main.c').write_text('int bh_cd_close_checkpoint(unsigned long*);int main(void){unsigned long n;return bh_cd_close_checkpoint(&n);}')
objects=[]
for source in [ROOT/'src/bh_cd_close.c',ROOT/'tests/cd_close.c',out/'main.c']:
 obj=out/(source.stem+str(len(objects))+'.obj');objects.append(obj)
 run([ROOT/'analysis/toolchain-sp3/bin/CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(obj),source])
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(out/'cd-check.exe'),*objects,'winmm.lib','kernel32.lib'])
stdout=run([out/'cd-check.exe'])
(HERE/'cd-focused.json').write_text(json.dumps(dict(passed=True,commands=commands,stdout=stdout,original_code_executed=False),indent=2)+'\n')
print(stdout)
