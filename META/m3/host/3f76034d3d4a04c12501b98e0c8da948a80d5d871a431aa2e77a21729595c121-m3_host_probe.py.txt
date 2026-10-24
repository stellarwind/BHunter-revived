"""Compile/link/run isolated x86 host-capability queries; never runs BHUNTER."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import uuid
from m3_source_probe import registry_state

ROOT = Path(__file__).resolve().parents[1]

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    manifest = json.loads((ROOT / 'META/reference-manifest.json').read_text())
    roots = {k: Path(v) for k, v in manifest['roots'].items()}
    originals = {v['path']: v['sha256'] for v in manifest['files'] if v['root'] == 'repo'}
    for p, h in originals.items():
        assert sha(ROOT / p) == h
    run_id = uuid.uuid4().hex[:12]
    folder = ROOT / 'build/m3/host' / run_id
    folder.mkdir(parents=True)
    sdk = Path('D:/Decomp/dx7sdk/include')
    env = os.environ.copy()
    for k in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(k, None)
    env['PATH'] = os.pathsep.join([str(roots['compiler']), str(roots['shared']), env.get('PATH', '')])
    env['INCLUDE'] = os.pathsep.join(str(roots[k] / 'include') for k in ('sp3', 'vc'))
    env['LIB'] = os.pathsep.join(str(roots[k] / 'lib') for k in ('sp3', 'vc'))
    before = registry_state()
    report = dict(run_id=run_id, scope='Host MCI CD-audio and DirectPlay3A provider availability only', original_code_executes=False, source_hashes={p: sha(ROOT / p) for p in ['tools/m3_host_probe.c', 'tools/m3_host_probe.py']}, sdk_dplay_header=dict(path=str(sdk / 'dplay.h'), sha256=sha(sdk / 'dplay.h')), original_hashes=originals, registry_before=before, commands=[])
    report['source_snapshots'] = {}
    evidence = ROOT / 'META/m3/host'
    evidence.mkdir(exist_ok=True)
    for p, h in report['source_hashes'].items():
        dest = evidence / (h + '-' + Path(p).name + '.txt')
        if dest.exists():
            assert sha(dest) == h
        else:
            dest.write_bytes((ROOT / p).read_bytes())
        report['source_snapshots'][p] = dest.relative_to(ROOT).as_posix()
    def run(label, argv):
        r = subprocess.run([str(v) for v in argv], cwd=folder, env=env, capture_output=True, text=True, timeout=30, creationflags=subprocess.CREATE_NO_WINDOW)
        report['commands'].append(dict(label=label, argv=[str(v) for v in argv], exit_code=r.returncode, output=r.stdout + r.stderr))
        print(label, r.returncode)
        assert r.returncode == 0, r.stdout + r.stderr
        return r
    try:
        run('compile', [roots['compiler'] / 'CL.EXE', '/nologo', '/c', '/ML', '/O2', '/G5', '/I' + str(sdk), '/Fo' + str(folder / 'host.obj'), ROOT / 'tools/m3_host_probe.c'])
        run('link', [roots['vc'] / 'bin/LINK.EXE', '/nologo', '/subsystem:console', '/out:' + str(folder / 'host.exe'), folder / 'host.obj', 'kernel32.lib', 'winmm.lib', 'ole32.lib'])
        r = run('host-query', [folder / 'host.exe'])
        report['events'] = [json.loads(line) for line in r.stdout.splitlines() if line.startswith('{')]
        report['executable_sha256'] = sha(folder / 'host.exe')
        report['executable_path'] = (folder / 'host.exe').relative_to(ROOT).as_posix()
        report['passed'] = report['events'][-1]['event'] == 'host-probe-complete'
        print(r.stdout)
    finally:
        report['registry_after'] = registry_state()
        assert report['registry_after'] == before
        for p, h in originals.items():
            assert sha(ROOT / p) == h
        path = evidence / (run_id + '.json')
        path.write_text(json.dumps(report, indent=2) + '\n')
        print('Evidence', path.relative_to(ROOT))

if __name__ == '__main__':
    main()
