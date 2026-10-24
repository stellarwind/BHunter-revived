"""Link and exercise the entity resource draw owner with real queue producers."""
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
EVIDENCE=Path(__file__).resolve().parent
OUT=ROOT/'build/m4/entity-resource-draw'
OUT.mkdir(parents=True,exist_ok=True)
COMPILER=ROOT/'analysis/toolchain-sp3/bin'
VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
SDK=Path('D:/Decomp/dx7sdk')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
env=os.environ.copy()
for key in ('CL','_CL_','LINK','_LINK_'):env.pop(key,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC))
env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result=dict(target='entity-resource-draw-test.exe',passed=False,commands=[],objects={},
            original_code_required=False,game_launches=0)
def run(label,args,required=True):
    process=subprocess.run([str(a) for a in args],cwd=OUT,env=env,
                           capture_output=True,text=True)
    result['commands'].append(dict(label=label,argv=[str(a) for a in args],
                                   exit_code=process.returncode,
                                   output=process.stdout+process.stderr))
    print(label,process.returncode)
    if required and process.returncode:raise RuntimeError(process.stdout+process.stderr)
    return process
try:
    objects=[]
    for source in sorted((ROOT/'build/m0').glob('bh_*.obj')):
        if source.stem in ('bh_draw_queue','bh_draw_submit','bh_entity_resource_draw'):
            continue
        if not (ROOT/'src'/f'{source.stem}.c').exists():continue
        target=OUT/source.name
        before=sha(source)
        shutil.copyfile(source,target)
        assert sha(source)==before==sha(target)
        result['objects'][source.name]=dict(origin=str(source.relative_to(ROOT)),
            sha256=before,source='src/'+source.stem+'.c',
            source_sha256=sha(ROOT/'src'/f'{source.stem}.c'))
        objects.append(target)
    assert objects
    for source in ('src/bh_draw_queue.c','src/bh_draw_submit.c',
                   'src/bh_entity_resource_draw.c','tests/entity_resource_draw.c'):
        target=OUT/(Path(source).stem+'.obj')
        flags=['/DBH_ENTITY_RESOURCE_DRAW_STANDALONE'] if source.startswith('tests/') else []
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c',
            '/ML','/O2','/G5','/Oi','/Gy','/I'+str(SDK/'include'),
            '/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',
            *flags,'/Fo'+str(target),ROOT/source])
        objects.append(target)
    linker=VC/'bin/LINK.EXE'
    libraries=[SDK/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib']
    base=[linker,'/nologo','/subsystem:console']
    run('link',[*base,'/map:'+str(OUT/'entity-resource-draw.map'),
        '/out:'+str(OUT/'entity-resource-draw-test.exe'),*objects,*libraries])
    process=run('scenario',[OUT/'entity-resource-draw-test.exe'])
    result['scenario']=json.loads(process.stdout.splitlines()[-1])
    assert result['scenario']['failures']==0
    owner=OUT/'bh_entity_resource_draw.obj'
    control=run('missing-owner',[*base,
        '/out:'+str(OUT/'missing-owner.exe'),
        *[p for p in objects if p!=owner],*libraries],False)
    assert control.returncode and 'LNK2001' in control.stdout
    result['sources']={name:sha(ROOT/name) for name in
        ('src/bh_entity_resource_draw.c','src/bh_entity_resource_draw.h',
         'tests/entity_resource_draw.c','src/bh_draw_queue.c',
         'src/bh_draw_submit.c')}
    shutil.copyfile(OUT/'entity-resource-draw.map',EVIDENCE/'link-map.txt')
    result['executable_sha256']=sha(OUT/'entity-resource-draw-test.exe')
    result['passed']=True
finally:
    (EVIDENCE/'focused-build.json').write_text(json.dumps(result,indent=2)+'\n',
                                                encoding='utf-8',newline='\n')
