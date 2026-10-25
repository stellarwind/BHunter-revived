"""Compile real attachment TU and source dependencies; no original execution."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/hud-attachment/attachment';OUT.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
compiler=ROOT/'analysis/toolchain-sp3/bin';shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=dict(os.environ);env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join([str(sp3/'include'),str(vc/'include')]);env['LIB']=os.pathsep.join([str(sp3/'lib'),str(vc/'lib')])
sources=['src/bh_attachment.c','src/bh_draw_queue.c','src/bh_screen_quad.c','src/bh_screen_clipper.c','src/bh_round.c','src/bh_fp.c','tests/attachment.c']
result=dict(target='bhunter-attachment-check.exe',original_code_executed=False,passed=False,commands=[],sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources})
def run(command):
 p=subprocess.run([str(x) for x in command],env=env,cwd=OUT,capture_output=True,text=True)
 result['commands'].append(dict(command=[str(x) for x in command],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:
  (HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 return p
objects=[]
for source in sources:
 obj=OUT/(Path(source).stem+('_test' if source.startswith('tests/') else '')+'.obj');objects.append(obj)
 flags=['/DBH_ATTACHMENT_STANDALONE'] if source.startswith('tests/') else []
 run([compiler/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*flags,'/Fo'+str(obj),ROOT/source])
run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(OUT/'bhunter-attachment-check.map'),'/out:'+str(OUT/'bhunter-attachment-check.exe'),*objects])
p=run([OUT/'bhunter-attachment-check.exe']);result['scenario']=json.loads(p.stdout);result['passed']=True
(HERE/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout)
