"""Compile full flyby owner and its real newly restored dependencies."""
import json,os,subprocess
from pathlib import Path
folder=Path(__file__).resolve().parent;root=folder.parents[2];out=root/'build/flyby-camera';out.mkdir(parents=True,exist_ok=True)
manifest=json.loads((root/'META/reference-manifest.json').read_text());roots={k:Path(v) for k,v in manifest['roots'].items()}
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env['PATH']]);env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'));env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
commands=[]
def run(args):
 p=subprocess.run([str(a) for a in args],cwd=out,env=env,capture_output=True,text=True)
 commands.append(dict(args=[str(a) for a in args],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:print(p.stdout,p.stderr);raise RuntimeError(args)
 return p.stdout
try:
 names=['bh_flyby_camera','bh_camera_route','bh_camera_curve','bh_camera_modes','bh_camera_state','bh_atlas_text','bh_text_queue']
 flags=['/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500']
 for name in names:
  run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/Fo'+str(out/(name+'.obj')),root/('src/'+name+'.c')])
 run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/DBH_FLYBY_CAMERA_STANDALONE','/Fo'+str(out/'flyby_camera_tests.obj'),root/'tests/flyby_camera.c'])
 objects=[p for p in (root/'build/m0').glob('bh_*.obj') if p.stem not in names]
 run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/'flyby_camera.map'),'/out:'+str(out/'flyby_camera.exe'),*objects,*[out/(n+'.obj') for n in names],out/'flyby_camera_tests.obj','D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
 print(run([out/'flyby_camera.exe']))
finally:
 (folder/'standalone.json').write_text(json.dumps(dict(commands=commands,passed=bool(commands) and all(c['exit_code']==0 for c in commands),original_code_executed=False,game_launches=0),indent=2)+'\n')
