"""Run the custom box with the old successor guard, then restore fixed binary.

Compiles one isolated negative-control source variant and links real game objects.
Does not edit production source or execute the actual training/mission level.
"""
import hashlib
import json
import os
import shutil
import subprocess
from pathlib import Path
from test_box_level import run_once

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'build/box-reuse-control'

def main():
    OUT.mkdir(exist_ok=True)
    build = json.loads((ROOT / 'build/game/build.json').read_text())
    assert build['passed']
    source = ROOT / 'src/bh_entities.c'
    variant = OUT / source.name
    text = source.read_text()
    old = 'if(next&&!active(pool,next))'
    assert text.count(old) == 1
    variant.write_text(text.replace(old, 'if(!successor(pool,next,generation))'))
    compiler = next(c['argv'] for c in build['commands'] if c['argv'][-1] == str(source))
    argv = [('/Fo' + str(OUT / 'bh_entities.obj')) if a.startswith('/Fo') else str(variant) if a == str(source) else a for a in compiler]
    argv.insert(1, '/I' + str(ROOT / 'src'))
    vc = Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
    sp3 = Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
    env = dict(os.environ)
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = str(ROOT / 'analysis/toolchain-sp3/bin') + os.pathsep + 'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN' + os.pathsep + env.get('PATH', '')
    env['INCLUDE'] = str(sp3 / 'include') + os.pathsep + str(vc / 'include')
    env['LIB'] = str(sp3 / 'lib') + os.pathsep + str(vc / 'lib')
    commands = []
    def call(args):
        p = subprocess.run(args, cwd=OUT, env=env, capture_output=True, text=True)
        commands.append(dict(argv=args, exit_code=p.returncode, output=p.stdout+p.stderr))
        assert p.returncode == 0, p.stdout+p.stderr
    call(argv)
    link = list(build['commands'][-1]['argv'])
    link = [('/out:' + str(OUT / 'BHunter-negative.exe')) if a.startswith('/out:') else ('/map:' + str(OUT / 'negative.map')) if a.startswith('/map:') else str(OUT / 'bh_entities.obj') if Path(a).name == 'bh_entities.obj' else a for a in link]
    rsp = OUT / 'link.rsp'
    rsp.write_text('\n'.join('"'+a+'"' for a in link[1:]))
    call([link[0], '@'+str(rsp)])
    box = ROOT / 'build/box-test/BHunter-box.exe'
    saved = OUT / 'BHunter-fixed.exe'
    shutil.copyfile(box, saved)
    try:
        shutil.copyfile(OUT / 'BHunter-negative.exe', box)
        result = run_once()
        for name in ('box-trace.csv', 'startup.log'):
            shutil.copyfile(ROOT / 'build/box-test' / name, OUT / name)
        log = result['startup_log']
        accepted = not result['passed'] and result.get('exit_code') == 12 and 'step=6' in log and 'active=1' in log
        report = dict(passed=accepted, scope='negative control in custom test box only',
                      commands=commands, source_sha256=hashlib.sha256(variant.read_bytes()).hexdigest(), run=result)
        (OUT / 'validation.json').write_text(json.dumps(report, indent=2)+'\n')
        print(json.dumps(dict(passed=accepted, ticks=result['ticks'], log=log)))
        return 0 if accepted else 1
    finally:
        shutil.copyfile(saved, box)
        assert box.read_bytes() == saved.read_bytes()

if __name__ == '__main__':
    raise SystemExit(main())
