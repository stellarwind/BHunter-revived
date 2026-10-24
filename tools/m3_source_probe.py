"""Compile/run a bounded native debugger probe of the original source check.

Opt-in diagnostic only. Original EXE is copied unchanged; never linked into the
integration target. No UI, window automation, input injection or screenshots.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import uuid
import winreg

ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def registry_state():
    states=[]
    for label,view in [('32',winreg.KEY_WOW64_32KEY),('64',winreg.KEY_WOW64_64KEY)]:
        try:
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE,r'SOFTWARE\InterActive Vision\BHunter\Paths',0,winreg.KEY_READ|view) as key:
                value,kind=winreg.QueryValueEx(key,'SourceDir');states.append(dict(view=label,exists=True,value=value,type=kind))
        except OSError as e:states.append(dict(view=label,exists=False,error=e.winerror))
    return states

def main():
    manifest=json.loads((ROOT/'META/reference-manifest.json').read_text());refs={v['path']:v for v in manifest['files'] if v['root']=='repo'}
    for p,v in refs.items():assert sha(ROOT/p)==v['sha256'],p
    registry_before=registry_state();assert not any(v['exists'] for v in registry_before),'Baseline needs absent original SourceDir; do not alter an installed configuration'
    folder=ROOT/'build/m3/source-probe';folder.mkdir(parents=True,exist_ok=True)
    game=folder/'BHUNTER.EXE';original=ROOT/'GAME_DATA/BHUNTER.EXE'
    if game.exists():assert sha(game)==sha(original),'Do not overwrite a different reference copy'
    else:shutil.copyfile(original,game)
    source=ROOT/'build/m3/reference-source';copyright=source/'Copyright.txt'
    assert sha(copyright)=='2338bda2a87a5a5fb2468f5e56080382e1a88b64bf92c362c6458dcf0a5307ac','Run m3_media.py --extract-source first'
    bad=folder/'altered-source';bad.mkdir(exist_ok=True);altered=bytearray(copyright.read_bytes());altered[0]^=1
    (bad/'Copyright.txt').write_bytes(altered)
    roots={k:Path(v) for k,v in manifest['roots'].items()}
    env=os.environ.copy()
    for k in ('CL','_CL_','LINK','_LINK_'):env.pop(k,None)
    env['PATH']=os.pathsep.join([str(roots['compiler']),str(roots['shared']),env.get('PATH','')])
    env['INCLUDE']=os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
    env['LIB']=os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
    commands=[];scenarios=[]
    report=dict(date='2026-10-24',passed=False,scope='Native OS-loaded original source-check diagnostic; stops before GUI creation',original_code_executes=True,temporary_breakpoint_bytes=2,original_file_patches=0,shipping_dependency=False,integration_target_changed=False,original_exe_sha256=sha(original),source_sha256=sha(ROOT/'tools/m3_source_probe.c'),driver_sha256=sha(Path(__file__)),commands=commands,scenarios=scenarios,registry_before=registry_before,m3_complete=False)
    def run(label,argv,timeout=30):
        p=subprocess.run([str(v) for v in argv],cwd=folder,env=env,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW)
        item=dict(label=label,argv=[str(v) for v in argv],exit_code=p.returncode,output=p.stdout+p.stderr);commands.append(item)
        (folder/(label+'.txt')).write_text(item['output']);print(label,p.returncode)
        if p.returncode:raise RuntimeError(item)
        return p
    try:
        run('compile',[roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(folder/'source-probe.obj'),ROOT/'tools/m3_source_probe.c'])
        run('link',[roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(folder/'source-probe.exe'),folder/'source-probe.obj','kernel32.lib','advapi32.lib'])
        for label,mode,location,expected in [('absent-registry','baseline',source,False),('real-copyright','redirect',source,True),('altered-copyright','redirect',bad,False)]:
            key='Software\\BhunterM3-'+uuid.uuid4().hex
            p=run(label,[folder/'source-probe.exe',mode,game,folder,location,key])
            events=[json.loads(v) for v in p.stdout.splitlines() if v.startswith('{')]
            observed=next(v for v in events if v['event']=='source-check-return')
            assert observed['accepted']==expected and observed['stopped_before_ui']
            assert next(v for v in events if v['event']=='breakpoint-bytes-restored')['verified']
            if mode=='redirect':assert next(v for v in events if v['event']=='private-key-cleanup')['result']==0
            try:
                with winreg.OpenKey(winreg.HKEY_CURRENT_USER,key,0,winreg.KEY_READ|winreg.KEY_WOW64_32KEY):raise AssertionError('private key left behind')
            except FileNotFoundError:pass
            scenarios.append(dict(name=label,expected=expected,events=events,private_key=key,private_key_absent_after=True))
        report['passed']=True
    finally:
        report['registry_after']=registry_state();assert report['registry_before']==report['registry_after'],'machine registry reference changed'
        for p,v in refs.items():assert sha(ROOT/p)==v['sha256'],p
        assert sha(game)==sha(original) and sha(copyright)=='2338bda2a87a5a5fb2468f5e56080382e1a88b64bf92c362c6458dcf0a5307ac'
        report['original_references_unchanged']=True;report['reference_copy_sha256']=sha(game)
        dest=ROOT/'META/m3';dest.mkdir(exist_ok=True);(dest/'source-probe.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    print(json.dumps(dict(passed=report['passed'],scenarios=[dict(name=v['name'],accepted=next(e['accepted'] for e in v['events'] if e['event']=='source-check-return')) for v in scenarios],original_references_unchanged=True)))

if __name__=='__main__':main()
