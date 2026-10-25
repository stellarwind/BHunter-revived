"""Build/run offscreen weapon HUD composition using real source definitions."""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/weapon-arc-lifecycle';OUT.mkdir(parents=True,exist_ok=True)
COMPILER=ROOT/'analysis/toolchain-sp3/bin';VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN');SDK=Path('D:/Decomp/dx7sdk')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
env=os.environ.copy()
for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC));env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result=dict(target='build/weapon-arc-lifecycle/bhunter-weapon-arc-lifecycle.exe',passed=False,
    original_code_required=False,gameplay_executed=False,sources={},dependency_objects={},commands=[])
def run(label,argv):
    p=subprocess.run([str(x) for x in argv],cwd=OUT,env=env,capture_output=True,text=True)
    output=p.stdout+p.stderr;(OUT/(label+'.txt')).write_text(output)
    result['commands'].append(dict(label=label,argv=[str(x) for x in argv],exit_code=p.returncode,output=output))
    print(output,end='')
    if p.returncode:raise RuntimeError(label+' failed')
    return p
try:
    compiled=[]
    for source,object_name,defines in [('src/bh_weapon_arc_lifecycle.c','bh_weapon_arc_lifecycle.obj',[]),('tests/weapon_arc_lifecycle.c','weapon_arc_lifecycle_tests.obj',['/DBH_WEAPON_ARC_LIFECYCLE_STANDALONE'])]:
        result['sources'][source]=digest(ROOT/source);compiled.append(OUT/object_name)
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',
            '/I'+str(SDK/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*defines,
            '/Fo'+str(OUT/object_name),ROOT/source])
    objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.stem!='bh_weapon_arc_lifecycle']
    result['dependency_objects']={str(p.relative_to(ROOT)):digest(p) for p in objects}
    run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(OUT/'bhunter-weapon-arc-lifecycle.map'),
        '/out:'+str(OUT/'bhunter-weapon-arc-lifecycle.exe'),*compiled,*objects,SDK/'lib/ddraw.lib',
        'kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
    for mode,args in [('clean',[])]:
        p=run('scenario-'+mode,[OUT/'bhunter-weapon-arc-lifecycle.exe',*args])
        result['test_'+mode]=json.loads(p.stdout.splitlines()[-1]);assert not result['test_'+mode]['failures']
        result['pixels_'+mode]={p.name:dict(bytes=p.stat().st_size,sha256=digest(p)) for p in OUT.glob('weapon-arc-lifecycle-*.bmp')}
    result['passed']=True
finally:
    (HERE/'focused-result.json').write_text(json.dumps(result,indent=2)+'\n')
