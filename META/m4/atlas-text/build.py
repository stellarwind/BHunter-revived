"""Real native clear/cursor target, freshly compile page/atlas dependencies."""
from pathlib import Path
import os,subprocess,json,hashlib
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'META/m4/atlas-text';BUILD=ROOT/'build/atlas-text';BUILD.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');cl=ROOT/'analysis/toolchain-sp3/bin/CL.EXE';sdk=Path('D:/Decomp/dx7sdk')
env['PATH']=os.pathsep.join([str(cl.parent),'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN',env['PATH']]);env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (sp3,vc));env['LIB']=os.pathsep.join(str(p/'lib') for p in (sp3,vc))
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
result={'target':'bhunter-atlas-text.exe','original_code_required':False,'game_launches':0,'commands':[],'passed':False}
def run(label,args):
 p=subprocess.run([str(x) for x in args],cwd=BUILD,env=env,text=True,capture_output=True);result['commands'].append({'label':label,'argv':[str(x) for x in args],'exit_code':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
 if p.returncode:raise RuntimeError(label+'\n'+p.stdout+p.stderr)
 return p
try:
 for source in ['src/bh_atlas_text.c','tests/atlas_text.c']:
  run('compile-'+source,[cl,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DBH_ATLAS_TEXT_STANDALONE','/Fo'+str(BUILD/(Path(source).stem+'.obj')),ROOT/source])
 objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.stem not in ('bh_atlas_text',)]+list(BUILD.glob('*.obj'))
 result['linked_objects']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in objects}
 run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(BUILD/'bhunter-atlas-text.map'),'/out:'+str(BUILD/'bhunter-atlas-text.exe'),*objects,sdk/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
 p=run('native-clean-checks',[BUILD/'bhunter-atlas-text.exe']);print(p.stdout,end='');result['clean_test']=json.loads(p.stdout.splitlines()[-1]);p=run('native-real-checks',[BUILD/'bhunter-atlas-text.exe',ROOT/'GAME_DATA/Bhunter.bin']);print(p.stdout,end='');result['test']=json.loads(p.stdout.splitlines()[-1]);result['passed']=not result['test']['failures']
finally:
 result['sources']={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in ['src/bh_atlas_text.c','src/bh_atlas_text.h','tests/atlas_text.c','src/bh_atlas_glyph.c','src/bh_atlas_page.c','src/bh_atlas_image.c']}
 (OUT/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n')
