"""Compile the real menu settings TU, link, run and reject omitted owner TU."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/target400-menu-settings';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_menu_settings.c','tests/menu_settings.c'];inputs=sources+['src/bh_menu_settings.h','src/bh_config.h']
result=dict(target='bhunter-menu-settings-check.exe',original_code_executed=False,game_executed=False,passed=False,commands=[],sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs})
def run(cmd,success=True):
 p=subprocess.run([str(x) for x in cmd],env=env,cwd=OUT,capture_output=True,text=True)
 result['commands'].append(dict(command=[str(x) for x in cmd],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if success and p.returncode:
  (HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for source in sources:
 obj=OUT/(Path(source).stem+'.obj');objects.append(obj)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/DBH_MENU_SETTINGS_STANDALONE','/Fo'+str(obj),ROOT/source])
link=[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref']
run(link+['/map:'+str(OUT/'menu-settings.map'),'/out:'+str(OUT/'bhunter-menu-settings-check.exe'),*objects,'kernel32.lib'])
p=run([OUT/'bhunter-menu-settings-check.exe']);result['stdout']=p.stdout
negative=run(link+['/out:'+str(OUT/'missing-menu-settings.exe'),objects[1],'kernel32.lib'],False)
assert negative.returncode and 'bh_menu_keys_motion' in negative.stdout and 'bh_menu_network_phone' in negative.stdout
result['missing_module_link_control']=True;result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout)
