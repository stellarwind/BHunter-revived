"""Compile and run complete integer software tables with real dependencies."""
from pathlib import Path
import os,subprocess,json,hashlib
ROOT=Path(__file__).resolve().parents[3];OUT=ROOT/'META/m4/software-tables';BUILD=ROOT/'build/software-tables';BUILD.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');cl=ROOT/'analysis/toolchain-sp3/bin/CL.EXE';sdk=Path('D:/Decomp/dx7sdk')
env['PATH']=os.pathsep.join([str(cl.parent),'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN',env['PATH']]);env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (sp3,vc));env['LIB']=os.pathsep.join(str(p/'lib') for p in (sp3,vc))
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
result={'target':'bhunter-software-tables.exe','original_code_required':False,'game_launches':0,'commands':[],'passed':False,'sources':{}}
def run(label,args):
 p=subprocess.run([str(x) for x in args],cwd=BUILD,env=env,text=True,capture_output=True);result['commands'].append({'label':label,'argv':[str(x) for x in args],'exit_code':p.returncode,'stdout':p.stdout,'stderr':p.stderr})
 if p.returncode:raise RuntimeError(label+'\n'+p.stdout+p.stderr)
 return p
try:
 objects=[]
 for source in ['src/bh_software_tables.c','src/bh_texture_tables.c','tests/software_tables.c']:
  obj=BUILD/(Path(source).stem+'.obj');objects.append(obj);result['sources'][source]=hashlib.sha256((ROOT/source).read_bytes()).hexdigest()
  run('compile-'+source,[cl,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DBH_SOFTWARE_TABLES_STANDALONE','/Fo'+str(obj),ROOT/source])
 run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(BUILD/'bhunter-software-tables.map'),'/out:'+str(BUILD/'bhunter-software-tables.exe'),*objects,'kernel32.lib'])
 p=run('exhaustive-checks',[BUILD/'bhunter-software-tables.exe']);print(p.stdout,end='');result['test']=json.loads(p.stdout.splitlines()[-1]);result['passed']=not result['test']['failures']
finally:
 (OUT/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n',newline='\n')
