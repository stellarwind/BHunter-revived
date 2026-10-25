"""Reproduce the source-only route-curve-length harness and retain exact build evidence."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
OUT=ROOT/'build/route-curve-length'
SOURCE=['src/bh_route_curve_length.c','src/bh_route_curve_length.h','tests/route_curve_length.c','src/bh_entities.c','src/bh_entities.h','src/bh_fp.c','src/bh_fp.h','src/bh_round.c','src/bh_round.h','src/bh_math.h','src/bh_scene.h','src/bh_world_geometry.h','src/bh_geometry.h','src/bh_portals.h','src/bh_camera_curve.c','src/bh_camera_curve.h','tests/route_curve_length_cases.h']

def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    OUT.mkdir(parents=True,exist_ok=True)
    compiler=ROOT/'analysis/toolchain-sp3/bin'
    shared=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
    vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
    sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
    env=os.environ.copy()
    for name in ('CL','_CL_','LINK','_LINK_'):env.pop(name,None)
    env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
    env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (sp3,vc))
    env['LIB']=os.pathsep.join(str(p/'lib') for p in (sp3,vc))
    result=dict(target='route-curve-length.exe',passed=False,original_code_required=False,
                game_launches=0,sources={s:digest(ROOT/s) for s in SOURCE},commands=[])
    result['tools']={str(path):digest(path) for path in (compiler/'cl.exe',vc/'bin/LINK.EXE')}
    def run(label,argv,expect_success=True):
        p=subprocess.run([str(a) for a in argv],cwd=ROOT,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        result['commands'].append(dict(label=label,argv=[str(a) for a in argv],returncode=p.returncode,output=p.stdout))
        if expect_success and p.returncode:raise RuntimeError(label+' failed: '+p.stdout)
        if not expect_success:
            assert p.returncode!=0 and '_bh_route_curve_length' in p.stdout and 'unresolved external' in p.stdout
        return p.stdout
    try:
        flags=['/nologo','/c','/ML','/O2','/G5','/Oi','/Gy']
        run('compile-source',[compiler/'cl.exe',*flags,'/Fo'+str(OUT/'bh_route_curve_length.obj'),ROOT/SOURCE[0]])
        run('compile-tests',[compiler/'cl.exe',*flags,'/DBH_ROUTE_CURVE_LENGTH_STANDALONE','/Fo'+str(OUT/'route_curve_length_tests.obj'),ROOT/SOURCE[2]])
        for name in ('bh_entities','bh_fp','bh_round','bh_camera_curve'):
            run('compile-'+name,[compiler/'cl.exe',*flags,'/Fo'+str(OUT/(name+'.obj')),ROOT/('src/'+name+'.c')])
        run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/verbose',
            '/map:'+str(OUT/'route-curve-length.map'),'/out:'+str(OUT/'route-curve-length.exe'),
            OUT/'bh_route_curve_length.obj',OUT/'route_curve_length_tests.obj',OUT/'bh_entities.obj',OUT/'bh_fp.obj',OUT/'bh_round.obj',OUT/'bh_camera_curve.obj'])
        run('missing-target-module-control',[vc/'bin/LINK.EXE','/nologo','/subsystem:console',
            '/out:'+str(OUT/'missing-target-must-not-link.exe'),
            OUT/'route_curve_length_tests.obj',OUT/'bh_entities.obj',OUT/'bh_fp.obj',OUT/'bh_round.obj',OUT/'bh_camera_curve.obj'],False)
        output=run('source-only-checks',[OUT/'route-curve-length.exe'])
        result['scenario']=json.loads(output.splitlines()[-1])
        assert result['scenario']['scenario']=='route-curve-length'
        assert result['scenario']['checks']>0 and result['scenario']['failures']==0
        result['executable_sha256']=digest(OUT/'route-curve-length.exe')
        result['passed']=True
        result['environment']={name:env[name] for name in ('INCLUDE','LIB')}
        result['missing_module_control_passed']=True
        snapshot=FOLDER/'sources';snapshot.mkdir(exist_ok=True)
        for source,sha in result['sources'].items():
            (snapshot/(sha+'-'+Path(source).name+'.txt')).write_bytes((ROOT/source).read_bytes())
        (FOLDER/'curve-length-link-map.txt').write_bytes((OUT/'route-curve-length.map').read_bytes())
        print(output,end='')
    finally:
        (FOLDER/'curve-length-build.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
