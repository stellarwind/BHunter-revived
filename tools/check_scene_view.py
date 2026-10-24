"""Audit the historical static-color source/native scene variant."""
import argparse,hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/scene-view'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clean():
    with tempfile.TemporaryDirectory(prefix='bhunter-scene-') as directory:
        tmp=Path(directory);env=os.environ.copy();env['GIT_INDEX_FILE']=str(tmp/'index')
        def git(*args):return subprocess.check_output(['git','-c','core.safecrlf=false',*args],cwd=ROOT,env=env,text=True,stderr=subprocess.DEVNULL).strip()
        git('read-tree','HEAD');git('add','-A');tree=git('write-tree')
        checkout=tmp/'clean scene source';checkout.mkdir();git('checkout-index','--all','--prefix='+checkout.as_posix()+'/')
        assert not (checkout/'GAME_DATA').exists() and not list(checkout.rglob('*.exe'))
        env.pop('GIT_INDEX_FILE');env['BH_COMPILER_DIR']=os.environ.get('BH_COMPILER_DIR',str(ROOT/'analysis/toolchain-sp3/bin'))
        p=subprocess.run([sys.executable,str(checkout/'tools/scene_snapshot.py')],cwd=tmp,env=env,capture_output=True,text=True,timeout=60)
        result=read(checkout/'build/scene-snapshot/result.json')
        (FOLDER/'clean.json').write_text(json.dumps(dict(source_tree=tree,original_assets_present=False,exit_code=p.returncode,output=p.stdout+p.stderr,result=result),indent=2)+'\n')
        assert not p.returncode and result['passed'],p.stdout+p.stderr
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--clean',action='store_true');args=parser.parse_args()
    if args.clean:raise SystemExit('This variant is historical. Use tools/check_texture_owner.py --clean-scene for the current textured source; historical evidence is retained.')
    real=read(FOLDER/'integration-real.json');c=read(FOLDER/'clean.json')['result']
    assert real['passed'] and c['passed'] and not real['original_code_required'] and not c['original_code_required']
    assert real['controls']==c['controls'] and real['controls']['checks']==28 and not real['controls']['failures']
    for name,expected in real['sources'].items():
        path=ROOT/name
        if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
        assert sha(path)==expected==c['sources'][name],name
    scene=real['scene'];assert scene['passed'] and scene['models']==454 and scene['placements']==5815
    assert scene['shell_placements']==1455 and scene['triangles']==403 and scene['nonblack_pixels']==267907
    assert scene['unapplied_scripts']==202 and not scene['textured'] and scene['hresult']==0
    assert sha(FOLDER/'scene.bmp')==real['image_sha256']
    data=(FOLDER/'link-map.txt').read_bytes()
    for symbol in (b'_bh_scene_view_polygon',b'_bh_scene_level_read',b'_bh_blocks_load',b'_bh_geometry_vertex',b'_bh_mat34_transform_point'):assert symbol in data
    for entry in read(ROOT/'META/reference-manifest.json')['files']:
        if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
    print(json.dumps(dict(passed=True,native_render_controls=28,models=454,placements=5815,shell_placements=1455,triangles=403,textured=False,original_code_executed=False)))
if __name__=='__main__':main()
