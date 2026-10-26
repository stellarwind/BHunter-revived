import os,subprocess,json,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[4];here=Path(__file__).resolve().parent
out=root/'build/hour424-menu';out.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');compiler=root/'analysis/toolchain-sp3/bin';sdk=Path('D:/Decomp/dx7sdk')
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=str(compiler)+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH'];env['INCLUDE']=str(sp3/'include')+';'+str(vc/'include');env['LIB']=str(sp3/'lib')+';'+str(vc/'lib')
commands=[]
def run(args,success=True):
 p=subprocess.run(list(map(str,args)),cwd=out,env=env,text=True,capture_output=True);commands.append(dict(argv=list(map(str,args)),exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if success and p.returncode:raise RuntimeError(commands[-1])
 return p
sources=['src/bh_menu_row.c','tests/menu_row.c','META/m4/hour424/c2/driver.c'];new=[]
for i,source in enumerate(sources):
 obj=out/('module%d.obj'%i);new.append(obj);run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/Fo'+str(obj),root/source])
baseline=json.loads((root/'build/m0/result.json').read_text());link_command=next(c for c in baseline['commands'] if c['label']=='link');objects=[Path(v) for v in link_command['argv'] if Path(v).suffix.lower()=='.obj' and Path(v).stem.startswith('bh_') and Path(v).stem!='bh_menu_row'];linked={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in objects};exe=out/'menu-row.exe';link=vc/'bin/LINK.EXE';libs=[sdk/'lib/ddraw.lib',sdk/'lib/dinput.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib']
run([link,'/nologo','/subsystem:console','/opt:ref','/map:'+str(out/'menu-row.map'),'/out:'+str(exe),*objects,*new,*libs])
clean=run([exe]);real=run([exe,root/'GAME_DATA/Bhunter.bin']);fixtures=[json.loads(p.stdout.splitlines()[-1]) for p in [clean,real]]
n=run([link,'/nologo','/subsystem:console','/opt:ref','/out:'+str(out/'negative.exe'),*objects,*new[1:],*libs],False);assert n.returncode and 'LNK2001' in n.stdout+n.stderr and '_bh_menu_row_create' in n.stdout+n.stderr
result=dict(passed=not any(f['failures'] for f in fixtures),fixtures=fixtures,target=str(exe),commands=commands,sources={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources+['src/bh_menu_row.h']},linked_existing_objects=linked,game_launches=0,original_code_executed=False,new_whole_functions=2,implementation='candidate pending full integration')
(here/'focused-validation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(fixtures))
