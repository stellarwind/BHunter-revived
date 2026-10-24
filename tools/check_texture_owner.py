"""Audit recovered native texture ownership separately from staging adapters."""
import argparse,csv,hashlib,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];FOLDER=ROOT/'META/m4/texture-owner'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def clean_scene():
    with tempfile.TemporaryDirectory(prefix='bhunter-texture-') as directory:
        tmp=Path(directory);env=os.environ.copy();env['GIT_INDEX_FILE']=str(tmp/'index')
        def git(*args):return subprocess.check_output(['git','-c','core.safecrlf=false',*args],cwd=ROOT,env=env,text=True,stderr=subprocess.DEVNULL).strip()
        git('read-tree','HEAD');git('add','-A');tree=git('write-tree');checkout=tmp/'clean textured source';checkout.mkdir()
        git('checkout-index','--all','--prefix='+checkout.as_posix()+'/');env.pop('GIT_INDEX_FILE')
        assert not (checkout/'GAME_DATA').exists() and not list(checkout.rglob('*.exe'))
        env['BH_COMPILER_DIR']=os.environ.get('BH_COMPILER_DIR',str(ROOT/'analysis/toolchain-sp3/bin'))
        p=subprocess.run([sys.executable,str(checkout/'tools/scene_snapshot.py')],cwd=tmp,env=env,capture_output=True,text=True,timeout=60)
        report=dict(source_tree=tree,original_assets_present=False,exit_code=p.returncode,output=p.stdout+p.stderr,result=read(checkout/'build/scene-snapshot/result.json'))
        (FOLDER/'scene-clean.json').write_text(json.dumps(report,indent=2)+'\n')
        assert not p.returncode and report['result']['passed'],report['output']
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--clean-scene',action='store_true');args=parser.parse_args()
    if args.clean_scene:parser.error("Historical texture evidence is pinned; use tools/check_image_owner.py for the current variant.")
    for entry in read(ROOT/'META/reference-manifest.json')['files']:
        if entry['root']=='repo':assert sha(ROOT/entry['path'])==entry['sha256']
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def original(address,size):
        rva=address-0x400000;_,start,length,offset=next(s for s in sections if s[1]<=rva and rva+size<=s[1]+s[2])
        return exe[offset+rva-start:offset+rva-start+size]
    review=read(FOLDER/'review.json');instructions=0
    ledger=read(ROOT/'META/function-status.json')
    for rec in review['records']:
        rows=list(csv.DictReader((FOLDER/'original'/(rec['address']+'.tsv')).open(),delimiter='\t'));at=int(rec['address'],16);body=bytearray()
        for row in rows:
            data=bytes.fromhex(row['bytes']);assert int(row['address'],16)==at and original(at,len(data))==data
            body.extend(data);at+=len(data)
        assert len(rows)==rec['instructions'] and at-1==int(rec['boundary'].split('-')[1],16)
        assert hashlib.sha256(body).hexdigest()==rec['body_sha256'];instructions+=len(rows)
        status=next(r for r in ledger['records'] if r['id']==rec['address']+'-functional-c')
        assert status['implementation']=='func' and status['integration']=='linked_harness' and not status['game_admitted']
        assert status['source_sha256']==sha(ROOT/status['source'])
    assert struct.unpack('<I',original(0x491340,4))[0]==9
    for i,minimum in enumerate((16,8,1,8,4,1,4,2,1)):
        row=struct.unpack('<16I',original(0x491348+i*64,64));assert row==(32<<(i//3),32<<(i//3),8+i%3*8,1,minimum,0,0,0,32,0,0,0,0,0,0,0)
    real=read(FOLDER/'integration-real.json');clean=read(FOLDER/'clean/clean-checkout.json')['result']
    scene=read(FOLDER/'scene-real.json');scene_clean=read(FOLDER/'scene-clean.json')['result']
    for build in (real,clean):
        assert build['passed'] and not build['original_code_required']
        test=next(s for s in build['scenarios'] if s.get('scenario')=='texture-ownership');assert test['checks']==46 and not test['failures']
        assert build['missing_texture_control']==build['missing_scene_view_control']=='rejected with LNK2001'
        expected=build['sources']['src/bh_texture.c'];path=ROOT/'src/bh_texture.c'
        if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-bh_texture.c.txt')
        assert sha(path)==expected
    for name,expected in scene['sources'].items():
        path=ROOT/name
        if sha(path)!=expected:path=FOLDER/'sources'/(expected+'-'+path.name+'.txt')
        assert sha(path)==expected==scene_clean['sources'][name],name
    assert scene['controls']['checks']==28 and scene['texture_controls']['checks']==46 and scene_clean['texture_controls']==scene['texture_controls']
    assert scene['scene']['passed'] and scene['scene']['textured'] and scene['scene']['textures']==146
    assert scene['scene']['triangles']==397 and scene['scene']['nonblack_pixels']==267843 and scene['scene']['unapplied_scripts']==202
    assert sha(FOLDER/'scene.bmp')==scene['image_sha256']
    for map_name in ('link-map.txt','scene-link-map.txt','clean/link-map.txt'):
        data=(FOLDER/map_name).read_bytes()
        for symbol in (b'_bh_texture_resident',b'_bh_texture_slot_create',b'_bh_texture_preallocate',b'_bh_texture_formats'):assert symbol in data
    print(json.dumps(dict(passed=True,complete_functions=6,original_instructions_verified=instructions,texture_checks=46,render_controls=28,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],textures=146,triangles=397,original_code_executed=False)))
if __name__=='__main__':main()
