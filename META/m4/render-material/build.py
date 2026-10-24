"""Focused real native material target; current m0 real owner objects required."""
from pathlib import Path
import os,subprocess,json,hashlib
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'META/m4/render-material';BUILD=ROOT/'build/render-material';BUILD.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');cl=ROOT/'analysis/toolchain-sp3/bin/CL.EXE';sdk=Path('D:/Decomp/dx7sdk')
env['PATH']=os.pathsep.join([str(cl.parent),'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN',env['PATH']]);env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (sp3,vc));env['LIB']=os.pathsep.join(str(p/'lib') for p in (sp3,vc))
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
result={'target':'bhunter-render-material.exe','original_code_required':False,'game_launches':0,'commands':[],'passed':False}
def run(label,args):
 p=subprocess.run([str(x) for x in args],cwd=BUILD,env=env,text=True,capture_output=True);result['commands'].append({'label':label,'argv':[str(x) for x in args],'exit_code':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
 if p.returncode:raise RuntimeError(label+'\n'+p.stdout+p.stderr)
 return p
try:
 for source in ['src/bh_render_material.c','tests/render_material.c']:
  run('compile-'+source,[cl,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DBH_RENDER_MATERIAL_STANDALONE','/Fo'+str(BUILD/(Path(source).stem+'.obj')),ROOT/source])
 objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.stem!='bh_render_material']+list(BUILD.glob('*.obj'))
 result['linked_objects']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in objects}
 run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(BUILD/'bhunter-render-material.map'),'/out:'+str(BUILD/'bhunter-render-material.exe'),*objects,sdk/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
 p=run('native-checks',[BUILD/'bhunter-render-material.exe']);print(p.stdout,end='');result['test']=json.loads(p.stdout.splitlines()[-1]);result['passed']=not result['test']['failures']
finally:
 result['sources']={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in ['src/bh_render_material.c','src/bh_render_material.h','tests/render_material.c']}
 (OUT/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n')
