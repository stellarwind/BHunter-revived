"""Focused real-TU source-only route-advance compilation and execution."""
import json,os,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[3];folder=Path(__file__).resolve().parent
out=root/'build/camera-route';out.mkdir(parents=True,exist_ok=True)
manifest=json.loads((root/'META/reference-manifest.json').read_text());roots={k:Path(v) for k,v in manifest['roots'].items()}
env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env['PATH']])
env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
commands=[]
def run(args):
 p=subprocess.run([str(a) for a in args],cwd=out,env=env,capture_output=True,text=True)
 commands.append(dict(args=[str(a) for a in args],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:print(p.stdout,p.stderr);raise RuntimeError(args)
 return p.stdout
def compile(source,name,macro=None):
 run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*(['/D'+macro] if macro else []),'/Fo'+str(out/name),root/source])
def link(name,extras):
 objects=[p for p in (root/'build/m0').glob('bh_*.obj') if p.name not in ('bh_camera_route.obj','bh_camera_curve.obj')]
 run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/(name+'.map')),'/out:'+str(out/(name+'.exe')),*objects,out/'bh_camera_route.obj',out/'bh_camera_curve.obj',*[out/e for e in extras],'D:/Decomp/dx7sdk/lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
try:
 compile('src/bh_camera_route.c','bh_camera_route.obj');compile('src/bh_camera_curve.c','bh_camera_curve.obj')
 compile('tests/camera_route.c','camera_route_main.obj','BH_CAMERA_ROUTE_STANDALONE')
 link('camera_route',['camera_route_main.obj']);print(run([out/'camera_route.exe']))
 compile('tests/camera_route.c','camera_route_tests.obj')
 compile('tests/camera_route_real.c','camera_route_real_tests.obj','BH_CAMERA_ROUTE_REAL_STANDALONE')
 link('camera_route_real',['camera_route_tests.obj','camera_route_real_tests.obj']);print(run([out/'camera_route_real.exe',root/'GAME_DATA/Bhunter.bin']))
finally:
 (folder/'standalone.json').write_text(json.dumps(dict(commands=commands,passed=bool(commands) and all(c['exit_code']==0 for c in commands),original_code_executed=False,game_launches=0),indent=2)+'\n')
