"""Compile/link/run real source world segment and correction owners."""
import hashlib,json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
OUT=ROOT/'build/world-segment'
MODULES=['src/bh_world_segment.c','src/bh_camera_correction.c','tests/world_segment.c',
         'src/bh_math.c','src/bh_round.c','src/bh_fp.c','src/bh_route_query.c',
         'src/bh_geometry.c','src/bh_segment_query.c','src/bh_navigation.c',
         'src/bh_scene.c','src/bh_vfile.c','src/bh_portals.c','src/bh_camera_state.c',
         'src/bh_camera_modes.c','src/bh_rotation.c','src/bh_player_camera.c']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    OUT.mkdir(exist_ok=True,parents=True)
    compiler=ROOT/'analysis/toolchain-sp3/bin';vc=Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
    sp3=Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc');shared=vc.parent/'SHAREDIDE/BIN'
    env=os.environ.copy()
    for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
    env['PATH']=os.pathsep.join([str(compiler),str(shared),env.get('PATH','')])
    env['INCLUDE']=os.pathsep.join(str(p/'include') for p in (sp3,vc))
    env['LIB']=os.pathsep.join(str(p/'lib') for p in (sp3,vc))
    sources=MODULES+['src/'+p+'.h' for p in ('bh_world_segment','bh_entities','bh_math',
        'bh_scene','bh_geometry','bh_bsp','bh_fp','bh_segment_query','bh_route_query',
        'bh_navigation','bh_portals','bh_camera_state','bh_camera_modes','bh_player_camera',
        'bh_rotation','bh_round','bh_world_query','bh_world_geometry')]
    result=dict(target='world-segment.exe',passed=False,original_code_required=False,game_launches=0,
                sources={p:sha(ROOT/p) for p in sources},commands=[])
    def run(label,args):
        p=subprocess.run([str(a) for a in args],cwd=ROOT,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        result['commands'].append(dict(label=label,argv=[str(a) for a in args],returncode=p.returncode,output=p.stdout))
        if p.returncode:raise RuntimeError(label+': '+p.stdout)
        return p.stdout
    try:
        objects=[]
        for source in MODULES:
            obj=OUT/(Path(source).stem+'.obj');objects.append(obj)
            flags=['/DBH_WORLD_SEGMENT_STANDALONE'] if source=='tests/world_segment.c' else []
            run('compile-'+Path(source).stem,[compiler/'cl.exe','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/Fo'+str(obj),ROOT/source])
        run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/opt:ref','/verbose',
            '/map:'+str(OUT/'world-segment.map'),'/out:'+str(OUT/'world-segment.exe'),*objects,'kernel32.lib'])
        output=run('source-only-checks',[OUT/'world-segment.exe']);print(output,end='')
        result['scenario']=json.loads(output.splitlines()[-1]);assert not result['scenario']['failures']
        result['executable_sha256']=sha(OUT/'world-segment.exe');result['passed']=True
        (FOLDER/'standalone-link-map.txt').write_bytes((OUT/'world-segment.map').read_bytes())
    finally:(FOLDER/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
