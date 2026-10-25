from pathlib import Path
import os,subprocess,json,hashlib,sys
root=Path('D:/Decomp/Bhunter');out=root/'build/actor-basis';out.mkdir(parents=True,exist_ok=True)
env=os.environ.copy()
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=root/'analysis/toolchain-sp3/bin'
env['PATH']=str(compiler)+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH'];env['INCLUDE']=str(sp3/'include')+';'+str(vc/'include');env['LIB']=str(sp3/'lib')+';'+str(vc/'lib')
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
(out/'main.c').write_text('int bh_actor_basis_checkpoint(unsigned long *);int main(void){unsigned long n;return bh_actor_basis_checkpoint(&n);}\n')
commands=[];passed=False
def run(argv):
 p=subprocess.run([str(x) for x in argv],cwd=out,env=env,capture_output=True,text=True);commands.append(dict(argv=[str(x) for x in argv],exit_code=p.returncode,output=p.stdout+p.stderr));print(p.stdout+p.stderr);return p
objects=[]
sources=(root/'src/bh_actor_basis.c',root/'tests/actor_basis.c',root/'src/bh_round.c',root/'src/bh_fp.c',root/'src/bh_rotation.c',root/'src/bh_math.c',out/'main.c')
for src in sources:
 obj=out/(src.stem+'.obj');objects.append(obj)
 if run([compiler/'cl.exe','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(obj),src]).returncode:break
else:
 if not run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(out/'actor-basis.exe'),*objects]).returncode:
  p=run([out/'actor-basis.exe']);passed=p.returncode==0
hashes={str(p.relative_to(root)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
(root/'META/m4/actor-basis/focused-validation.json').write_text(json.dumps(dict(target='actor-basis.exe',original_code_required=False,game_launches=0,passed=passed,source_sha256=hashes,commands=commands),indent=2)+'\n')
sys.exit(0 if passed else 1)
