import os,json,hashlib,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[4]
here=Path(__file__).resolve().parent
out=root/'build/target600-c5';out.mkdir(parents=True,exist_ok=True)
vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
env=os.environ.copy();env['INCLUDE']=str(vc/'include');env['LIB']=str(vc/'lib');env['PATH']=str(root/'analysis/toolchain-sp3/bin')+';'+str(vc/'bin')+';'+env['PATH']
commands=[]
def run(cmd,ok=True):
 p=subprocess.run(list(map(str,cmd)),env=env,cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
 commands.append(dict(command=list(map(str,cmd)),exit_code=p.returncode,output=p.stdout))
 if ok:assert p.returncode==0,p.stdout
 return p
sources=['src/bh_network_debug.c','src/bh_gdi_text.c','src/bh_draw_queue.c','tests/network_debug.c']
objects=[]
for source in sources:
 obj=out/(Path(source).stem+'.obj');objects.append(obj)
 run([root/'analysis/toolchain-sp3/bin/CL.EXE','/nologo','/c','/ML','/O2','/G5','/Gy','/ID:/Decomp/dx7sdk/include','/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/DBH_NETWORK_DEBUG_STANDALONE','/Fo'+str(obj),source])
fresh={p.stem for p in objects};retained=[];linked_existing_objects={}
depdir=out/'admitted-dependencies';depdir.mkdir(exist_ok=True)
for p in (root/'build/m0').glob('bh_*.obj'):
 if p.stem in fresh or not (root/'src'/(p.stem+'.c')).is_file():continue
 body=p.read_bytes();copy=depdir/p.name;copy.write_bytes(body);retained.append(copy);linked_existing_objects[str(p.relative_to(root))]=hashlib.sha256(body).hexdigest()
objects+=retained
link=[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref']
exe=out/'bhunter-network-debug-check.exe';run(link+['/out:'+str(exe)]+objects+['kernel32.lib','user32.lib','dsound.lib','ole32.lib','winmm.lib','gdi32.lib','D:/Decomp/dx7sdk/lib/ddraw.lib','D:/Decomp/dx7sdk/lib/dinput.lib']);p=run([exe]);result=json.loads(p.stdout)
control=run(link+['/out:'+str(out/'missing.exe')]+objects[1:]+['kernel32.lib','user32.lib','dsound.lib','ole32.lib','winmm.lib','gdi32.lib','D:/Decomp/dx7sdk/lib/ddraw.lib','D:/Decomp/dx7sdk/lib/dinput.lib'],False);assert control.returncode!=0
result.update(linked_existing_objects=linked_existing_objects,retained_object_limit='Exact retained admitted object hashes recorded; parent fresh full integration remains required.',target=exe.name,commands=commands,missing_module_link_control=True,original_code_executed=False,sources={s:hashlib.sha256((root/s).read_bytes()).hexdigest() for s in sources})
(here/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n');print(p.stdout)
