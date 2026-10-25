import json,os,subprocess
from pathlib import Path
folder=Path(__file__).resolve().parent;root=folder.parents[2];out=root/'build/horizon-color';out.mkdir(parents=True,exist_ok=True)
roots={k:Path(v)for k,v in json.loads((root/'META/reference-manifest.json').read_text())['roots'].items()};env=os.environ.copy()
for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env['PATH']]);env['INCLUDE']=os.pathsep.join(str(roots[k]/'include')for k in ('sp3','vc'));env['LIB']=os.pathsep.join(str(roots[k]/'lib')for k in ('sp3','vc'))
commands=[]
def run(args):
 p=subprocess.run([str(a)for a in args],cwd=out,env=env,capture_output=True,text=True);commands.append(dict(args=[str(a)for a in args],exit_code=p.returncode,stdout=p.stdout,stderr=p.stderr))
 if p.returncode:print(p.stdout,p.stderr);raise RuntimeError(args)
 return p.stdout
try:
 for source,obj,macro in [('src/bh_horizon_color.c','bh_horizon_color.obj',None),('src/bh_texture_tables.c','bh_texture_tables.obj',None),('tests/horizon_color.c','horizon_color_tests.obj','BH_HORIZON_COLOR_STANDALONE')]:
  run([roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*(['/D'+macro]if macro else []),'/Fo'+str(out/obj),root/source])
 run([roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/'horizon_color.map'),'/out:'+str(out/'horizon_color.exe'),out/'bh_horizon_color.obj',out/'bh_texture_tables.obj',out/'horizon_color_tests.obj'])
 print(run([out/'horizon_color.exe']))
finally:
 (folder/'standalone.json').write_text(json.dumps(dict(commands=commands,passed=bool(commands)and all(c['exit_code']==0 for c in commands),original_code_executed=False,game_launches=0),indent=2)+'\n')
