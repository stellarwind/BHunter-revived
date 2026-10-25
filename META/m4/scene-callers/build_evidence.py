"""Freeze source integration evidence and admit only this reviewed dependency closure."""
import argparse,csv,hashlib,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
FOLDER=Path(__file__).resolve().parent

def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,indent=2)+'\n')
def owners():
    rows=[]
    def add(address,function,source,role,scenarios,review):
        rows.append(dict(address=address,function=function,source='src/'+source+'.c',role=role,
            scenarios=scenarios.split(),review=review))
    review='META/m4/scene-callers-review/REVIEW.md'
    add('00477fe0','bh_camera_queue_pose','bh_camera_queue','engine_candidate','camera-queue scene-callers-native scene-background-native',review)
    add('00477830','bh_camera_queue_execute','bh_camera_queue','engine_candidate','camera-queue scene-callers-native scene-background-native',review)
    add('0041c020','bh_scene_background_step','bh_scene_background','game','scene-background scene-background-native',review)
    for a,f in [('00421740','setup'),('00475640','create'),('004756e0','bind_axis'),('00475770','bind_key'),('00475810','bind_string'),('00475870','apply_packet'),('00475990','transition')]:
        add(a,'bh_player_input_'+f,'bh_player_input','game' if a=='00421740' else 'engine_candidate','player-input','META/m4/player-input/review.json')
    for a,f in [('0047b2e0','bh_camera_state_init'),('0047b210','bh_camera_state_set_mode'),('0047b320','bh_camera_state_set_correction'),('00479910','bh_camera_mode_smooth'),('0047aac0','bh_camera_state_update')]:
        add(a,f,'bh_camera_state','engine_candidate','camera-state player-camera world-segment scene-callers-native','META/m4/camera-state/owner-review.json')
    for i,a in enumerate(['00479db0','00479e30','00479ff0','0047a0f0','0047a160','0047a1f0','0047a280','0047a2f0','0047a480','0047a7b0','00479f10','0047a610']):
        add(a,'bh_camera_mode_'+str(i),'bh_camera_modes','engine_candidate','camera-modes camera-state player-camera','META/m4/camera-modes/README.md')
    add('00479c70','bh_camera_look_at','bh_camera_modes','engine_candidate','camera-modes camera-state','META/m4/camera-modes/README.md')
    add('004221e0','bh_player_camera_reset','bh_player_camera','game','player-camera world-segment scene-callers-native','META/m4/player-camera/owner-review.json')
    add('004341a0','bh_camera_curve','bh_camera_curve','engine_candidate','camera-curve camera-route','META/m4/scene-callers-review/CURVE_REVIEW.md')
    for a,f in [('00413420','bh_actor_basis_build_groups'),('00413f80','bh_actor_basis_acquire')]:
        add(a,f,'bh_actor_basis','game','actor-basis','META/m4/actor-basis/owner-review.json')
    for a,f in [('0040c4a0','bh_world_segment_run'),('0040ba40','bh_world_segment_room_trace'),('0040b020','bh_world_segment_actor_trace')]:
        add(a,f,'bh_world_segment','game','world-segment','META/m4/world-segment/review.json')
    add('0041e6b0','bh_camera_correct','bh_camera_correction','game','world-segment','META/m4/world-segment/review.json')
    add('004296a0','bh_camera_route_advance','bh_camera_route','game','camera-route camera-route-real','META/m4/camera-route/REVIEW.md')
    for address,function in [('00470900','bh_atlas_page_text'),('00470c90','bh_atlas_page_text_width'),('00470d40','bh_atlas_page_image')]:
        add(address,function,'bh_atlas_text','engine_candidate','atlas-text text-queue','META/m4/atlas-text/owner-review.json')
    for address,function in [('00478bd0','bh_text_queue_text'),('00478ce0','bh_text_queue_clear'),('00477ea0','bh_text_queue_execute_text'),('00477f00','bh_text_queue_execute_clear')]:
        add(address,function,'bh_text_queue','engine_candidate','text-queue','META/m4/text-queue/review.json')
    add('00429fb0','bh_flyby_camera_step','bh_flyby_camera','game','flyby-camera','META/m4/flyby-camera/REVIEW.md')
    add('0045d7a0','bh_horizon_color_prepare','bh_horizon_color','engine_candidate','horizon-color','META/m4/horizon-color/REVIEW.md')
    add('004215c0','bh_player_input_broadcast','bh_player_input_events','game','player-input-events','META/m4/player-frontier/README.md')
    add('004757e0','bh_player_input_bind_kind4','bh_player_input_events','engine_candidate','player-input-events','META/m4/player-frontier/README.md')
    add('0044b0a0','bh_collision_resource_path','bh_collision_resource','engine_candidate','collision-resource','META/m4/collision-resource/owner-review.json')
    add('0044b130','bh_collision_cache_prepare','bh_collision_cache','engine_candidate','collision-cache','META/m4/collision-cache/owner-review.json')
    add('00474020','bh_input_evaluate','bh_input_evaluate','engine_candidate','input-evaluate','META/m4/input-poller-review/review.json')
    add('004756b0','bh_input_set_enabled','bh_input_evaluate','engine_candidate','input-evaluate','META/m4/input-poller-review/review.json')
    return rows

def original_reader():
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();pe=struct.unpack_from('<I',exe,60)[0]
    table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def raw(va,n):
        rva=va-0x400000;_,start,_,off=next(s for s in sections if s[1]<=rva and rva+n<=s[1]+s[2]);return exe[off+rva-start:off+rva-start+n]
    return raw

def export():
    mapped={r['address']:r for r in csv.DictReader((ROOT/'analysis/function-map.tsv').open(),delimiter='\t')}
    raw=original_reader();directory=FOLDER/'original';directory.mkdir(exist_ok=True)
    rows=owners()
    for row in rows:
        a=row['address'];m=mapped[a]
        instructions=json.loads(subprocess.check_output(['ghidra','--projects-dir',str(ROOT/'ghidra-projects'),'--project','Bhunter','--program','BHUNTER.EXE','disasm','0x'+a,'-n',m['instructions']],text=True))
        at=int(a,16);body=bytearray()
        for r in instructions:
            assert int(r['address'],16)==at
            b=bytes.fromhex(r['bytes']);body.extend(b);at+=len(b)
        assert len(instructions)==int(m['instructions']) and len(body)==int(m['body_bytes'])
        assert body==raw(int(a,16),len(body))
        path=directory/(a+'.tsv')
        with path.open('w',newline='') as stream:
            w=csv.DictWriter(stream,['address','bytes','mnemonic','operands'],delimiter='\t',lineterminator='\n');w.writeheader()
            w.writerows({**r,'operands':', '.join(r['operands'])} for r in instructions)
        row.update(original=path.relative_to(ROOT).as_posix(),bytes=len(body),instructions=len(instructions),body_sha256=hashlib.sha256(body).hexdigest(),end_inclusive=f'{at-1:08x}')
    table=raw(0x492d40,48)
    assert list(struct.unpack('<12I',table))==[int(a,16) for a in ['00479db0','00479e30','00479ff0','0047a0f0','0047a160','0047a1f0','0047a280','0047a2f0','0047a480','0047a7b0','00479f10','0047a610']]
    write(FOLDER/'owners.json',rows);write(FOLDER/'camera-table.json',dict(address='00492d40',bytes=table.hex(),entries=[f'{x:08x}' for x in struct.unpack('<12I',table)]))
    print('Exported and verified',len(rows),'complete original owners')

def finalize():
    rows=read(FOLDER/'owners.json');real=read(ROOT/'build/m0/result.json');clean_report=read(FOLDER/'clean/clean-checkout.json');clean=clean_report['result']
    assert real['passed'] and clean['passed'] and not clean_report['original_assets_present']
    assert real['sources']==clean['sources']
    for name,digest in real['sources'].items():assert sha(ROOT/name)==digest,name
    for row in rows:
        assert row['source'] in real['sources'] and (ROOT/row['review']).exists(),row
    write(FOLDER/'integration-real.json',real)
    (FOLDER/'link-map.txt').write_bytes((ROOT/'build/m0/bhunter-integration.map').read_bytes())
    sources=set(['src/bh_flyby_credits.h','src/bh_draw_queue.h','tests/checkpoint.c','tools/m0.py','tools/check_scene_callers.py',str(Path(__file__).relative_to(ROOT)).replace('\\','/')])
    for row in rows:
        source=row['source'];sources.add(source)
        header=source[:-2]+'.h'
        if (ROOT/header).exists():sources.add(header)
    sources.update('tests/'+name+'.c' for name in ['camera_queue','camera_state','camera_modes','player_input','player_camera','scene_background','camera_curve','world_segment','actor_basis','camera_route','camera_route_real','scene_callers_native','scene_background_native','atlas_text','text_queue','flyby_camera','horizon_color','player_input_events','collision_resource','collision_cache','input_evaluate'])
    snapshots={}
    for name in sorted(sources):
        p=ROOT/name;digest=sha(p);target=FOLDER/'sources'/(digest+'-'+p.name+'.txt');target.parent.mkdir(exist_ok=True);target.write_bytes(p.read_bytes());snapshots[name]=str(target.relative_to(ROOT)).replace('\\','/')
    focused=sorted(set(s for row in rows for s in row['scenarios'])|{'scene-callers-native','scene-background-native'})
    fixtures=lambda build:{s:next(r for r in build['scenarios'] if r.get('scenario')==s) for s in focused}
    control_count=sum(k.startswith('missing_') for k in real)
    assert control_count==sum(k.startswith('missing_') for k in clean)
    images={p.name:sha(p) for p in (ROOT/'build/m0').glob('*.bmp') if p.name.startswith(('scene-callers-native-','background-'))}
    for name in images:(FOLDER/name).write_bytes((ROOT/'build/m0'/name).read_bytes())
    validation=dict(passed=True,complete_new_functions=len(rows),translation_units=sum(c['label'].startswith('compile-') for c in real['commands']),missing_module_controls=control_count,
        real_total=real['test'],clean_total=clean['test'],real_focused=fixtures(real),clean_focused=fixtures(clean),
        sources={name:sha(ROOT/name) for name in snapshots},source_snapshots=snapshots,native_images=images,
        original_code_executed=False,game_launches=0,gameplay_executed=False,original_reference_files_changed=False,
        admitted_owners=[r['address'] for r in rows],unimplemented_callers=['0041c860','00428f50'])
    write(FOLDER/'validation.json',validation)
    ledger_path=ROOT/'META/function-status.json';ledger=read(ledger_path);old_ids={r['id'] for r in ledger['records']}
    for row in rows:
        record_id=row['address']+'-functional-c';assert record_id not in old_ids,record_id
        ledger['records'].append(dict(id=record_id,original_va=row['address'],function=row['function'],role=row['role'],
            provenance_confidence='Complete reviewed ordinary owner and real callers/table; CRT/import/compiler/shared tails excluded. Engine authorship unresolved where engine_candidate.',
            source=row['source'],source_sha256=sha(ROOT/row['source']),implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,
            validation=dict(abi='32-bit cdecl source with explicit owning/borrowed contexts and checked layouts; not a binary drop-in ABI claim',raw_byte_match=False,differential=False,
                boundary=f"Complete original{row['address']}..{row['end_inclusive']} ({row['bytes']}bytes/{row['instructions']}instructions)",
                fixtures={s:validation['real_focused'][s] for s in row['scenarios']},clean_fixtures={s:validation['clean_focused'][s] for s in row['scenarios']},
                limits=['Finite moderate masked-PC53 source checks and instruction-derived contracts; PC24/transcendental edges/universal original numerical equivalence unvalidated.',
                    'No original-code adapter or game execution; source game, full callers, horizon/software rasterizers/startup/weapon/mission lifetimes remain unfinished.']),
            evidence=[row['review'],row['original'],'META/m4/scene-callers/validation.json','META/m4/scene-callers/integration-real.json','META/m4/scene-callers/clean/clean-checkout.json']))
    for a in ['0041c860','00428f50']:
        if not any(r.get('original_va')==a for r in ledger['records']):
            ledger['records'].append(dict(id=a+'-reviewed-frontier',original_va=a,function='FUN_'+a,role='game',provenance_confidence='High: complete entity render callback installed by reviewed original constructors',implementation='not_started',integration='not_built',game_admitted=False,
                validation=dict(boundary='Complete RET owner; decompilation and dependency review only, no source implementation admission',limits=['Horizon/HUD/handoff/recipient and constructor dependencies remain real blockers; no omitted-body or success-stub implementation']),evidence=['META/m4/scene-callers-review/REVIEW.md','META/m4/scene-callers/validation.json']))
    support=[('bh_collision_cache_lifecycle-adapter','src/bh_collision_cache.c','bh_collision_cache_dispose'),('bh_collision_resource_load-adapter','src/bh_collision_resource.c','bh_collision_resource_load'),('bh_horizon_color_lifecycle-adapter','src/bh_horizon_color.c','bh_horizon_color_init'),('bh_camera_mode_dispatch-adapter','src/bh_camera_modes.c','bh_camera_mode_update'),('bh_camera_contextual_binding-adapter','src/bh_camera_state.c','bh_camera_state_bind_correction'),
        ('bh_player_input_lifecycle-adapter','src/bh_player_input.c','bh_player_input_owner_init'),('bh_player_input_packet_bounds-adapter','src/bh_player_input.c','bh_player_input_apply_packet_checked'),
        ('bh_scene_background_bindings-adapter','src/bh_scene_background.c','bh_scene_background_init'),('bh_player_camera_field-adapter','src/bh_player_camera.c','bh_player_camera_state'),
        ('bh_world_segment_bindings-adapter','src/bh_world_segment.c','bh_world_segment_init'),('bh_camera_correction_bridge-adapter','src/bh_camera_correction.c','bh_camera_correct_callback'),('bh_text_queue_fifo-adapter','src/bh_text_queue.c','bh_text_queue_drain')]
    for record_id,source,function in support:
        assert not any(r['id']==record_id for r in ledger['support_implementations'])
        ledger['support_implementations'].append(dict(id=record_id,source=source,function=function,source_sha256=sha(ROOT/source),implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,evidence=['META/m4/scene-callers/validation.json'],validation='Explicit source ownership/bounds/dispatch binding; no original function count or original-code execution'))
    ledger['updated']='2026-10-25';write(ledger_path,ledger)
    print(json.dumps({k:validation[k] for k in ['complete_new_functions','translation_units','missing_module_controls','real_total','clean_total']}))
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('action',choices=['export','finalize']);args=parser.parse_args()
    export() if args.action=='export' else finalize()
