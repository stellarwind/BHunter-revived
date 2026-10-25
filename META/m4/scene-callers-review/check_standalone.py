"""Compile real scene caller dependencies and link source-only focused targets."""
import json,os,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3]
out=root/'build/scene-callers-review';out.mkdir(parents=True,exist_ok=True)
manifest=json.loads((root/'META/reference-manifest.json').read_text());roots={k:Path(v) for k,v in manifest['roots'].items()}
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env['PATH']])
env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
commands=[]
def run(args):
 p=subprocess.run([str(a) for a in args],env=env,cwd=out,capture_output=True,text=True)
 commands.append(dict(args=[str(a) for a in args],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:print(p.stdout,p.stderr);raise RuntimeError(args)
 return p.stdout
try:
 for name in ('camera_queue','scene_background','camera_curve'):
  flags=['/I'+str(Path('D:/Decomp/dx7sdk/include')),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500']
  run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/Fo'+str(out/('bh_'+name+'.obj')),root/('src/bh_'+name+'.c')])
  run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/DBH_'+name.upper()+'_STANDALONE','/Fo'+str(out/(name+'_tests.obj')),root/('tests/'+name+'.c')])
  objects=[p for p in (root/'build/m0').glob('bh_*.obj') if p.name not in ('bh_camera_queue.obj','bh_scene_background.obj','bh_camera_curve.obj')]
  run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/(name+'.map')),'/out:'+str(out/(name+'.exe')),*objects,out/('bh_'+name+'.obj'),out/(name+'_tests.obj'),'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
  print(run([out/(name+'.exe')]))
 flags=['/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500']
 run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/DBH_SCENE_BACKGROUND_NATIVE_STANDALONE','/Fo'+str(out/'scene_background_native_tests.obj'),root/'tests/scene_background_native.c'])
 objects=[p for p in (root/'build/m0').glob('bh_*.obj') if p.name not in ('bh_camera_queue.obj','bh_scene_background.obj','bh_camera_curve.obj')]
 run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/'scene_background_native.map'),'/out:'+str(out/'scene_background_native.exe'),*objects,out/'bh_camera_queue.obj',out/'bh_scene_background.obj',out/'scene_background_native_tests.obj','D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
 print(run([out/'scene_background_native.exe',root/'GAME_DATA/Bhunter.bin']))
finally:
 (root/'META/m4/scene-callers-review/standalone.json').write_text(json.dumps(dict(commands=commands,passed=bool(commands) and all(c['exit_code']==0 for c in commands),original_code_executed=False,game_launches=0),indent=2)+'\n')
