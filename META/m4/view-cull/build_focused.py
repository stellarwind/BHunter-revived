"""Build and run the source-only view cull contract target."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'build/view-cull';OUT.mkdir(parents=True,exist_ok=True)
EVIDENCE=Path(__file__).resolve().parent
COMPILER=ROOT/'analysis/toolchain-sp3/bin'
VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
env=os.environ.copy()
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC))
env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result=dict(target='bhunter-view-cull.exe',passed=False,commands=[],original_code_required=False,game_launches=0)
def run(label,args,success=True):
    p=subprocess.run([str(x) for x in args],cwd=OUT,env=env,capture_output=True,text=True)
    result['commands'].append(dict(label=label,argv=[str(x) for x in args],exit_code=p.returncode,output=p.stdout+p.stderr))
    print(label,p.returncode)
    if success and p.returncode:raise RuntimeError(p.stdout+p.stderr)
    return p
sources=['src/bh_view_cull.c','src/bh_main_camera.c','src/bh_view_camera.c','src/bh_math.c','src/bh_rotation.c','src/bh_round.c','src/bh_fp.c','tests/view_cull.c']
try:
    objects=[]
    for source in sources:
        obj=OUT/(Path(source).stem+'.obj');objects.append(obj)
        flags=['/DBH_VIEW_CULL_STANDALONE'] if source=='tests/view_cull.c' else []
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/Fo'+str(obj),ROOT/source])
    run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(OUT/'bhunter-view-cull.map'),'/out:'+str(OUT/'bhunter-view-cull.exe'),*objects,'kernel32.lib'])
    p=run('scenario',[OUT/'bhunter-view-cull.exe']);result['scenario']=json.loads(p.stdout.splitlines()[-1]);assert not result['scenario']['failures']
    control=run('missing-cull-owner-control',[VC/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(OUT/'missing-view-cull.exe'),*objects[1:],'kernel32.lib'],False)
    assert control.returncode and 'LNK2001' in control.stdout and '_bh_view_cull_sphere' in control.stdout
    result['sources']={s:hashlib.sha256((ROOT/s).read_bytes()).hexdigest() for s in sources+['src/bh_view_cull.h','src/bh_main_camera.h']}
    result['executable_sha256']=hashlib.sha256((OUT/'bhunter-view-cull.exe').read_bytes()).hexdigest()
    (EVIDENCE/'link-map.txt').write_bytes((OUT/'bhunter-view-cull.map').read_bytes())
    result['passed']=True
finally:
    (EVIDENCE/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
