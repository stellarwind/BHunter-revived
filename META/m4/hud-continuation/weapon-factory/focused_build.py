"""Compile real weapon factory TU, link real source owners, run source checks."""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/weapon-factory';OUT.mkdir(parents=True,exist_ok=True)
COMPILER=ROOT/'analysis/toolchain-sp3/bin'
VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
SDK=Path('D:/Decomp/dx7sdk')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
env=os.environ.copy()
for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC))
env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result=dict(target='build/weapon-factory/bhunter-weapon-factory.exe',passed=False,
            original_code_required=False,gameplay_executed=False,sources={},commands=[])
def run(label,argv):
    p=subprocess.run([str(x) for x in argv],cwd=OUT,env=env,capture_output=True,text=True)
    output=p.stdout+p.stderr;(OUT/(label+'.txt')).write_text(output)
    result['commands'].append(dict(label=label,argv=[str(x) for x in argv],exit_code=p.returncode,output=output))
    print(output,end='')
    if p.returncode:raise RuntimeError(label+' failed')
    return p
try:
    compiled=[]
    for source,object_name in [('src/bh_weapon_factory.c','bh_weapon_factory.obj'),
                               ('src/bh_weapon_timer.c','bh_weapon_timer.obj'),
                               ('tests/weapon_factory.c','weapon_factory_tests.obj')]:
        result['sources'][source]=digest(ROOT/source);compiled.append(OUT/object_name)
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',
            '/I'+str(SDK/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',
            *(['/DBH_WEAPON_FACTORY_STANDALONE'] if source.startswith('tests/') else []),
            '/Fo'+str(OUT/object_name),ROOT/source])
    objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj')
             if p.name.count('.')==1 and not p.stem.endswith('_agent') and
             p.stem not in ('bh_weapon_factory','bh_weapon_timer')]
    result['dependency_objects']={str(p.relative_to(ROOT)):digest(p) for p in objects}
    run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console',
        '/map:'+str(OUT/'bhunter-weapon-factory.map'),'/out:'+str(OUT/'bhunter-weapon-factory.exe'),
        *compiled,*objects,SDK/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
    p=run('scenario',[OUT/'bhunter-weapon-factory.exe'])
    result['test']=json.loads(p.stdout.splitlines()[-1]);assert not result['test']['failures']
    result['passed']=True
finally:
    (HERE/'focused-result.json').write_text(json.dumps(result,indent=2)+'\n')
