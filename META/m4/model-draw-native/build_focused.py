"""Compose actual native model drawing using snapshotted current m0 objects."""
import hashlib,json,os,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];EVIDENCE=Path(__file__).resolve().parent
OUT=ROOT/'build/model-draw-native';OUT.mkdir(parents=True,exist_ok=True)
COMPILER=ROOT/'analysis/toolchain-sp3/bin';VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');SDK=Path('D:/Decomp/dx7sdk')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
env=os.environ.copy()
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC));env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result=dict(target='bhunter-model-draw-native.exe',passed=False,commands=[],objects={},excluded_stale_objects=[],original_code_required=False,game_launches=0)
def run(label,args,success=True):
    p=subprocess.run([str(a) for a in args],cwd=OUT,env=env,capture_output=True,text=True)
    result['commands'].append(dict(label=label,argv=[str(a) for a in args],exit_code=p.returncode,output=p.stdout+p.stderr));print(label,p.returncode)
    if success and p.returncode:raise RuntimeError(p.stdout+p.stderr)
    return p
local=['src/bh_model_draw.c','src/bh_model_draw_lit.c','tests/model_draw_native.c']
try:
    objects=[];exclude={Path(s).stem+'.obj' for s in local}
    for p in sorted((ROOT/'build/m0').glob('bh_*.obj')):
        if p.name in exclude:continue
        if not (ROOT/'src'/f'{p.stem}.c').exists():
            result['excluded_stale_objects'].append(str(p.relative_to(ROOT)));continue
        dst=OUT/p.name;before=sha(p);shutil.copyfile(p,dst);assert sha(p)==before==sha(dst)
        result['objects'][p.name]=dict(sha256=before,origin=str(p.relative_to(ROOT)),source='src/'+p.stem+'.c',source_sha256=sha(ROOT/'src'/f'{p.stem}.c'))
        objects.append(dst)
    assert objects
    for source in local:
        obj=OUT/(Path(source).stem+'.obj');objects.append(obj)
        flags=['/DBH_MODEL_DRAW_NATIVE_STANDALONE'] if source.startswith('tests/') else []
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(SDK/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',*flags,'/Fo'+str(obj),ROOT/source])
    linker=VC/'bin/LINK.EXE';link=[linker,'/nologo','/subsystem:console']
    libraries=[SDK/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib']
    run('link',[*link,'/map:'+str(OUT/'bhunter-model-draw-native.map'),'/out:'+str(OUT/'bhunter-model-draw-native.exe'),*objects,*libraries])
    p=run('native-scenario',[OUT/'bhunter-model-draw-native.exe']);result['scenario']=json.loads(p.stdout.splitlines()[-1]);assert not result['scenario']['failures']
    for owner in ('bh_model_draw.obj','bh_model_draw_lit.obj','bh_native_primitive.obj'):
        control=run('missing-'+Path(owner).stem,[*link,'/out:'+str(OUT/('missing-'+Path(owner).stem+'.exe')),*[p for p in objects if p.name!=owner],*libraries],False)
        assert control.returncode and 'LNK2001' in control.stdout
    result['sources']={s:sha(ROOT/s) for s in local+['src/bh_model_draw.h','src/bh_model_draw_lit.h','src/bh_native_primitive.h']}
    result['bitmaps']={}
    for p in sorted(OUT.glob('model-native-*.bmp')):
        shutil.copyfile(p,EVIDENCE/p.name);result['bitmaps'][p.name]=sha(p)
    assert len(result['bitmaps'])==5
    shutil.copyfile(OUT/'bhunter-model-draw-native.map',EVIDENCE/'link-map.txt')
    result['executable_sha256']=sha(OUT/'bhunter-model-draw-native.exe');result['passed']=True
finally:
    (EVIDENCE/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
