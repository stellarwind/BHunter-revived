"""Linked death/return candidate and direct-call/read-order audit. Not graph proof."""
import argparse
import json
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,owner_instructions,parse_coff,digest,image_bytes
from compare_ai_response_motion_math import verify_linked


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-response-finish-match/static-audit');a=p.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    path=ROOT/'build/game/bh_actor_response.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_response_finish');section=sections[owner['section']]
    rows,listing,end,_=owner_instructions(path,symbols,sections,owner['name']);image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=json.loads((ROOT/'META/m4/target656/response/original/00439910.disasm.json').read_text());assert b''.join(bytes.fromhex(r['bytes']) for r in original)==image_bytes(image,0x439910,2231)
    names={0x476e90:'_bh_entity_set_query',0x47c140:'_bh_sound_stop',0x4341a0:'_bh_camera_curve',0x47ddf0:'_rand',0x403120:'_bh_script_debris_create',0x47bd40:'_bh_sound_request',0x423400:'_bh_remote_choose_slot',0x476f20:'_bh_entity_acquire',0x43d7c0:'_bh_target_construct_moving',0x4770f0:'_bh_entities_query',0x477070:'_bh_entity_release'}
    old=[names[int(r['operands'][0],16)] for r in original if r['mnemonic']=='CALL' and 0x439de7<=int(r['address'],16)<0x43a18a]
    calls=[]
    for r in section['relocations']:
        if r['kind']!=20:continue
        row=next(v for v in rows if v['address']<=r['offset']<v['address']+len(v['data']));assert row['mnemonic']=='CALL'
        calls.append((row['address'],r['symbol']['name']))
    # Explicit source-only private binding/unlink and arithmetic extractions.
    # The earlier pool release belongs only to binding-failure cleanup.
    release_calls=[at for at,name in calls if name=='_bh_entity_release'];assert len(release_calls)==2
    adapted={'_bh_actor_damage_death_velocity','_bh_script_lifetime_bind_context','_unlink_actor'}
    new=[name for at,name in calls if name not in adapted and at!=release_calls[0]];assert old==new,(old,new)
    last_random=max(at for at,name in calls if name=='_rand');construct=next(at for at,name in calls if name=='_bh_target_construct_moving')
    def reads(offset):return [r['address'] for r in rows if r['mnemonic']=='MOV' and len(r['operands'].split(','))==2 and r['operands'].split(',')[1].strip().lower()=='dword ptr [esi + '+hex(offset)+']']
    assert any(last_random<at<construct for at in reads(0x114))
    assert any(at<last_random for at in reads(0x100)) and any(at<last_random for at in reads(0x1ac))
    loot=next(at for at,name in calls if name=='_bh_remote_choose_slot');acquire=next(at for at,name in calls if name=='_bh_entity_acquire')
    kind_compares=[r['address'] for r in rows if r['mnemonic']=='CMP' and 'dword ptr [esi + 0xfc]' in r['operands']]
    assert any(loot<at<acquire for at in kind_compares)
    assert any(acquire<at<last_random for at in reads(0xfc))
    # Actual mission publication, including a real callback relocation.
    callback=next(r for r in section['relocations'] if r['kind']==6 and r['symbol']['name']=='_bh_mission_sound_tick')
    callback_row=next(r for r in rows if r['address']<=callback['offset']<r['address']+len(r['data']))
    stores=[r for r in rows if acquire<r['address']<=callback_row['address'] and r['mnemonic']=='MOV' and r['operands'].startswith('dword ptr [ebx +')]
    assert [r['operands'].split(',')[0] for r in stores]==['dword ptr [ebx + 0x180]','dword ptr [ebx + 0x104]','dword ptr [ebx + 0xcc]']
    assert stores[0]['operands'].endswith(', 0x3fc00000')
    linked=verify_linked(symbols,sections);(out/'linked.json').write_text(json.dumps(linked,indent=2)+'\n')
    result=dict(original_va='00439910',original_owner_bytes=2231,source_finish_bytes=end,object_sha256=digest(obj),byte_match=False,whole_tick_graph_proved=False,direct_death_call_order_matches_with_declared_adapters=True,live_kind_and_constructor_reads_audited=True,mission_publication_order_audited=True,source_calls=calls,
                limits=['Direct static call ordering does not prove dynamic branch/call/arithmetic/store equivalence.','Velocity extraction has its own original102B native oracle; other effect/loot/notification dependencies still need connected original-owner comparisons.','Source context, mission private binding, legacy orientation and checked unlink are explicit adapters.'])
    (out/'source.asm').write_text(listing);(out/'candidate.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='source_calls'}))


if __name__=='__main__':main()
