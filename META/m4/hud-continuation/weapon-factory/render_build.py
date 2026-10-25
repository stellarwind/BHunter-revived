"""Compile whole 00479090 owner, link real source dependencies, run offscreen check."""
from pathlib import Path
import hashlib,json,os,struct,subprocess
ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
OUT=ROOT/'build/render-execute';OUT.mkdir(parents=True,exist_ok=True)
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
result=dict(target='build/render-execute/bhunter-render-execute.exe',passed=False,
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
    sources=[('src/bh_render_execute.c','bh_render_execute.obj'),
             ('src/bh_frame_render.c','bh_frame_render.obj'),
             ('src/bh_message_panel.c','bh_message_panel.obj'),
             ('tests/render_execute.c','render_execute_tests.obj')]
    for source,object_name in sources:
        result['sources'][source]=digest(ROOT/source);compiled.append(OUT/object_name)
        run('compile-'+Path(source).stem,[COMPILER/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',
            '/I'+str(SDK/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500',
            *(['/DBH_RENDER_EXECUTE_STANDALONE'] if source.startswith('tests/') else []),
            '/Fo'+str(OUT/object_name),ROOT/source])
    exclude={'bh_render_execute','bh_frame_render','bh_message_panel'}
    objects=[p for p in (ROOT/'build/m0').glob('bh_*.obj')
             if p.name.count('.')==1 and not p.stem.endswith('_agent') and p.stem not in exclude]
    result['dependency_objects']={str(p.relative_to(ROOT)):digest(p) for p in objects}
    run('link',[VC/'bin/LINK.EXE','/nologo','/subsystem:console',
        '/map:'+str(OUT/'bhunter-render-execute.map'),'/out:'+str(OUT/'bhunter-render-execute.exe'),
        *compiled,*objects,SDK/'lib/ddraw.lib','kernel32.lib','dsound.lib','user32.lib','ole32.lib','winmm.lib'])
    p=run('scenario',[OUT/'bhunter-render-execute.exe'])
    result['test']=json.loads(p.stdout.splitlines()[-1]);assert not result['test']['failures']
    result['output_sha256']=digest(OUT/'render-execute-white.bmp')
    result['passed']=True
finally:
    (HERE/'render-result.json').write_text(json.dumps(result,indent=2)+'\n')
