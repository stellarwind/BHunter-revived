"""Compile the real reticle TU and link a focused source-only checkpoint.

Dependency objects are the current real m0 source module objects, recorded by
hash. This does not replace the enclosing full/clean integration admission.
"""
from pathlib import Path
import hashlib,json,os,subprocess
ROOT=Path(__file__).resolve().parents[4];HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/weapon-hud-target';OUT.mkdir(parents=True,exist_ok=True)
COMPILER=ROOT/'analysis/toolchain-sp3/bin'
VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
SDK=Path('D:/Decomp/dx7sdk')
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
env=os.environ.copy()
for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(root/'include') for root in (SP3,VC))
env['LIB']=os.pathsep.join(str(root/'lib') for root in (SP3,VC))
result=dict(target='build/weapon-hud-target/bhunter-weapon-hud-target.exe',passed=False,
    original_code_required=False,commands=[],sources={},dependency_objects={})
def run(label,argv):
    p=subprocess.run([str(x) for x in argv],cwd=OUT,env=env,capture_output=True,text=True)
    output=p.stdout+p.stderr;(OUT/(label+'.txt')).write_text(output)
    result['commands'].append(dict(label=label,argv=[str(x) for x in argv],exit_code=p.returncode,output=output))
    if p.returncode:raise RuntimeError(label+' failed: '+output)
    return p
try:
    for source,object_name,defines in [
        ('src/bh_weapon_hud_target.c','bh_weapon_hud_target.obj',[]),
        ('tests/weapon_hud_target.c','weapon_hud_target_tests.obj',['/DBH_WEAPON_HUD_TARGET_STANDALONE'])]:
        result['sources'][source]=digest(ROOT/source)
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',
            '/I'+str(SDK/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*defines,
            '/Fo'+str(OUT/object_name),ROOT/source])
    for source in ['src/bh_weapon_hud_target.h','src/bh_weapon_hud_state.h']:
        result['sources'][source]=digest(ROOT/source)
    objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj') if p.stem!='bh_weapon_hud_target']
    assert objects
    result['dependency_objects']={str(p.relative_to(ROOT)):digest(p) for p in objects}
    run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(OUT/'bhunter-weapon-hud-target.map'),
        '/out:'+str(OUT/'bhunter-weapon-hud-target.exe'),OUT/'bh_weapon_hud_target.obj',OUT/'weapon_hud_target_tests.obj',
        *objects,SDK/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
    p=run('scenario',[OUT/'bhunter-weapon-hud-target.exe'])
    result['test']=json.loads(p.stdout.splitlines()[-1]);assert result['test']['failures']==0
    result['passed']=True;print(p.stdout,end='')
finally:
    (HERE/'focused-result.json').write_text(json.dumps(result,indent=2)+'\n')
