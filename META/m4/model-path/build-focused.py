from pathlib import Path
import os,subprocess,json,hashlib
root=Path.cwd(); out=root/'META/m4/model-path'; build=root/'build/model-path';build.mkdir(exist_ok=True)
env=os.environ.copy()
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC');sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');cl=root/'analysis/toolchain-sp3/bin/CL.EXE';sdk=Path('D:/Decomp/dx7sdk')
env['PATH']=str(cl.parent)+';D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN;'+env['PATH'];env['INCLUDE']=str(sp3/'include')+';'+str(vc/'include');env['LIB']=str(sp3/'lib')+';'+str(vc/'lib')
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
result={'target':'model-path.exe','original_code_required':False,'commands':[],'passed':False}
def run(args):
 p=subprocess.run([str(x) for x in args],cwd=build,env=env,text=True,capture_output=True);result['commands'].append({'argv':[str(x) for x in args],'exit_code':p.returncode,'output':p.stdout+p.stderr});print(p.stdout+p.stderr);return p.returncode
try:
 for source in ['src/bh_model_path.c','tests/model_path.c']:
  assert not run([cl,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/Fo'+str(build/(Path(source).stem+'.obj')),root/source])
 objects=list((root/'build/m0').glob('bh_*.obj'))+list(build.glob('*.obj'))
 result['linked_objects']={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in objects}
 assert not run([vc/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(build/'model-path.map'),'/out:'+str(build/'model-path.exe'),*objects,sdk/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
 assert not run([build/'model-path.exe'])
 result['passed']=True
finally:
 result['sources']={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in ['src/bh_model_path.c','src/bh_model_path.h','tests/model_path.c']}
 (out/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n')
