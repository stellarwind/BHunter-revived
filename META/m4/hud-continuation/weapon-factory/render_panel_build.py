"""Compile and run original-asset panel through whole source renderer owner."""
from pathlib import Path
import hashlib,json,os,struct,subprocess
ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/render-execute-panel';OUT.mkdir(parents=True,exist_ok=True)
COMPILER=ROOT/'analysis/toolchain-sp3/bin'
VC=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
SP3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
SHARED=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
SDK=Path('D:/Decomp/dx7sdk')
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def bmp_delta(first,second):
    a,b=first.read_bytes(),second.read_bytes()
    ao,bo=struct.unpack_from('<I',a,10)[0],struct.unpack_from('<I',b,10)[0]
    width,height=struct.unpack_from('<ii',a,18)
    assert (width,height)==(640,480) and struct.unpack_from('<ii',b,18)==(width,height)
    stride=(width*3+3)&~3
    points=[(x,height-1-y) for y in range(height) for x in range(width)
            if a[ao+y*stride+x*3:ao+y*stride+x*3+3]!=
               b[bo+y*stride+x*3:bo+y*stride+x*3+3]]
    assert points
    return dict(changed_pixels=len(points),bounds=[min(x for x,y in points),
                min(y for x,y in points),max(x for x,y in points),
                max(y for x,y in points)])
env=os.environ.copy()
for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
env['PATH']=os.pathsep.join([str(COMPILER),str(SHARED),env.get('PATH','')])
env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (SP3,VC))
env['LIB']=os.pathsep.join(str(p/'lib') for p in (SP3,VC))
result=dict(target='build/render-execute-panel/bhunter-render-execute-panel.exe',passed=False,
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
    for source,object_name in [('src/bh_render_execute.c','bh_render_execute.obj'),
                               ('src/bh_frame_render.c','bh_frame_render.obj'),
                               ('src/bh_message_panel.c','bh_message_panel.obj'),
                               ('src/bh_messages.c','bh_messages.obj'),
                               ('tests/render_execute_panel.c','render_execute_panel_tests.obj')]:
        result['sources'][source]=digest(ROOT/source);compiled.append(OUT/object_name)
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',
            '/I'+str(SDK/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',
            *(['/DBH_RENDER_EXECUTE_PANEL_STANDALONE'] if source.startswith('tests/') else []),
            '/Fo'+str(OUT/object_name),ROOT/source])
    exclude={'bh_render_execute','bh_frame_render','bh_message_panel','bh_messages'}
    objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj')
             if p.name.count('.')==1 and not p.stem.endswith('_agent') and p.stem not in exclude]
    result['dependency_objects']={str(p.relative_to(ROOT)):digest(p) for p in objects}
    run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console',
        '/map:'+str(OUT/'bhunter-render-execute-panel.map'),
        '/out:'+str(OUT/'bhunter-render-execute-panel.exe'),
        *compiled,*objects,SDK/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
    archive=ROOT/'GAME_DATA/Bhunter.bin';result['archive_sha256']=digest(archive)
    p=run('scenario',[OUT/'bhunter-render-execute-panel.exe',archive])
    result['test']=json.loads(p.stdout.splitlines()[-1]);assert not result['test']['failures']
    result['outputs']={name:digest(OUT/name) for name in
                       ('render-execute-panel-blank.bmp',
                        'render-execute-panel-blank-primed.bmp',
                        'render-execute-panel-typed.bmp')}
    result['blank_to_typed']=bmp_delta(OUT/'render-execute-panel-blank-primed.bmp',
                                        OUT/'render-execute-panel-typed.bmp')
    result['passed']=True
finally:
    (HERE/'render-panel-result.json').write_text(json.dumps(result,indent=2)+'\n')
