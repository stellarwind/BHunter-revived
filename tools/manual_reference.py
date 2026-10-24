"""Launch the preserved original for human play; no automated input or scenarios."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import uuid

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only', action='store_true')
    options = parser.parse_args()
    manifest = json.loads((ROOT/'META/reference-manifest.json').read_text())
    roots = {k: Path(v) for k, v in manifest['roots'].items()}
    folder = ROOT/'build/manual-original'
    folder.mkdir(parents=True, exist_ok=True)
    for name in ('BHUNTER.EXE', 'Bhunter.bin'):
        relative = 'GAME_DATA/'+name
        expected = next(v['sha256'] for v in manifest['files'] if v['path']==relative)
        original = ROOT/relative
        assert sha(original)==expected, 'Original changed: '+relative
        target = folder/name
        if not target.exists():
            shutil.copyfile(original, target)
        assert sha(target)==expected, 'Staged copy differs: '+name
    compat = json.loads((ROOT/'META/m3/compat/ddrawcompat-v0.7.2.json').read_text())
    dll = ROOT/compat['staged_dll']
    assert sha(dll)==compat['dll_sha256']
    if not (folder/'ddraw.dll').exists():
        shutil.copyfile(dll, folder/'ddraw.dll')
    assert sha(folder/'ddraw.dll')==compat['dll_sha256']
    source = ROOT/'build/m3/reference-source'
    assert sha(source/'Copyright.txt')=='2338bda2a87a5a5fb2468f5e56080382e1a88b64bf92c362c6458dcf0a5307ac'
    profile = folder/'profile'
    profile.mkdir(exist_ok=True)
    if not (profile/'BHUNTER.CFG').exists():
        shutil.copyfile(ROOT/'build/m3/startup-ddrawcompat/profile-dfa3b4981226/BHUNTER.CFG', profile/'BHUNTER.CFG')
    env = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = os.pathsep.join([str(roots['compiler']), str(roots['shared']), env.get('PATH', '')])
    env['INCLUDE'] = os.pathsep.join(str(roots[k]/'include') for k in ('sp3','vc'))
    env['LIB'] = os.pathsep.join(str(roots[k]/'lib') for k in ('sp3','vc'))
    commands = [
        [roots['compiler']/'CL.EXE','/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/Fo'+str(folder/'manual.obj'),ROOT/'tools/manual_reference.c'],
        [roots['vc']/'bin/LINK.EXE','/nologo','/subsystem:console','/out:'+str(folder/'manual-launcher.exe'),folder/'manual.obj','kernel32.lib','advapi32.lib','user32.lib'],
    ]
    for i, command in enumerate(commands):
        result = subprocess.run([str(v) for v in command], cwd=folder, env=env, capture_output=True, text=True)
        (folder/('compile.txt' if i==0 else 'link.txt')).write_text(result.stdout+result.stderr)
        if result.returncode:
            raise RuntimeError(result.stdout+result.stderr)
    print('Prepared original game with DDrawCompat; manual controls only.', flush=True)
    if options.prepare_only:
        return
    key = 'Software\\BhunterM3-'+uuid.uuid4().hex
    with (folder/'session.log').open('w') as log:
        result = subprocess.run([str(folder/'manual-launcher.exe'),str(folder/'BHUNTER.EXE'),str(folder),str(source),str(profile),key],cwd=folder,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError('Manual launcher failed; see build/manual-original/session.log')

if __name__ == '__main__':
    main()
