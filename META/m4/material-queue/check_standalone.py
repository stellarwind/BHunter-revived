"""Compile real translation units and link the real queue/image setter owners."""
import json,os,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
out=root/'build/material-queue';out.mkdir(parents=True,exist_ok=True)
roots={k:Path(v) for k,v in json.loads((root/'META/reference-manifest.json').read_text())['roots'].items()}
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env['PATH']])
env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
commands=[]
def run(args):
 p=subprocess.run([str(a) for a in args],cwd=out,env=env,capture_output=True,text=True)
 commands.append(dict(args=[str(a) for a in args],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:raise RuntimeError(p.stdout+p.stderr)
 return p.stdout
try:
 objects=[]
 for source in ['src/bh_material_queue.c','src/bh_draw_queue.c','src/bh_texture_policy.c','src/bh_clipper.c','src/bh_matrix_inverse.c','src/bh_math.c','src/bh_round.c','tests/material_queue.c']:
  obj=out/(Path(source).stem+'.obj');objects.append(obj)
  run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DBH_MATERIAL_QUEUE_STANDALONE','/Fo'+str(obj),root/source])
 run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/'material_queue.map'),'/out:'+str(out/'material_queue.exe'),*objects,'kernel32.lib'])
 print(run([out/'material_queue.exe']))
finally:
 (here/'standalone.json').write_text(json.dumps(dict(commands=commands,passed=bool(commands) and all(c['exit_code']==0 for c in commands),original_code_executed=False,game_launches=0),indent=2)+'\n')
