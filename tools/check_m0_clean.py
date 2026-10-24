"""Check out the current source tree through a temporary Git index; build without assets."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import argparse

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence-dir',type=Path,default=ROOT/'META/m0',help='Destination for this run; choose a new milestone directory to retain historical evidence')
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='bhunter-m0-') as tmp:
        tmp = Path(tmp)
        env = os.environ.copy()
        env['GIT_INDEX_FILE'] = str(tmp / 'index')

        def git(*args):
            return subprocess.check_output(['git', *args], cwd=ROOT, env=env, text=True).strip()

        git('read-tree', 'HEAD')
        git('add', '-A')
        tree = git('write-tree')
        checkout = tmp / 'clean source checkout'
        checkout.mkdir()
        git('checkout-index', '--all', '--prefix='+checkout.as_posix()+'/')
        assert not (checkout / 'GAME_DATA').exists()
        assert not (checkout / 'analysis/toolchain-sp3').exists()
        assert not list(checkout.rglob('*.exe'))
        env.pop('GIT_INDEX_FILE')
        env['BH_COMPILER_DIR'] = os.environ.get('BH_COMPILER_DIR', str(ROOT / 'analysis/toolchain-sp3/bin'))
        p = subprocess.run(['cmd.exe','/d','/c',str(checkout / 'build.cmd')], cwd=tmp, env=env, capture_output=True, text=True)
        print(p.stdout+p.stderr, end='')
        result_path = checkout / 'build/m0/result.json'
        result = json.loads(result_path.read_text()) if result_path.exists() else None
        report = dict(source_tree=tree, method='temporary Git index checkout; includes current uncommitted source; real index untouched', original_assets_present=False, compiler_in_checkout=False, launch_working_directory=str(tmp), checkout=str(checkout), exit_code=p.returncode, result=result)
        destination = args.evidence_dir.resolve()
        destination.mkdir(parents=True,exist_ok=True)
        (destination / 'clean-checkout.json').write_text(json.dumps(report,indent=2)+'\n')
        if p.returncode or not result or not result['passed']:
            raise RuntimeError('Clean checkout failed')
        (destination / 'link-map.txt').write_bytes((checkout / 'build/m0/bhunter-integration.map').read_bytes())
        print('Clean source checkout passed; evidence '+str(destination / 'clean-checkout.json'))


if __name__ == '__main__':
    main()
