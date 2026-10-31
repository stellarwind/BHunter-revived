"""Build and run standalone timing, portal, video layout and GPU transition checks."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime-dir', type=Path, default=ROOT / 'build')
    args = parser.parse_args()
    runtime = args.runtime_dir.resolve()
    output = ROOT / 'build-cache' / 'runtime-checks'
    output.mkdir(parents=True, exist_ok=True)
    gcc = shutil.which('gcc')
    if not gcc:
        parser.error('32-bit MinGW gcc required')
    cases = {
        'timing': ['tools/tests/sdl_timing.c', 'src/bh_render_interpolation.c', 'src/bh_rotation.c', 'src/bh_math.c', 'src/bh_round.c'],
        'portal': ['tools/tests/portal_clip.c', 'src/bh_portal_clip.c', 'src/bh_clipper.c', 'src/bh_matrix_inverse.c', 'src/bh_math.c', 'src/bh_round.c'],
        'video-layout': ['tools/tests/video_layout.c', 'src/bh_main_camera.c', 'src/bh_view_camera.c', 'src/bh_fp.c', 'src/bh_math.c', 'src/bh_rotation.c', 'src/bh_round.c'],
    }
    for name, sources in cases.items():
        executable = output / (name + '.exe')
        subprocess.run([gcc, '-O2', '-I' + str(ROOT / 'src'), *[str(ROOT / s) for s in sources], '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True, timeout=30)
    sdk = ROOT / 'toolchain/SDL3/SDL3-3.4.18'
    executable = output / 'video-transitions.exe'
    subprocess.run([os.environ.get('BH_GPU_CXX', shutil.which('g++') or 'g++'), '-std=c++17', '-O2', '-static',
                    '-DDIRECT3D_VERSION=0x0500', '-DDIRECTDRAW_VERSION=0x0500', '-I' + str(sdk / 'include'),
                    str(ROOT / 'tools/tests/sdl_video_transitions.cpp'), str(sdk / 'lib/x86/SDL3.lib'),
                    '-ld3dcompiler_47', '-luser32', '-lgdi32', '-o', str(executable)], check=True)
    subprocess.run([str(executable)], cwd=runtime, env=dict(os.environ, PATH=str(runtime) + os.pathsep + os.environ.get('PATH', '')), check=True, timeout=60)

if __name__ == '__main__':
    main()
