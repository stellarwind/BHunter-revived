import hashlib,json,os,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[3]
OUT=ROOT/'build/script-lifetime';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')]);env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_script_lifetime.c','tests/script_lifetime.c','src/bh_script_timer.c','src/bh_sound_queue.c','src/bh_entities.c','src/bh_math.c','src/bh_fp.c','src/bh_round.c']
result=dict(target='bhunter-script-lifetime-check.exe',passed=False,original_code_executed=False,commands=[],sources={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources+['src/bh_script_lifetime.h']})
def run(argv,expect=True):
 p=subprocess.run([str(x) for x in argv],env=env,cwd=OUT,capture_output=True,text=True)
 result['commands'].append(dict(argv=[str(x) for x in argv],exit_code=p.returncode,output=p.stdout+p.stderr))
 if expect and p.returncode:
  (HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for s in sources:
 o=OUT/(Path(s).stem+('_test' if s.startswith('tests/') else '')+'.obj');objects.append(o)
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/DBH_SCRIPT_LIFETIME_STANDALONE','/Fo'+str(o),ROOT/s])
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/map:'+str(OUT/'script-lifetime.map'),'/out:'+str(OUT/'bhunter-script-lifetime-check.exe'),*objects,'kernel32.lib'])
p=run([OUT/'bhunter-script-lifetime-check.exe']);result['stdout']=p.stdout;print(p.stdout)
n=run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/out:'+str(OUT/'missing-lifetime.exe'),*objects[1:],'kernel32.lib'],False)
assert n.returncode and 'bh_script_sound_actor_query' in n.stdout
result['missing_module_link_control']=True;result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n')
