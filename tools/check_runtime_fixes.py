"""Compile real production units; execute focused non-renderer gameplay fixtures.

Uses the pinned compile contexts recorded by m0. Never runs the full suite or
any renderer, game loop, native input capture, or original code.
"""
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    out = ROOT / 'build/runtime-fixes'
    out.mkdir(exist_ok=True)
    baseline = json.loads((ROOT / 'build/m0/result.json').read_text())
    env = dict(os.environ)
    vc = Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC')
    sp3 = Path('D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc')
    compiler = ROOT / 'analysis/toolchain-sp3/bin'
    shared = Path('D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN')
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = os.pathsep.join([str(compiler), str(shared), env.get('PATH', '')])
    env['INCLUDE'] = os.pathsep.join([str(sp3 / 'include'), str(vc / 'include')])
    env['LIB'] = os.pathsep.join([str(sp3 / 'lib'), str(vc / 'lib')])
    result = dict(passed=False, sources={}, commands=[], scenarios=[],
                  target='build/runtime-fixes/runtime-fixes.exe',
                  software_renderer_executed=False, original_code_required=False,
                  gameplay_automation=False)
    selected = {'tests/physics_step.c', 'tests/target_construct.c', 'tests/input_collection.c',
                'tests/physics_response.c', 'tests/entity_collision.c', 'tests/sweep.c',
                'tests/world_query.c', 'tests/player_query.c', 'tests/actor_vehicle_draw.c', 'tests/actor_route.c', 'tests/actor_route_lifetime.c',
                'tests/script_actor_draw.c', 'tests/script_actor_tick.c', 'tests/script_actor_query.c',
                'tests/actor_launch.c', 'tests/actor_damage.c', 'tests/actor_response.c', 'tests/actor_construct.c', 'tests/actor_pursuit_route.c', 'tests/script_actor_construct.c', 'tests/script_actor_route.c'}
    selected |= {'tests/script_vm.c','tests/script_wait.c','tests/pickup.c','tests/pickup_basic.c',
                 'tests/world_segment.c','tests/actor_attack.c','tests/ballistic_projectile.c','tests/bh_weapon_fire.c','tests/bh_weapon_homing_track.c'}
    selected |= {'tests/script_attached_sound.c', 'tests/script_timer.c', 'tests/segment_query.c', 'tests/effect_projectile.c', 'tests/effect_orbit_tick.c', 'tests/script_lifetime.c', 'tests/bh_pulse_projectile.c', 'tests/homing_projectile.c', 'tests/bh_effect_split.c', 'tests/script_motion.c', 'tests/bh_area_projectile.c', 'tests/projectile_lifetime.c', 'tests/projectile_create.c'}
    objects = []
    selected.add('tests/sound_tick.c')

    def run(argv, cwd=out):
        process = subprocess.run([str(a) for a in argv], cwd=cwd, env=env,
                                 text=True, capture_output=True, timeout=120)
        result['commands'].append(dict(argv=[str(a) for a in argv], exit_code=process.returncode,
                                       output=process.stdout + process.stderr))
        if process.returncode:
            raise RuntimeError(process.stdout + process.stderr)
        return process.stdout

    try:
        for cmd in baseline['commands']:
            if not cmd['label'].startswith('compile-'):
                continue
            source = Path(cmd['argv'][-1])
            name = source.relative_to(ROOT).as_posix()
            if not name.startswith('src/') and name not in selected:
                continue
            argv = list(cmd['argv'])
            output_index = next(i for i, arg in enumerate(argv) if arg.startswith('/Fo'))
            obj = out / Path(argv[output_index][3:]).name
            argv[output_index] = '/Fo' + str(obj)
            run(argv)
            objects.append(obj)
            result['sources'][name] = hashlib.sha256(source.read_bytes()).hexdigest()
        runner = ROOT / 'tests/runtime_fixes.c'
        obj = out / 'runtime_fixes.obj'
        run([compiler / 'CL.EXE', '/nologo', '/c', '/ML', '/O2', '/G5', '/Gy', '/Fo' + str(obj), runner])
        objects.append(obj)
        result['sources']['tests/runtime_fixes.c'] = hashlib.sha256(runner.read_bytes()).hexdigest()
        golden = ROOT / 'tests/actor_damping_golden.h'
        result['sources']['tests/actor_damping_golden.h'] = hashlib.sha256(golden.read_bytes()).hexdigest()
        golden = ROOT / 'tests/ballistic_launch_golden.h'
        result['sources']['tests/ballistic_launch_golden.h'] = hashlib.sha256(golden.read_bytes()).hexdigest()
        for header in (ROOT / 'src').glob('*.h'):
            result['sources'][header.relative_to(ROOT).as_posix()] = hashlib.sha256(header.read_bytes()).hexdigest()
        sdk = Path('D:/Decomp/dx7sdk')
        link = [vc / 'bin/LINK.EXE', '/nologo', '/subsystem:console',
                '/out:' + str(out / 'runtime-fixes.exe'), '/map:' + str(out / 'runtime-fixes.map'),
                *objects, sdk / 'lib/ddraw.lib', sdk / 'lib/dinput.lib', 'kernel32.lib',
                'dsound.lib', 'user32.lib', 'ole32.lib', 'winmm.lib', 'gdi32.lib', 'advapi32.lib', 'comctl32.lib']
        response = out / 'link.rsp'
        response.write_text('\n'.join('"' + str(a) + '"' for a in link[1:]) + '\n')
        run([link[0], '@' + str(response)])
        result['translation_units'] = len(objects)
        for args in ([], [str(ROOT / 'GAME_DATA/Bhunter.bin')]):
            output = run([out / 'runtime-fixes.exe', *args])
            result['scenarios'].append(dict(archive=bool(args), output=output,
                                           checks=[json.loads(line) for line in output.splitlines() if line.startswith('{')]))
            print(output)
        result['executable_sha256'] = hashlib.sha256((out / 'runtime-fixes.exe').read_bytes()).hexdigest()
        result['passed'] = True
    finally:
        (out / 'validation.json').write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
