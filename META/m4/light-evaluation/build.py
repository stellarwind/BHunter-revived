"""Focused named source-only target; no original executable code is loaded."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'build/light-evaluation';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
COMPILER=ROOT/'analysis/toolchain-sp3/bin'
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=os.pathsep.join([str(COMPILER),'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN',env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC))
env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result={'target':'bhunter-light-evaluation.exe','passed':False,'original_code_executed':False,'game_launches':0,'commands':[],'sources':{}}
def run(label,args):
 p=subprocess.run([str(a) for a in args],cwd=OUT,env=env,capture_output=True,text=True)
 result['commands'].append({'label':label,'argv':[str(a) for a in args],'returncode':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
 if p.returncode:raise RuntimeError(label+'\n'+p.stdout+p.stderr)
 return p
try:
 objects=[]
 for name in ('src/bh_light_eval.c','src/bh_light.c','src/bh_math.c','src/bh_round.c','src/bh_fp.c','tests/light_eval.c'):
  src=ROOT/name;obj=OUT/(src.stem+('_test' if name.startswith('tests/') else '')+'.obj');objects.append(obj)
  result['sources'][name]=hashlib.sha256(src.read_bytes()).hexdigest()
  run('compile-'+name,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/DBH_LIGHT_EVAL_STANDALONE','/Fo'+str(obj),src])
 run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(OUT/'bhunter-light-evaluation.map'),'/out:'+str(OUT/'bhunter-light-evaluation.exe'),*objects,'kernel32.lib'])
 p=run('focused-checks',[OUT/'bhunter-light-evaluation.exe']);print(p.stdout,end='');result['test']=json.loads(p.stdout.splitlines()[-1]);result['passed']=not result['test']['failures']
finally:
 (ROOT/'META/m4/light-evaluation/focused-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n')
