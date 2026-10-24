"""Audit M2's recorded linked variants and requirement evidence against this tree.

Does not rebuild or run original code. Run the documented real/clean builds and
evidence checkers first. This command rejects stale source/build evidence.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]

def sha(path):return hashlib.sha256((ROOT/path).read_bytes()).hexdigest()
def read(path):return json.loads((ROOT/path).read_text(encoding='utf-8-sig'))
def require(condition,detail):
    if not condition:raise ValueError(detail)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write-report',action='store_true',help='Record this passing audit; default is read-only')
    args=parser.parse_args()
    real=read('META/m2/media/pcx-integration-real.json')
    checkout=read('META/m2/pcx-clean/clean-checkout.json');clean=checkout['result']
    for name,build in [('real',real),('clean',clean)]:
        require(build['passed'] and build['test']['failures']==0,name+' linked scenario failed')
        require(build['target']=='bhunter-integration.exe' and not build['original_code_required'],name+' target scope')
        commands=build['commands'];compiles=[v for v in commands if v['label'].startswith('compile-')]
        require(len(compiles)==23 and all(v['exit_code']==0 for v in compiles),name+' real-TU compilation')
        require(next(v for v in commands if v['label']=='link')['exit_code']==0,name+' positive link')
        require(not any('/force' in str(v).lower() for c in commands for v in c['argv']),name+' unresolved-definition allowance')
        controls={k:v for k,v in build.items() if k.startswith('missing_')}
        require(len(controls)==11 and all(v=='rejected with LNK2001' for v in controls.values()),name+' missing-module controls')
        for module in ('definition','vfile','config','paths','scene','bsp','blocks','round','entities','wave','pcx'):
            negative=next(v for v in commands if v['label']=='missing-'+module+'-control')
            require(negative['exit_code']!=0 and 'LNK2001' in negative['output'],name+' negative link '+module)
    require(not checkout['original_assets_present'] and not checkout['compiler_in_checkout'] and checkout['exit_code']==0,'clean checkout scope')
    require(real['sources']==clean['sources'],'real/clean variants differ')
    for p,h in real['sources'].items():require(sha(p)==h,'stale source variant '+p)
    manifest=read('META/reference-manifest.json')
    require(real['dependency_manifest_sha256']==sha('META/reference-manifest.json'),'real dependency manifest')
    require(clean['dependency_manifest_sha256']==hashlib.sha256((ROOT/'META/reference-manifest.json').read_text().encode()).hexdigest(),'Git LF manifest content')
    refs={v['path']:v for v in manifest['files'] if v['root']=='repo'}
    require(len(refs)==4,'four immutable references')
    for p,v in refs.items():require(sha(p)==v['sha256'] and (ROOT/p).stat().st_size==v['bytes'],'immutable reference changed '+p)
    scenario=lambda b,n:next(v for v in b['scenarios'] if v['scenario']==n)
    required=['m0-source-math','math-numerical','vfile','config-container-path','scene-readers','bsp-block-data','entity-pool','wave-readers','pcx-readers']
    for b in (real,clean):
        for n in required:
            s=scenario(b,n);require(s['checks']>0 and s['failures']==0,'required scenario '+n)
    assets=scenario(real,'vfile-real-assets')
    require(assets['entries']==1229 and assets['payload_bytes']==12839409 and assets['classes']==[505,459,203,60,1,1],'all archive entries/payload classes')
    scenes=[v for v in real['scenarios'] if v['scenario']=='scene-real-assets']
    require({v['backend'] for v in scenes}=={'embedded','preloaded','loose'},'scene backends')
    require(all((v['placements'],v['links'],v['route_nodes'],v['route_cells'])==(5815,543,283,80089) for v in scenes),'complete scene/route data')
    block=scenario(real,'blocks-real-assets');entity=scenario(real,'entities-real-data')
    require((block['bsp_files'],block['backend_decodes'],block['registry_models'],block['unapplied_scripts'],block['resolved_placements'])==(505,1515,454,202,5815),'BSP/registry/sidecar placement closure')
    require(entity['initial_capacity']==9600 and entity['placement_slots']==5815 and not entity['gameplay_callbacks_installed'],'actual arena ownership')
    wave=scenario(real,'wave-real-assets');pcx=scenario(real,'pcx-real-assets')
    require(wave['files']==60 and wave['backend_decodes']==180 and wave['sample_bytes_compared']==5762313 and not wave['playback_objects_created'],'WAV data scope')
    require(pcx['files']==459 and pcx['backend_decodes']==1377 and pcx['pixel_bytes_compared']==24442368 and pcx['palette_settings']==4 and not pcx['surface_objects_created'],'PCX data scope')
    oracle=read('META/m2/math/oracle-final.json');require(oracle['passed'],'historical oracle result')
    for p,h in oracle['sources'].items():require(sha(p)==h,'math evidence belongs to another variant '+p)
    require(sha('tests/math_golden.h')==oracle['golden']['sha256'],'captured original-result data')
    for row in oracle['results']:
        if 'function' not in row or row['mode'] not in (0,2):continue
        require(all(row[k]==0 for k in ['return_differences','finite_word_differences','classification_or_infinity_differences','fp_status_differences','guard_or_control_failures']),'PC53/64 numeric discrepancy')
    require(sum(v['cases'] for v in oracle['results'] if v.get('mode')==0 and 'function' in v)==188928,'PC53 oracle coverage')
    require(scenario(clean,'math-numerical')['golden_cases']==4032 and scenario(clean,'math-numerical')['profile']=='x87-PC53-nearest-masked','source profile/fixture coverage')
    matching=read('META/m2/final/matching-verification.json')
    require(sha('analysis/astra-review/matched_math.cpp')==matching['source_sha256'],'matching source context')
    combined=[v for v in matching['builds'] if v['label'].startswith('combined-clean-')]
    require(len(combined)==2 and all(len(b['functions'])==4 and sum(v['target_bytes'] for v in b['functions'])==335 and all(v['raw_match'] and not v['relocations'] for v in b['functions']) for b in combined),'separate four-function matching positive control')
    mapping=(ROOT/'META/m2/media/pcx-link-map.txt').read_text(encoding='latin1')
    require(sha('META/m2/media/pcx-link-map.txt')==real['map_sha256'],'link-map variant')
    require(not re.search(r'map_original|original_image|historical_math_main|math_oracle\.obj',mapping,re.I),'original code adapter in integration target')
    # Every current admitted source variant must have real owning link symbols.
    ledger=read('META/function-status.json');linked=[];historical=[]
    for v in ledger['records']+ledger['support_implementations']:
        if v.get('integration')!='linked_harness':continue
        require(v['source_sha256']==sha(v['source']),'ledger variant '+v['id'])
        require(not v.get('game_admitted'),'unproved game admission '+v['id'])
        for p in v['evidence']:require((ROOT/p).exists(),'missing evidence '+p)
        if v['source'] not in real['sources']:
            # Historical linked C variants retain their historical evidence;
            # they must never borrow this different variant's link/test claim.
            historical.append(v['id']);continue
        if v['implementation'] in ('func','adapter'):
            symbol=v['function'];obj=Path(v['source']).stem+'.obj'
            require(any('_'+symbol+' ' in line and obj in line for line in mapping.splitlines()),'owning definition '+symbol)
            linked.append(v['id'])
    definitions=['src/bh_math.c','src/bh_fp.c','src/bh_vfile.c','src/bh_config.c','src/bh_paths.c','src/bh_scene.c','src/bh_bsp.c','src/bh_blocks.c','src/bh_entities.c','src/bh_round.c','src/bh_wave.c','src/bh_pcx.c']
    for p in definitions:
        source=(ROOT/p).read_text()
        require(not re.search(r'PAGE_EXECUTE|map_original|LoadLibrary|GetProcAddress|__asm',source),'unreviewed original-code/platform escape '+p)
    evidence=['META/m2/MATH.md','META/m2/VFILE.md','META/m2/CONFIG.md','META/m2/SCENE.md','META/m2/BLOCKS.md','META/m2/ENTITIES.md','META/m2/WAVE.md','META/m2/PCX.md','META/m2/media/pcx-validation.json','META/m2/math/oracle-final.json','META/m2/final/matching-verification.json','META/m2/media/pcx-integration-real.json','META/m2/pcx-clean/clean-checkout.json']
    requirements=[
        dict(requirement='One chosen variant of each assigned math routine and checked shared layouts',status='proven',evidence=['src/bh_math.c','src/bh_math.h','tests/math_numeric.c','META/m2/math/oracle-final.json'],limit='Ten assigned routines; original wider engine/game math remains future recovery'),
        dict(requirement='Alternative matching artifact separately reproducible',status='proven',evidence=['META/m2/final/matching-verification.json'],limit='Four C++ object variants; no transfer of byte-match evidence into C linked variants'),
        dict(requirement='PC24 resolved or intended numerical environment explicitly constrained and verified',status='proven_constrained_profile',evidence=['src/bh_fp.c','tests/math_numeric.c','tests/checkpoint.c'],limit='PC53/nearest/masked integration boundary; PC24 and original driver/thread transitions unresolved before game admission'),
        dict(requirement='Expanded zero/alias/magnitude/exceptional numerical testing',status='proven',evidence=['tests/math_cases.c','tests/math_numeric.c','tests/math_golden.h','META/m2/math/oracle-final.json'],limit='Explicit alias preconditions and NaN payload/sign exclusions'),
        dict(requirement='Allocation/container contracts, CRT reuse and real lifecycle ownership',status='proven',evidence=['src/bh_entities.c','src/bh_config.c','tests/entities.c','tests/config.c'],limit='Assigned foundation owners; game callbacks/resource lifetime/graph semantics remain later work'),
        dict(requirement='Archive/VFILE, paths/config and all archive entries validated',status='proven',evidence=['src/bh_vfile.c','src/bh_paths.c','src/bh_config.c','tests/vfile.c','tests/config.c'],limit='Legacy unversioned config shape cannot uniquely identify every incompatible file'),
        dict(requirement='Bounded asset readers required by reviewed first scene path',status='proven',evidence=['src/bh_scene.c','src/bh_bsp.c','src/bh_blocks.c','src/bh_wave.c','src/bh_pcx.c'],limit='Six archive payload classes; TXT sidecars read/retained, script execution and world construction are M4 behavior'),
        dict(requirement='Malformed/version-mismatched data rejected without out-of-bounds access',status='proven_with_format_limits',evidence=['tests/scene.c','tests/blocks.c','tests/wave.c','tests/pcx.c','tests/vfile.c'],limit='Guarded finite fixture coverage, explicit caps and dialect checks; no universal guarantee for unversioned formats'),
        dict(requirement='Foundations compile/link/execute together with no unresolved or fabricated definitions',status='proven',evidence=['META/m2/media/pcx-integration-real.json','META/m2/media/pcx-link-map.txt','META/m2/pcx-clean/clean-checkout.json'],limit='Harness admission; callback fixtures isolated in tests, source game target not claimed'),
        dict(requirement='Representative real assets loaded and invalid data diagnosed',status='proven',evidence=['META/m2/media/pcx-integration-real.json'],limit='All archive data available locally; no rendered/playable/audio acceptance scenario claimed'),
    ]
    report=dict(date='2026-10-24',passed=True,m2_complete=True,scope='Full M2 foundation integration gate in RESTORATION_PLAN.md; no source-game admission',real_integration=real['test'],clean_integration=clean['test'],translation_units=23,missing_module_controls=11,linked_implementation_variants=linked,historical_linked_variants_excluded_from_current_claim=historical,source_hashes=real['sources'],immutable_references={p:v['sha256'] for p,v in refs.items()},requirements=requirements,artifacts={p:sha(p) for p in evidence},remaining_m2_requirements=[],later_requirements=['M3 runnable original reference, feature/media/scenario inventory','Original runtime x87 driver/thread transitions and PC24 constraints before gameplay admission','M4 complete source world/script/navigation/texture/audio consumers and playable slice','CD audio/MCI behavior and media, DirectPlay/networking retained','Modern source compiler/backend/platform game admission, M8 full reachable-dependency audit'],audit_tool_sha256=sha('tools/check_m2_exit.py'))
    if args.write_report:(ROOT/'META/m2/final/exit-validation.json').write_text(json.dumps(report,indent=2)+'\n',newline='\n')
    print(json.dumps(dict(passed=True,m2_complete=True,real_checks=real['test']['checks'],clean_checks=clean['test']['checks'],requirements=len(requirements),report_written=args.write_report)))

if __name__=='__main__':main()
