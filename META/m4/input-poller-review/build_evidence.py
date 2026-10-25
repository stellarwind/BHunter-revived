"""Reproduce the source-only player-input harness and retain exact build evidence."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent
OUT=ROOT/'build/input-evaluate'
SOURCE=['src/bh_input_evaluate.c','src/bh_input_evaluate.h','tests/input_evaluate.c','src/bh_player_input.c','src/bh_player_input.h']

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
    result=dict(target='input-evaluate.exe',passed=False,original_code_required=False,
                game_launches=0,sources={s:digest(ROOT/s) for s in SOURCE},commands=[])
    def run(label,argv):
        p=subprocess.run([str(a) for a in argv],cwd=ROOT,env=env,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
        result['commands'].append(dict(label=label,argv=[str(a) for a in argv],returncode=p.returncode,output=p.stdout))
        if p.returncode:raise RuntimeError(label+' failed: '+p.stdout)
        return p.stdout
    try:
        flags=['/nologo','/c','/ML','/O2','/G5','/Oi','/Gy']
        run('compile-source',[compiler/'cl.exe',*flags,'/Fo'+str(OUT/'bh_player_input_events.obj'),ROOT/SOURCE[0]])
        run('compile-tests',[compiler/'cl.exe',*flags,'/DBH_INPUT_EVALUATE_STANDALONE','/Fo'+str(OUT/'player_input_events_tests.obj'),ROOT/SOURCE[2]])
        run('compile-input-owner',[compiler/'cl.exe',*flags,'/Fo'+str(OUT/'input_owner.obj'),ROOT/'src/bh_player_input.c'])
        run('link',[vc/'bin/LINK.EXE','/nologo','/subsystem:console','/verbose',
            '/map:'+str(OUT/'input-evaluate.map'),'/out:'+str(OUT/'input-evaluate.exe'),
            OUT/'bh_player_input_events.obj',OUT/'player_input_events_tests.obj',OUT/'input_owner.obj'])
        output=run('source-only-checks',[OUT/'input-evaluate.exe'])
        result['scenario']=json.loads(output.splitlines()[-1])
        assert result['scenario']['scenario']=='input-evaluate'
        assert result['scenario']['checks']>0 and result['scenario']['failures']==0
        result['executable_sha256']=digest(OUT/'input-evaluate.exe')
        result['passed']=True
        (FOLDER/'standalone-link-map.txt').write_bytes((OUT/'input-evaluate.map').read_bytes())
        print(output,end='')
    finally:
        (FOLDER/'standalone-build.json').write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
