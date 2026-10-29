"""Static response death/return instruction graphs, with declared source adapters."""
import argparse
import itertools
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes
from compare_ai_response_motion_math import verify_linked


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_response.obj');p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-response-finish-match/math-audit');a=p.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_response_finish');section=sections[owner['section']];source,listing,end,_=owner_instructions(a.object,symbols,sections,owner['name'])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    start,stop=0x439dcd,0x43a1c7;original=[];old_constants={};old_calls={}
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    for row in json.loads((ROOT/'META/m4/target656/response/original/00439910.disasm.json').read_text()):
        va=int(row['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        if row['mnemonic']=='CALL':old_calls[va-start]={0x476e90:'query',0x47c140:'sound',0x4341a0:'curve',0x47ddf0:'rand',0x403120:'debris',0x47bd40:'sound-request',0x423400:'loot',0x476f20:'acquire',0x43d7c0:'construct',0x4770f0:'broadcast',0x477070:'pool'}[int(row['operands'][0],16)]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:old_constants[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    new_constants={};new_calls={};bindings={}
    for r in section['relocations']:
        row=next(v for v in source if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol'];at=row['address']
        if r['kind']==20:new_calls[at]={'_bh_entity_set_query':'query','_bh_sound_stop':'sound','_bh_camera_curve':'curve','_rand':'rand','_bh_actor_damage_death_velocity':'velocity','_bh_script_debris_create':'debris','_bh_sound_request':'sound-request','_bh_remote_choose_slot':'loot','_bh_entity_acquire':'acquire','_bh_script_lifetime_bind_context':'mission-bind','_bh_target_construct_moving':'construct','_bh_entities_query':'broadcast','_unlink_actor':'unlink','_bh_entity_release':'pool'}[s['name']]
        elif s['name'] in ('_bh_mission_sound_tick','_bh_actor_vehicle_tick'):bindings[at]=dict(kind='callback',value=('callback','mission-tick' if s['name']=='_bh_mission_sound_tick' else 'vehicle-tick'))
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F');size=8 if 'QWORD' in row['operands'].upper() else 4;at_data=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];new_constants[at]=constant(sections[s['section']]['data'][at_data:at_data+size])
    helper=next(s for s in symbols.values() if s['name']=='_unlink_actor');h=sections[helper['section']];rows,_,_,_=owner_instructions(a.object,symbols,sections,helper['name']);assert [(r['kind'],r['symbol']['name']) for r in h['relocations']]==[(20,'_bh_actor_route_read')]
    helpers={'unlink':(rows,{}, {next(r['address'] for r in rows if r['mnemonic']=='CALL'):'state'}, {}, h)}
    proofs=[];ocover=set();ncover=set()
    configurations=itertools.product((1,2,6),(0,1,2,65535),(0,1,2,65535),range(4),(False,True),(False,True),(False,True),(False,True),(False,True))
    for kind,first,second,match,previous,next_,acquired,tag,move in configurations:
        settings=dict(special=True,motion=True,finish=True,smoked=True,kind=kind,slots=(first,second),acquired=acquired,tag=tag,callback_move=move)
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,0,0,match,previous,next_,**settings)
        new,nc,nv=trace(source,new_constants,new_calls,bindings,section,image,True,0,0,match,previous,next_,helpers=helpers,**settings)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(settings=settings,match=match,previous=previous,next=next_,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Response finish graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(kind=kind,first=first,second=second,match=match,previous=previous,next=next_,acquired=acquired,tag=tag,move=move,graph_sha256=digest(json.dumps((old,oc)).encode())))
    for smoked,unit,fade,below in itertools.product((False,True),repeat=4):
        bits=(1|int(below)<<1) if smoked else int(below)
        settings=dict(special=True,motion=True,finish=True,smoked=smoked,unit_scale=unit,fade_equal=fade)
        rows=original if smoked else [r for r in original if r['address']>=0x43a18f-start]
        old,oc,ov=trace(rows,old_constants,old_calls,{},None,image,False,0,bits,0,False,False,**settings)
        new,nc,nv=trace(source,new_constants,new_calls,bindings,section,image,True,0,bits,0,False,False,helpers=helpers,**settings)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(settings=settings,bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Response healthy graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(healthy=True,smoked=smoked,unit=unit,fade=fade,below=below,graph_sha256=digest(json.dumps((old,oc)).encode())))
    assert ocover=={r['address'] for r in original},('Original tail instructions missing',set(r['address'] for r in original)-ocover)
    missing=[r for r in source if r['address'] not in ncover];groups=[];group=[]
    for r in missing:
        group.append((r['mnemonic'],r['operands']))
        if r['mnemonic']=='RET':groups.append(group);group=[]
    epilogue=[('POP','edi'),('POP','esi'),('POP','ebp'),('POP','ebx'),('ADD','esp, 0x54'),('RET','')]
    assert groups==[[('MOV','dword ptr [ecx + 0x18], 0x1')]+epilogue,
                    [('MOV','edx, dword ptr [esp + 0x6c]'),('MOV','dword ptr [edx + 0x18], 0x1')]+epilogue,
                    [('MOV','edx, dword ptr [edi]'),('PUSH','ebx'),('PUSH','edx'),('CALL','0x374'),('ADD','esp, 0x8'),('MOV','dword ptr [ebp + 0x18], 0x1')]+epilogue] and not group
    linked=verify_linked(symbols,sections);(out/'linked.json').write_text(json.dumps(linked,indent=2)+'\n')
    (out/'source.asm').write_text(listing)
    result=dict(original_va='00439910',tail_start=hex(start),tail_end_exclusive=hex(stop),original_tail_bytes=stop-start,source_finish_bytes=end,valid_tail_graphs_equal=True,all_original_tail_instructions_covered=True,original_instructions=len(ocover),source_core_instructions=len([v for v in ncover if isinstance(v,int)]),byte_match=False,whole_tick_graph_proved=False,cases=len(proofs),object_sha256=digest(obj),source_adapter_only_unvisited=[r['address'] for r in missing],proofs=proofs,
                limits=['PC53 nearest/masked, bounded finite arithmetic; same-value sum/product commutation and identity double spills only.','Velocity helper uses its independently original102B-tested retained-X/f32-YZ numeric contract; helper internals are outside this owner graph.','Source private mission/legacy-angle/registry guards are explicit adapters; invalid-domain error blocks are not original-equivalent.','Curve/effect/loot/pool/sound/target/broadcast dependencies normalized by caller values; their internals are shared and not independently original-executed here.','Callback room relocation is explicit and fresh list reads are audited; arbitrary aliases, NaN payloads, exception flags, other FP modes and whole-tick composition remain unproved.'])
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
