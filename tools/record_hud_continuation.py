"""Bind the completed HUD continuation to real/clean linked source evidence."""
import hashlib
import json
from pathlib import Path
import shutil
import struct

ROOT=Path(__file__).resolve().parents[1]
HERE=ROOT/'META/m4/hud-continuation'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def write(p,v):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(v,indent=2)+'\n')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    real=read(ROOT/'build/m0/result.json');clean_report=read(HERE/'clean/clean-checkout.json');clean=clean_report['result']
    assert real['passed'] and clean['passed'] and real['sources']==clean['sources']
    assert not clean_report['original_assets_present'] and not clean_report['compiler_in_checkout']
    assert not real['original_code_required'] and not clean['original_code_required']
    for p,h in real['sources'].items():assert sha(ROOT/p)==h,p
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    ledger=read(ROOT/'META/function-status.json');assert hashlib.sha256(exe).hexdigest()==ledger['reference_exe_sha256']
    pe=struct.unpack_from('<I',exe,60)[0];table=pe+24+struct.unpack_from('<H',exe,pe+20)[0]
    sections=[struct.unpack_from('<IIII',exe,table+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
    def raw(at,n):
        r=at-0x400000;s=next(s for s in sections if s[1]<=r and r+n<=s[1]+s[2]);return exe[s[3]+r-s[1]:s[3]+r-s[1]+n]
    family=HERE/'weapon-factory'
    # Every row is a whole original owner; identical entrypoints share source.
    specs=[
      ('0041c860','bh_player_scene_draw','bh_player_scene',['player-scene','player-scene-native'],'game'),
      ('00428f50','bh_flyby_scene_draw','bh_flyby_scene',['flyby-scene','flyby-scene-native'],'game'),
      ('0042a9b0','bh_flyby_scene_rearm_entity','bh_flyby_scene',['flyby-scene'],'game'),
      ('00433820','bh_weapon_single_timer_tick','bh_weapon_timer',['weapon-factory'],'game'),
      ('00433b00','bh_weapon_single_timer_tick','bh_weapon_timer',['weapon-factory'],'game'),
      ('00442a30','bh_message_enqueue','bh_messages',['message-weapon','message-panel','message-panel-native','render-execute-panel'],'game'),
      ('0042f730','bh_weapon_empty_trigger','bh_weapon_empty',['message-weapon'],'game'),
      ('004309d0','bh_weapon_empty_trigger','bh_weapon_empty',['message-weapon'],'game'),
      ('00432560','bh_weapon_empty_trigger','bh_weapon_empty',['message-weapon'],'game'),
      ('00433dc0','bh_weapon_index9_trigger','bh_weapon_empty',['message-weapon'],'game'),
      ('00442440','bh_message_panel_render','bh_message_panel',['message-panel','message-panel-native','render-execute-panel'],'game'),
      ('004428b0','bh_message_panel_advance','bh_message_panel',['message-panel'],'game'),
      ('00442950','bh_message_panel_advance','bh_message_panel',['message-panel','message-panel-native','render-execute-panel'],'game'),
      ('00442b60','bh_message_panel_advance','bh_message_panel',['message-panel','message-panel-native','render-execute-panel'],'game'),
      ('00442bf0','bh_message_panel_advance','bh_message_panel',['message-panel','message-panel-native','render-execute-panel'],'game'),
      ('00442cd0','bh_message_panel_query','bh_message_panel',['message-panel'],'game'),
      ('00478b60','bh_message_text_plain','bh_message_panel',['message-panel','message-panel-native','render-execute-panel'],'engine_candidate'),
      ('00477e80','bh_message_text_execute_plain','bh_message_panel',['message-panel-native','render-execute-panel'],'engine_candidate'),
      ('00421bc0','bh_player_reset','bh_player_reset',['player-reset'],'game'),
      ('0045ca10','bh_frame_clear_depth','bh_frame_render',['frame-render','render-execute','render-execute-panel','player-scene-native'],'engine_candidate'),
      ('0045cbc0','bh_frame_clear_color','bh_frame_render',['frame-render','render-execute','render-execute-panel'],'engine_candidate'),
      ('00479090','bh_render_execute','bh_render_execute',['render-execute','render-execute-panel','player-scene-native'],'engine_candidate')]
    owners=[]
    for address,function,module,scenarios,role in specs:
        source='src/'+module+'.c'
        if address=='00479090':
            original=HERE/'original/00479090.ranges.json';r=read(original);ranges=r['ranges']
            for part in ranges:assert bytes.fromhex(part['hex'])==raw(int(part['start'],16),part['bytes'])
            n=sum(p['bytes'] for p in ranges);instructions=sum(p['instructions'] for p in ranges)
        else:
            paths=[HERE/'original'/(address+'.disasm.json'),family/(address+'.disasm.json'),ROOT/'META/m4/hud-attachment/next-player-review'/(address+'.disasm.json')]
            original=next(p for p in paths if p.is_file());rows=read(original);at=int(address,16);body=bytearray()
            for row in rows:
                assert int(row['address'],16)==at
                b=bytes.fromhex(row['bytes']);assert b==raw(at,len(b));body.extend(b);at+=len(b)
            assert rows[-1]['mnemonic']=='RET'
            n=len(body);instructions=len(rows)
            ranges=[dict(start=address,end_exclusive=f'{at:08x}',bytes=n,instructions=instructions,sha256=hashlib.sha256(body).hexdigest())]
        row=dict(address=address,function=function,source=source,role=role,scenarios=scenarios,
                 original=original.relative_to(ROOT).as_posix(),bytes=n,instructions=instructions,ranges=ranges)
        owners.append(row)
        fixtures={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in scenarios}
        clean_fixtures={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in scenarios}
        assert all(r['checks']>0 and not r['failures'] for r in fixtures.values())
        assert all(not r['failures'] and (r['checks']>0 or r.get('archive_skipped')) for r in clean_fixtures.values())
        identifier=address+'-functional-c-hud-continuation'
        record=dict(id=identifier,original_va=address,function=function,role=role,
                    provenance_confidence='Whole PE boundary, callers/table role and imports reviewed; historical engine authorship unresolved',
                    source=source,source_sha256=real['sources'][source],implementation='func',integration='linked_harness',
                    target='build/m0/bhunter-integration.exe',game_admitted=False,
                    validation=dict(boundary=ranges,raw_byte_match=False,original_code_executed=False,
                      fixtures=fixtures,clean_fixtures=clean_fixtures,
                      abi='Pinned x86 cdecl source binding; actual entity offsets where observed; PC53 nearest/masked profile',
                      limits=['Functional source and explicit context/selector adapters; no instruction match claim.',
                        'Safety guards and reported dependency/native errors extend original invalid-state behavior.',
                        'Full startup, player construction, mission callbacks and gameplay remain unfinished.',
                        'Software raster dispatch, PC24 behavior, CD audio and DirectPlay remain open.']),
                    evidence=[row['original'],'META/m4/hud-continuation/README.md','META/m4/hud-continuation/validation.json',
                              'META/m4/hud-continuation/integration-real.json','META/m4/hud-continuation/clean/clean-checkout.json'])
        for old in ledger['records']:
            if old['original_va']==address and old['id']!=identifier and not old.get('historical_variant'):
                assert old['implementation'] in ('not_started','wip'),old['id']
                old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records']=[r for r in ledger['records'] if r['id']!=identifier]+[record]
        assert ('_'+function).encode() in (ROOT/'build/m0/bhunter-integration.map').read_bytes(),function
    # The factory keeps verified identities for unavailable methods; do not count it as a full original.
    adapter=dict(id='00433ea0-construction-registry-adapter',original_va='00433ea0',function='bh_weapon_factory_create',
       role='game',provenance_confidence='Complete factory initialization inspected; callback method closure remains partial',
       source='src/bh_weapon_factory.c',source_sha256=real['sources']['src/bh_weapon_factory.c'],implementation='adapter',
       integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,
       validation=dict(raw_byte_match=False,scope='Actual pool allocations, generation-checked ten-entry registry and six verified method identities. Unrecovered methods remain non-callable data and explicit errors; no full factory/player constructor claim.',
          fixtures={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in ['weapon-factory','player-reset','player-scene']},
          limits=['Several firing/projectile methods remain unrecovered. Source binding admits one active factory context.']),
       evidence=['META/m4/hud-continuation/weapon-factory/README.md','META/m4/hud-continuation/validation.json'])
    ledger['records']=[r for r in ledger['records'] if r['id']!=adapter['id']]+[adapter]
    support=[]
    for identifier,function,source,scope in [
      ('message-panel-allocation-binding-adapter','bh_message_panel_create','src/bh_message_panel.c','Only the panel allocation slice of004474c0; not full scene startup. Real pool/query/step/refresh plus borrowed native resources.'),
      ('plain-text-ring-drain-adapter','bh_message_text_drain','src/bh_message_panel.c','Shared typed FIFO accepts plain, positioned and clear records; original whole00479090 admitted separately.'),
      ('flyby-entity-binding-adapter','bh_flyby_scene_bind','src/bh_flyby_scene.c','Exact entity and generation bind context to real source callbacks; next mission callbacks remain verified identity data.')]:
        support.append(dict(id=identifier,function=function,source=source,source_sha256=real['sources'][source],
            implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',
            game_admitted=False,validation=dict(scope=scope),evidence=['META/m4/hud-continuation/validation.json']))
    ledger['support_implementations']=[r for r in ledger.get('support_implementations',[]) if r['id'] not in {a['id'] for a in support}]+support
    ledger['updated']='2026-10-25';write(ROOT/'META/function-status.json',ledger)
    active=[r for r in ledger['records'] if r['implementation'] in ('func','bm') and r['integration']=='linked_harness' and not r.get('historical_variant') and not r.get('validation',{}).get('superseded_by')]
    count=len({r['original_va'] for r in active});assert count==300+len(owners),count
    for row in active:
        assert real['sources'][row['source']]==sha(ROOT/row['source']),row['id']
    write(HERE/'owners.json',owners);write(HERE/'integration-real.json',real)
    shutil.copyfile(ROOT/'build/m0/bhunter-integration.map',HERE/'link-map.txt')
    sources=dict(real['sources'])
    for p in ['tools/m0.py','tools/record_hud_continuation.py','tools/check_hud_continuation.py',
              'tools/export_blender_level.py','tools/blender_level_scene.py','tools/check_blender_level.py']:
        sources[p]=sha(ROOT/p)
    snapshots={}
    for name,digest in sources.items():
        target=HERE/'sources'/(digest+'-'+Path(name).name+'.txt');target.parent.mkdir(exist_ok=True)
        if not target.exists():shutil.copyfile(ROOT/name,target)
        snapshots[name]=target.relative_to(ROOT).as_posix()
    focused={s for o in owners for s in o['scenarios']}|{'weapon-factory'}
    validation=dict(passed=True,new_complete_functions=len(owners),complete_new_functions=len(owners),active_original_functions=count,
       translation_units=sum(c['label'].startswith('compile-') for c in real['commands']),
       missing_module_controls=sum(k.startswith('missing_') for k in real),sources=sources,source_snapshots=snapshots,
       real_total=real['test'],clean_total=clean['test'],
       real_focused={s:next(r for r in real['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},
       clean_focused={s:next(r for r in clean['scenarios'] if r.get('scenario')==s) for s in sorted(focused)},
       original_code_executed=False,game_launches=0,gameplay_executed=False,native_input_capture_calls=0,native_input_recenter_calls=0,
       notes=['Original00479090 owns its disjoint00450930 tail; counted once.',
              'Player native fixture uses640x480 and complete renderer; prior320px captures retain separate snapshot.',
              'The user ended the timed block early; no new work after closeout request.'])
    write(HERE/'validation.json',validation)
    print(json.dumps({k:validation[k] for k in ['passed','new_complete_functions','active_original_functions','translation_units','missing_module_controls','real_total','clean_total']}))

if __name__=='__main__':main()
