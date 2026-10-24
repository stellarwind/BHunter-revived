"""Pinned integration build and preservation checks. Python 3.10+, Windows."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import platform

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'META/reference-manifest.json'


def digest(path):
    h = hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda: f.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


def inventory(roots, references=True):
    entries = []
    for key in ('compiler', 'vc', 'sp3', 'shared'):
        base = roots[key]
        if key in ('vc', 'sp3'):
            files = [p for folder in ('include', 'lib') for p in (base / folder).rglob('*') if p.is_file()]
            if key == 'vc':
                files += [base / 'bin/LINK.EXE']
        elif key == 'shared':
            files = [base / name for name in ('MSPDB50.DLL', 'MSVCP50.DLL', 'MSDIS100.DLL')]
        else:
            files = [p for p in base.iterdir() if p.is_file()]
        if not files:
            raise RuntimeError('Empty dependency root: ' + str(base))
        for p in sorted(files):
            entries.append(dict(root=key, path=p.relative_to(base).as_posix(), bytes=p.stat().st_size, sha256=digest(p)))
    if references:
        for name in ('GAME_DATA/BHUNTER.EXE', 'GAME_DATA/Bhunter.bin', 'BHUNTER.mdf', 'BHUNTER.mds'):
            p = ROOT / name
            entries.append(dict(root='repo', path=name, bytes=p.stat().st_size, sha256=digest(p)))
    for name in ('tests/integration.c', 'analysis/math/differential.c', 'analysis/astra-review/matched_math.cpp'):
        p = ROOT / name
        entries.append(dict(root='fixtures', path=name, bytes=p.stat().st_size, sha256=digest(p)))
    return entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['build', 'record-manifest', 'verify-reference', 'probe-sp3-link'])
    parser.add_argument('--archive',type=Path,help='Also validate the pinned original Bhunter.bin as data; no original code is used')
    parser.add_argument('--math-regression-control',action='store_true',help='Require the archived pre-correction C variant to fail the current numerical fixtures')
    parser.add_argument('--vc-root', type=Path, default=os.environ.get('BH_VC_ROOT', 'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/VC'))
    parser.add_argument('--sp3-root', type=Path, default=os.environ.get('BH_SP3_ROOT', 'D:/Decomp/MSCVC/MSVC5/SP3_EXTRACTED/VS97_SP3/all/vc'))
    parser.add_argument('--compiler-dir', type=Path, default=os.environ.get('BH_COMPILER_DIR', str(ROOT / 'analysis/toolchain-sp3/bin')))
    parser.add_argument('--shared-dir', type=Path, default=os.environ.get('BH_SHARED_DIR', 'D:/Decomp/MSCVC/MSVC5/TOOLS/DEVSTUDIO/SHAREDIDE/BIN'))
    parser.add_argument('--directplay-sdk',type=Path,default=os.environ.get('BH_DIRECTPLAY_SDK','D:/Decomp/dx7sdk'))
    args = parser.parse_args()
    roots = dict(repo=ROOT, fixtures=ROOT, compiler=args.compiler_dir.resolve(), vc=args.vc_root.resolve(), sp3=args.sp3_root.resolve(), shared=args.shared_dir.resolve())
    if args.action in ('build', 'probe-sp3-link'):
        # Invalidate an earlier success even if dependency verification fails.
        write(ROOT / 'build/m0' / ('sp3-link-probe.json' if args.action=='probe-sp3-link' else 'result.json'),
              dict(schema_version=1, target='bhunter-integration.exe', passed=False, phase='dependency_verification'))
    if args.action == 'record-manifest':
        if MANIFEST.exists():
            raise RuntimeError('Manifest exists; review changes explicitly rather than overwriting preservation evidence')
        write(MANIFEST, dict(schema_version=1, scope='Original media and complete supplied include/lib trees; tools are local prerequisites, not redistributed', roots={k:str(v) for k,v in roots.items()}, files=inventory(roots)))
        print('Recorded', MANIFEST)
        return
    manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
    entries = [e for e in manifest['files'] if (e['root']=='repo') == (args.action=='verify-reference')]
    for e in entries:
        p = roots[e['root']] / e['path']
        if p.stat().st_size != e['bytes'] or digest(p) != e['sha256']:
            raise RuntimeError('Pinned dependency/reference changed: ' + str(p))
    if args.action == 'verify-reference':
        print('Verified all four immutable EXE/BIN/MDF/MDS references')
        return
    directplay_manifest_path=ROOT/'META/native-directplay-dependencies.json'
    directplay_manifest=json.loads(directplay_manifest_path.read_text())
    directplay_sdk=args.directplay_sdk.resolve()
    for dependency in directplay_manifest['files']:
        path=directplay_sdk/dependency['path']
        if path.stat().st_size!=dependency['bytes'] or digest(path)!=dependency['sha256']:
            raise RuntimeError('Pinned DirectPlay SDK dependency changed: '+str(path))
    archive=args.archive.resolve() if args.archive else None
    if archive:
        expected=next(e for e in manifest['files'] if e['path']=='GAME_DATA/Bhunter.bin')
        if digest(archive)!=expected['sha256']:
            raise RuntimeError('Real-asset scenario requires the pinned Bhunter.bin')
    out = ROOT / 'build/m0'
    out.mkdir(parents=True, exist_ok=True)
    result = dict(schema_version=1, target='bhunter-integration.exe', original_code_required=False, host=platform.platform(), python=sys.version, commands=[], passed=False)
    env = os.environ.copy()
    for name in ('CL', '_CL_', 'LINK', '_LINK_'):
        env.pop(name, None)
    env['PATH'] = os.pathsep.join([str(roots['compiler']), str(roots['shared']), env.get('PATH','')])
    env['INCLUDE'] = os.pathsep.join(str(roots[k] / 'include') for k in ('sp3','vc'))
    env['LIB'] = os.pathsep.join(str(roots[k] / 'lib') for k in ('sp3','vc'))
    result['environment'] = {k:env[k] for k in ('INCLUDE','LIB')}
    result['dependency_manifest_sha256'] = digest(MANIFEST)
    result['native_directplay_dependencies'] = dict(manifest_sha256=digest(directplay_manifest_path),root=str(directplay_sdk),files=directplay_manifest['files'])
    result['sources'] = {s:digest(ROOT/s) for s in ('src/bh_sound_listener.c','tests/sound_listener.c','src/bh_network_transport.c','src/bh_network_transport.h','tests/network_transport.c','src/bh_network_codec.c','src/bh_network_codec.h','tests/network_codec.c','src/bh_debris_response.c','src/bh_debris_response.h','tests/debris_response.c','src/bh_math.c','src/bh_math.h','src/bh_fp.c','src/bh_fp.h','src/bh_vfile.c','src/bh_vfile.h','src/bh_config.c','src/bh_config.h','src/bh_config_defaults.h','src/bh_paths.c','src/bh_paths.h','src/bh_scene.c','src/bh_scene.h','src/bh_bsp.c','src/bh_bsp.h','src/bh_round.c','src/bh_round.h','src/bh_blocks.c','src/bh_blocks.h','src/bh_pcx.c','src/bh_pcx.h','src/bh_script_timer.c','src/bh_script_timer.h','tests/script_timer.c','tests/script_timer_cases.h','src/bh_pcx_reader.c','src/bh_pcx_reader.h','tests/pcx_reader.c','src/bh_riff.c','src/bh_riff.h','tests/riff.c','src/bh_sound_queue.c','src/bh_sound_queue.h','tests/sound_queue.c','src/bh_draw_queue.c','src/bh_draw_queue.h','tests/draw_queue.c','src/bh_contact_effects.c','src/bh_contact_effects.h','tests/contact_effects.c','src/bh_physics_response.c','src/bh_physics_response.h','tests/physics_response.c','src/bh_entity_collision.c','src/bh_entity_collision.h','tests/entity_collision.c','src/bh_sweep.c','src/bh_sweep.h','tests/sweep.c','src/bh_collision_query.c','src/bh_collision_query.h','tests/collision_query.c','src/bh_collision_model.c','src/bh_collision_model.h','tests/collision_model.c','tests/saved_contact.h','src/bh_segment_query.c','src/bh_segment_query.h','tests/segment_query.c','src/bh_position.c','src/bh_position.h','tests/position.c','tests/position_cases.h','src/bh_world_query.c','src/bh_world_query.h','tests/world_query.c','src/bh_route_query.c','src/bh_route_query.h','tests/route_query.c','tests/route_query_cases.h','src/bh_navigation.c','src/bh_navigation.h','tests/navigation.c','tests/navigation_cases.h','src/bh_rotation.c','src/bh_rotation.h','src/bh_world_geometry.c','src/bh_world_geometry.h','tests/world_geometry.c','tests/world_geometry_cases.h','src/bh_geometry.c','src/bh_geometry.h','tests/geometry.c','tests/geometry_cases.h','src/bh_portals.c','src/bh_portals.h','tests/portals.c','tests/portal_cases.h','tests/pcx.c','tests/pcx_manifest.h','tests/pcx_palette_cases.h','src/bh_wave.c','src/bh_wave.h','tests/wave.c','tests/wave_manifest.h','src/bh_entities.c','src/bh_entities.h','tests/entities.c','tests/entity_query_cases.h','tests/blocks.c','tests/bsp_manifest.h','tests/block_names.h','tests/scene.c','tests/config.c','tests/integration.c','tests/checkpoint.c','tests/vfile.c','tests/math_cases.c','tests/math_cases.h','tests/math_numeric.c','tests/math_golden.h')}
    if archive:result['archive'] = dict(path=str(archive),sha256=digest(archive))

    def run(label, command, expect_success=True):
        p = subprocess.run([str(x) for x in command], cwd=out, env=env, capture_output=True, text=True)
        log = p.stdout + p.stderr
        (out / (label+'.txt')).write_text(log, encoding='utf-8')
        result['commands'].append(dict(label=label, argv=[str(x) for x in command], exit_code=p.returncode, output=log))
        print(label + ': exit ' + str(p.returncode) + ' (build/m0/' + label + '.txt)')
        if expect_success and p.returncode:
            print(log, end='')
            raise RuntimeError(label+' failed: '+str(p.returncode))
        return p

    try:
        cl = roots['compiler'] / 'CL.EXE'
        modules=[('src/bh_sound_listener.c','bh_sound_listener.obj'),('tests/sound_listener.c','sound_listener_tests.obj'),('src/bh_network_transport.c','bh_network_transport.obj'),('tests/network_transport.c','network_transport_tests.obj'),('src/bh_network_codec.c','bh_network_codec.obj'),('tests/network_codec.c','network_codec_tests.obj'),('src/bh_debris_response.c','bh_debris_response.obj'),('tests/debris_response.c','debris_response_tests.obj'),('src/bh_sound_queue.c','bh_sound_queue.obj'),('tests/sound_queue.c','sound_queue_tests.obj'),('src/bh_draw_queue.c','bh_draw_queue.obj'),('tests/draw_queue.c','draw_queue_tests.obj'),('src/bh_contact_effects.c','bh_contact_effects.obj'),('tests/contact_effects.c','contact_effects_tests.obj'),('src/bh_physics_response.c','bh_physics_response.obj'),('tests/physics_response.c','physics_response_tests.obj'),('src/bh_entity_collision.c','bh_entity_collision.obj'),('tests/entity_collision.c','entity_collision_tests.obj'),('src/bh_sweep.c','bh_sweep.obj'),('tests/sweep.c','sweep_tests.obj'),('src/bh_collision_query.c','bh_collision_query.obj'),('tests/collision_query.c','collision_query_tests.obj'),('src/bh_math.c','bh_math.obj'),('src/bh_fp.c','bh_fp.obj'),('src/bh_vfile.c','bh_vfile.obj'),('tests/integration.c','integration.obj'),('tests/vfile.c','vfile_tests.obj'),('tests/checkpoint.c','checkpoint.obj'),('tests/math_cases.c','math_cases.obj'),('tests/math_numeric.c','math_numeric.obj'),('src/bh_config.c','bh_config.obj'),('src/bh_paths.c','bh_paths.obj'),('tests/config.c','config_tests.obj'),('src/bh_scene.c','bh_scene.obj'),('tests/scene.c','scene_tests.obj'),('src/bh_bsp.c','bh_bsp.obj'),('src/bh_blocks.c','bh_blocks.obj'),('tests/blocks.c','blocks_tests.obj'),('src/bh_round.c','bh_round.obj'),('src/bh_entities.c','bh_entities.obj'),('tests/entities.c','entities_tests.obj'),('src/bh_wave.c','bh_wave.obj'),('tests/wave.c','wave_tests.obj'),('src/bh_pcx.c','bh_pcx.obj'),('tests/pcx.c','pcx_tests.obj'),('src/bh_portals.c','bh_portals.obj'),('tests/portals.c','portals_tests.obj'),('src/bh_geometry.c','bh_geometry.obj'),('tests/geometry.c','geometry_tests.obj'),('src/bh_rotation.c','bh_rotation.obj'),('src/bh_world_geometry.c','bh_world_geometry.obj'),('tests/world_geometry.c','world_geometry_tests.obj'),('src/bh_navigation.c','bh_navigation.obj'),('tests/navigation.c','navigation_tests.obj'),('src/bh_route_query.c','bh_route_query.obj'),('tests/route_query.c','route_query_tests.obj'),('src/bh_world_query.c','bh_world_query.obj'),('tests/world_query.c','world_query_tests.obj'),('src/bh_position.c','bh_position.obj'),('tests/position.c','position_tests.obj'),('src/bh_segment_query.c','bh_segment_query.obj'),('tests/segment_query.c','segment_query_tests.obj'),('src/bh_collision_model.c','bh_collision_model.obj'),('tests/collision_model.c','collision_model_tests.obj'),('src/bh_riff.c','bh_riff.obj'),('tests/riff.c','riff_tests.obj'),('src/bh_pcx_reader.c','bh_pcx_reader.obj'),('tests/pcx_reader.c','pcx_reader_tests.obj'),('src/bh_script_timer.c','bh_script_timer.obj'),('tests/script_timer.c','script_timer_tests.obj')]
        for source, obj in modules:
            flags=['/Dmain=bh_math_checkpoint'] if source=='tests/integration.c' else []
            if source in ('src/bh_network_transport.c','tests/network_transport.c'):
                flags+=['/I'+str(directplay_sdk/'include')]
            run('compile-'+Path(source).stem,[cl,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy',*flags,'/Fo'+str(out/obj),ROOT/source])
        linker = roots['compiler']/'link.exe' if args.action=='probe-sp3-link' else roots['vc']/'bin/LINK.EXE'
        objects=[out/obj for _,obj in modules]
        p = run('link',[linker,'/nologo','/subsystem:console','/verbose','/map:'+str(out/'bhunter-integration.map'),'/out:'+str(out/'bhunter-integration.exe'),*objects,'kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
        p = run('scenario',[out/'bhunter-integration.exe',*([archive] if archive else [])])
        result['scenarios']=[json.loads(line) for line in p.stdout.splitlines() if line.startswith('{')]
        result['test'] = json.loads(p.stdout.splitlines()[-1])
        for scenario in ('m0-source-math','math-numerical','vfile','config-container-path','scene-readers','bsp-block-data','entity-pool','wave-readers','pcx-readers','portal-matching','geometry-faces','world-geometry','navigation','route-query','world-query','position','segment-query','sound-listener','network-transport','network-codec','debris-response','sound-queue','draw-command-queue','contact-effects','physics-response','entity-collision','swept-collision','collision-model','collision-traversal','riff-optional','pcx-buffered-reader','script-timers'):
            rows=[row for row in result['scenarios'] if row.get('scenario')==scenario]
            if len(rows)!=1 or rows[0]['checks']<=0 or rows[0]['failures']:
                raise RuntimeError('Required scenario did not pass: '+scenario)
        if result['test']['failures'] or result['test']['checks'] <= 14 or result['test']['scenario']!='m2-foundations':
            raise RuntimeError('Unexpected scenario result')
        if archive and (result['test']['archive_entries']!=1229 or digest(archive)!=result['archive']['sha256']):
            raise RuntimeError('Real-asset scenario incomplete or archive changed')
        for backend in ('EXTERNAL ', 'EMBEDDED ', 'PRELOADED'):
            if 'VFILE:'+backend+':vopen(bhvf_alpha.dat,rb)\n' not in p.stdout:
                raise RuntimeError('VFILE diagnostic path missing: '+backend)
        result['vfile_diagnostic_controls']='all three exact original diagnostic strings exercised'
        result['executable_sha256'] = digest(out/'bhunter-integration.exe')
        result['map_sha256'] = digest(out/'bhunter-integration.map')
        # Positive link above establishes that this is a missing-module failure,
        # rather than an unavailable linker or CRT. No /FORCE allowances.
        p = run('missing-definition-control',[linker,'/nologo','/subsystem:console','/out:'+str(out/'missing-definition-control.exe'),*[p for p in objects if p.name!='bh_math.obj'],'kernel32.lib','dsound.lib','user32.lib','ole32.lib'],False)
        if p.returncode == 0 or 'LNK2001' not in p.stdout+p.stderr or '_bh_mat34_multiply' not in p.stdout+p.stderr:
            raise RuntimeError('Missing-definition link guard did not reject missing module')
        result['missing_definition_control'] = 'rejected with LNK2001'
        p = run('missing-vfile-control',[linker,'/nologo','/subsystem:console','/out:'+str(out/'missing-vfile-control.exe'),*[p for p in objects if p.name!='bh_vfile.obj'],'kernel32.lib','dsound.lib','user32.lib','ole32.lib'],False)
        if p.returncode==0 or 'LNK2001' not in p.stdout+p.stderr or '_bh_vfile_mount' not in p.stdout+p.stderr:
            raise RuntimeError('Missing VFILE module was not rejected')
        result['missing_vfile_control']='rejected with LNK2001'
        for name,symbol in [('sound_listener','_bh_sound_listener'),('network_transport','_bh_network_send'),('network_codec','_bh_packet_receive'),('debris_response','_bh_debris_receive'),('config','_bh_config_load'),('paths','_bh_paths_init'),('scene','_bh_scene_level_read'),('bsp','_bh_bsp_decode'),('blocks','_bh_blocks_load'),('round','_bh_round_float'),('entities','_bh_entity_acquire'),('wave','_bh_wave_read'),('pcx','_bh_pcx_read'),('portals','_bh_portals_match'),('geometry','_bh_geometry_face'),('rotation','_bh_mat34_rotate_y'),('world_geometry','_bh_world_geometry_build'),('navigation','_bh_navigation_build'),('route_query','_bh_route_plan'),('world_query','_bh_world_query_run'),('position','_bh_position_read'),('segment_query','_bh_segment_query_run'),('collision_model','_bh_collision_model_load'),('collision_query','_bh_collision_query_run'),('sweep','_bh_sweep_run'),('entity_collision','_bh_entity_collision_dispatch'),('physics_response','_bh_physics_contact_response'),('draw_queue','_bh_draw_oriented'),('contact_effects','_bh_contact_spark_create'),('sound_queue','_bh_sound_request'),('riff','_bh_riff_scan'),('pcx_reader','_bh_pcx_reader_next'),('script_timer','_bh_script_timer_bind')]:
            p=run('missing-'+name+'-control',[linker,'/nologo','/subsystem:console','/out:'+str(out/('missing-'+name+'-control.exe')),*[obj for obj in objects if obj.name!='bh_'+name+'.obj'],'kernel32.lib','dsound.lib','user32.lib','ole32.lib'],False)
            if p.returncode==0 or 'LNK2001' not in p.stdout+p.stderr or symbol not in p.stdout+p.stderr:
                raise RuntimeError('Missing '+name+' module was not rejected')
            result['missing_'+name+'_control']='rejected with LNK2001'
        if args.math_regression_control:
            baseline=ROOT/'META/m2/math/baseline-bh_math.c.txt'
            negative=out/'math-regression.obj'
            run('compile-math-regression',[cl,'/nologo','/c','/ML','/O2','/G5','/Oi','/Gy','/I'+str(ROOT/'src'),'/Fo'+str(negative),'/Tc'+str(baseline)])
            run('link-math-regression',[linker,'/nologo','/subsystem:console','/out:'+str(out/'bhunter-math-regression-control.exe'),*[negative if obj.name=='bh_math.obj' else obj for obj in objects],'kernel32.lib','dsound.lib','user32.lib','ole32.lib'])
            p=run('scenario-math-regression',[out/'bhunter-math-regression-control.exe'],False)
            scenarios=[json.loads(line) for line in p.stdout.splitlines() if line.startswith('{')]
            numerical=next(row for row in scenarios if row['scenario']=='math-numerical')
            vfile=next(row for row in scenarios if row['scenario']=='vfile')
            if p.returncode==0 or numerical['failures']==0 or vfile['failures']:
                raise RuntimeError('Archived numerical regression was not selectively rejected')
            result['math_regression_control']=dict(source=str(baseline.relative_to(ROOT)),sha256=digest(baseline),exit_code=p.returncode,numerical=numerical,vfile=vfile)
        result['passed'] = True
        print(json.dumps(result['test']))
    finally:
        write(out / ('sp3-link-probe.json' if args.action=='probe-sp3-link' else 'result.json'),result)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError) as error:
        print('M0 ERROR:', error, file=sys.stderr)
        sys.exit(1)
