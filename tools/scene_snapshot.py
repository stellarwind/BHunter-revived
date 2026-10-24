"""Build/run the source-only offscreen Direct3D2 static geometry checkpoint."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
import m0
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive',type=Path)
    parser.add_argument('--sdk',type=Path,default=Path(os.environ.get('BH_DIRECTPLAY_SDK','D:/Decomp/dx7sdk')))
    args=parser.parse_args();archive=args.archive.resolve() if args.archive else None;sdk=args.sdk.resolve()
    baseline=json.loads((ROOT/'META/reference-manifest.json').read_text())
    if archive:assert sha(archive)==next(e['sha256'] for e in baseline['files'] if e['path']=='GAME_DATA/Bhunter.bin')
    for entry in json.loads((ROOT/'META/native-render-dependencies.json').read_text())['files']:
        assert sha(sdk/entry['path'])==entry['sha256'],entry['path']
    roots=dict(repo=ROOT,fixtures=ROOT,compiler=Path(os.environ.get('BH_COMPILER_DIR',str(ROOT/'analysis/toolchain-sp3/bin'))),vc=Path(os.environ.get('BH_VC_ROOT','D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')),sp3=Path(os.environ.get('BH_SP3_ROOT','D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')),shared=Path(os.environ.get('BH_SHARED_DIR','D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')))
    for entry in m0.inventory(roots,references=False):
        expected=next(e for e in baseline['files'] if e['root']==entry['root'] and e['path']==entry['path'])
        assert expected['sha256']==entry['sha256'],entry['path']
    out=ROOT/'build/scene-snapshot';out.mkdir(parents=True,exist_ok=True)
    env=os.environ.copy()
    for n in ('CL','_CL_','LINK','_LINK_'):env.pop(n,None)
    env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env.get('PATH','')])
    env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
    env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
    report=dict(passed=False,original_code_required=False,commands=[],sources={})
    def run(label,argv):
        p=subprocess.run(list(map(str,argv)),cwd=out,env=env,capture_output=True,text=True,timeout=60)
        (out/(label+'.txt')).write_text(p.stdout+p.stderr)
        report['commands'].append(dict(label=label,argv=list(map(str,argv)),exit_code=p.returncode,output=p.stdout+p.stderr))
        print(label+': '+str(p.returncode))
        if p.returncode:raise RuntimeError(p.stdout+p.stderr)
        return p
    try:
        modules=('bh_view_camera','bh_clipper','bh_matrix_inverse','bh_texture_draw','bh_model','bh_scene_snapshot','bh_scene_view','bh_texture','bh_texture_image','bh_texture_tables','bh_pcx_reader','bh_render_guids','bh_pcx','bh_scene','bh_blocks','bh_bsp','bh_geometry','bh_math','bh_fp','bh_rotation','bh_round','bh_vfile')
        for name in modules:
            source=ROOT/'src'/(name+'.c');report['sources'][source.relative_to(ROOT).as_posix()]=sha(source)
            flags=['/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500'] if name in ('bh_clipper','bh_texture_draw','bh_model','bh_scene_view','bh_scene_snapshot','bh_texture','bh_texture_image','bh_texture_tables','bh_render_guids') else []
            run('compile-'+name,[roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/Fo'+str(out/(name+'.obj')),source])
        for header in (ROOT/'src').glob('*.h'):
            report['sources'][header.relative_to(ROOT).as_posix()]=sha(header)
        source=ROOT/'tests/scene_view.c';report['sources']['tests/scene_view.c']=sha(source)
        run('compile-controls',[roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/Fo'+str(out/'controls.obj'),source])
        run('link-controls',[roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(out/'scene-view-controls.exe'),out/'controls.obj',out/'bh_scene_view.obj',out/'bh_view_camera.obj',out/'bh_clipper.obj',out/'bh_matrix_inverse.obj',out/'bh_math.obj',out/'bh_round.obj',out/'bh_texture.obj',out/'bh_texture_draw.obj',out/'bh_texture_image.obj',out/'bh_texture_tables.obj',out/'bh_pcx_reader.obj',out/'bh_render_guids.obj',out/'bh_pcx.obj',out/'bh_vfile.obj',out/'bh_fp.obj',sdk/'lib/ddraw.lib','kernel32.lib','user32.lib'])
        p=run('controls',[out/'scene-view-controls.exe']);report['controls']=json.loads(p.stdout.splitlines()[-1]);assert not report['controls']['failures']
        source=ROOT/'tests/texture.c';report['sources']['tests/texture.c']=sha(source)
        run('compile-texture-controls',[roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(sdk/'include'),'/DDIRECT3D_VERSION=0x0500','/DDIRECTDRAW_VERSION=0x0500','/Fo'+str(out/'texture_controls.obj'),source])
        run('link-texture-controls',[roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(out/'texture-controls.exe'),out/'texture_controls.obj',out/'bh_texture.obj',out/'bh_texture_draw.obj',out/'bh_texture_image.obj',out/'bh_texture_tables.obj',out/'bh_pcx_reader.obj',out/'bh_render_guids.obj',out/'bh_scene_view.obj',out/'bh_view_camera.obj',out/'bh_clipper.obj',out/'bh_matrix_inverse.obj',out/'bh_math.obj',out/'bh_round.obj',out/'bh_fp.obj',out/'bh_pcx.obj',out/'bh_vfile.obj',sdk/'lib/ddraw.lib','kernel32.lib','user32.lib'])
        p=run('texture-controls',[out/'texture-controls.exe']);report['texture_controls']=json.loads(p.stdout.splitlines()[-1]);assert not report['texture_controls']['failures']
        run('link',[roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/map:'+str(out/'scene.map'),'/out:'+str(out/'bhunter-scene-snapshot.exe'),*[out/(n+'.obj') for n in modules],sdk/'lib/ddraw.lib','kernel32.lib','user32.lib'])
        if archive:
            p=run('render',[out/'bhunter-scene-snapshot.exe',archive,out/'scene.bmp'])
            report['scene']=json.loads(p.stdout.splitlines()[-1]);assert report['scene']['passed']
            assert sha(archive)==next(e['sha256'] for e in baseline['files'] if e['path']=='GAME_DATA/Bhunter.bin')
            report['image_sha256']=sha(out/'scene.bmp')
        report['executable_sha256']=sha(out/'bhunter-scene-snapshot.exe');report['passed']=True
    finally:
        (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
