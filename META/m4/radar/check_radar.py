"""Compile complete radar owner and corrected real rendering dependencies."""
import hashlib,json,os,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
out=root/'build/radar';out.mkdir(parents=True,exist_ok=True)
roots={k:Path(v) for k,v in json.loads((root/'META/reference-manifest.json').read_text())['roots'].items()}
env=os.environ.copy()
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env['PATH']]);env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'));env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
commands=[]
def run(args):
 p=subprocess.run([str(a) for a in args],env=env,cwd=out,capture_output=True,text=True)
 commands.append(dict(args=[str(a) for a in args],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:raise RuntimeError(p.stdout+p.stderr)
 return p.stdout
flags=['/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500']
sources=['src/bh_radar.c','src/bh_radar.h','src/bh_pickup_draw.c','src/bh_pickup_draw.h','tests/radar.c','tests/radar_native.c']
try:
 for unit in ('radar','pickup_draw'):
  run([roots['compiler']/'CL.EXE',*flags,'/Fo'+str(out/('bh_'+unit+'.obj')),root/('src/bh_'+unit+'.c')])
 for name in ('radar','radar_native'):
  run([roots['compiler']/'CL.EXE',*flags,'/DBH_'+name.upper()+'_STANDALONE','/Fo'+str(out/(name+'_tests.obj')),root/('tests/'+name+'.c')])
  objects=sorted(p for p in (root/'build/m0').glob('bh_*.obj') if p.name not in ('bh_pickup_draw.obj','bh_radar.obj'))
  run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/(name+'.map')),'/out:'+str(out/(name+'.exe')),*objects,out/'bh_pickup_draw.obj',out/'bh_radar.obj',out/(name+'_tests.obj'),'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','winmm.lib','dsound.lib','user32.lib','ole32.lib'])
  print(run([out/(name+'.exe'),*([root/'GAME_DATA/Bhunter.bin'] if name=='pickup_native' else [])]))
finally:
 (here/'radar-validation.json').write_text(json.dumps(dict(commands=commands,passed=bool(commands) and len(commands)==8 and all(c['exit_code']==0 for c in commands),source_sha256={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources},original_code_executed=False,game_launches=0),indent=2)+'\n')
