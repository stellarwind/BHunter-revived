"""Compile actual source TUs and link/execute only source argument fixtures."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/script-wait';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_script_wait.c','tests/script_wait.c','src/bh_script_timer.c','tests/script_timer.c','src/bh_entities.c','src/bh_math.c','src/bh_fp.c','src/bh_round.c'];inputs=sources+['src/bh_script_wait.h','src/bh_script_timer.h','tests/script_timer_cases.h']
result=dict(target='bhunter-script-wait-check.exe',original_code_executed=False,passed=False,commands=[],sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in inputs})
def run(command,success=True):
 p=subprocess.run([str(x) for x in command],env=env,cwd=OUT,capture_output=True,text=True)
 result['commands'].append(dict(command=[str(x) for x in command],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if success and p.returncode:
  (HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for source in sources:
 obj=OUT/(Path(source).stem+('_test' if source.startswith('tests/') else '')+'.obj');objects.append(obj)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/DBH_SCRIPT_WAIT_STANDALONE','/Fo'+str(obj),ROOT/source])
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/map:'+str(OUT/'script-wait.map'),'/out:'+str(OUT/'bhunter-script-wait-check.exe'),*objects,'kernel32.lib'])
p=run([OUT/'bhunter-script-wait-check.exe']);result['stdout']=p.stdout
negative=run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(OUT/'missing-script-wait.exe'),*objects[1:],'kernel32.lib'],False)
assert negative.returncode and 'bh_script_wait_template' in negative.stdout and 'bh_script_wait_child' in negative.stdout
result['missing_module_link_control']=True;result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n');print(p.stdout)
