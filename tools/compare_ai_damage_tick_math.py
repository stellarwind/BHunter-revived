"""Whole ordinary tick instruction graphs; explicit typed dependencies and PC53 domain."""
import argparse
import itertools
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes
from compare_ai_damage_random_math import bv,helper


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_damage.obj');p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-damage-tick-match/whole-audit');a=p.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_damage_tick_core');section=sections[owner['section']]
    source,listing,end,_=owner_instructions(a.object,symbols,sections,owner['name']);image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    def contract(name,roles):
        s=next(s for s in symbols.values() if s['name']==name);sec=sections[s['section']];rows,ll,_,_=owner_instructions(a.object,symbols,sections,name);constants={};calls={};bindings={}
        for r in sec['relocations']:
            row=next(v for v in rows if v['address']<=r['offset']<v['address']+len(v['data']));sym=r['symbol']
            if r['kind']==20:
                assert struct.unpack_from('<I',sec['data'],r['offset'])[0]==0
                calls[row['address']]=roles[sym['name']]
            elif sym['name']=='_normal_tick':
                assert struct.unpack_from('<I',sec['data'],r['offset'])[0]==0
                bindings[row['address']]=dict(kind='callback',value=('callback','normal-tick'))
            else:
                assert r['kind']==6 and row['mnemonic'].startswith('F'),(name,r)
                width=8 if 'QWORD' in row['operands'].upper() else 4;at=sym['value']+struct.unpack_from('<I',sec['data'],r['offset'])[0];constants[row['address']]=constant(sections[sym['section']]['data'][at:at+width])
        return rows,constants,calls,bindings,sec,ll
    roles={'_bh_actor_route_due':'due','_rand':'rand','_damage_tick_phase':'phase-value','_bh_camera_curve':'curve','_bh_script_particle_create':'particle','_bh_entity_set_query':'query','_bh_sound_stop':'sound','_bh_actor_damage_death_velocity':'velocity','_bh_script_debris_create':'debris','_bh_sound_request':'sound-request','_random_step':'rng-step','_bh_remote_choose_slot':'loot','_bh_target_construct_moving':'construct','_damage_query_room':'room','_bh_entity_release':'pool'}
    source,nc,ncalls,bindings,_,_=contract(owner['name'],roles)
    hr,hc,hcalls,hb,hs,hl=contract('_damage_tick_phase',{'_bh_actor_route_now':'clock'});helpers={'phase-value':(hr,hc,hcalls,hb,hs)}
    rr,_,_,_,rs,_=contract('_random_step',{});assert not rs['relocations'];random_helper=helper(rr)
    start,stop=0x4357d0,0x435e7d;original=[];oc={};ocalls={}
    for row in json.loads((ROOT/'META/m4/target600/b3/original/004357d0.disasm.json').read_text()):
        va=int(row['address'],16);assert start<=va<stop
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        if row['mnemonic']=='CALL':ocalls[va-start]={0x4355c0:'advance',0x47ddf0:'rand',0x4341a0:'curve',0x4025c0:'particle',0x476e90:'query',0x47c140:'sound',0x403120:'debris',0x47bd40:'sound-request',0x423400:'loot',0x43d7c0:'construct',0x477070:'pool'}[int(row['operands'][0],16)]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:oc[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    proofs=[];ocover=set();ncover=set()
    for bits,mode3,due_negative,unit,fade,previous,next_,move in itertools.product(range(32),*( (False,True),)*7):
        settings=dict(special=True,motion=True,smoke=True,finish=True,rng=True,rng_step=bv,ordinary=True,whole=True,mode3=mode3,due_negative=due_negative,unit_scale=unit,fade_equal=fade,callback_move=move)
        old,old_cmp,ov=trace(original,oc,ocalls,{},None,image,False,0,bits,0,previous,next_,**settings)
        new,new_cmp,nv=trace(source,nc,ncalls,bindings,section,image,True,0,bits,0,previous,next_,helpers=helpers,**settings)
        if old!=new or old_cmp!=new_cmp:
            (out/'difference.json').write_text(json.dumps(dict(bits=bits,mode3=mode3,due_negative=due_negative,unit=unit,fade=fade,previous=previous,next=next_,move=move,original=old,source=new,original_comparisons=old_cmp,source_comparisons=new_cmp),indent=2)+'\n');raise AssertionError('Whole ordinary tick graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(digest(json.dumps((old,old_cmp)).encode()))
    assert ocover=={r['address'] for r in original},('Missing original instructions',set(r['address'] for r in original)-ocover)
    missing=[r for r in source if r['address'] not in ncover]
    epilogue=[('POP','edi'),('POP','esi'),('POP','ebp'),('POP','ebx'),('ADD','esp, 0x80'),('RET','')]
    expected=[('MOV','dword ptr [edi + 0x2c], 0x1')]+epilogue+[('MOV','dword ptr [edi + 0x2c], 0x2')]+epilogue
    assert [(r['mnemonic'],r['operands']) for r in missing]==expected
    assert {v[1] for v in ncover if isinstance(v,tuple) and v[0]=='phase-value'}=={r['address'] for r in hr}
    result=dict(original_va='004357d0',original_owner_bytes=1709,source_core_bytes=end,whole_valid_PC53_graphs_equal=True,all_original_instructions_covered=True,cases=len(proofs),byte_match=False,strict_all_profile_cosmetic_only_proved=False,object_sha256=digest(obj),interpreter_sha256=digest((ROOT/'tools/compare_ai_damage_query_math.py').read_bytes()),random_helper=random_helper,graph_digest=digest(json.dumps(proofs).encode()),source_unvisited=[(r['address'],r['mnemonic'],r['operands']) for r in missing])
    result.update(all_valid_source_instructions_covered=True,actual_phase_helper_all_instructions_interpreted=True,source_legacy_angle_and_registry_error_paths_explicit=True,constructor_moves_room_and_inverts_neighbor_topology=True,loot_mutates_full32_random_and_constructor_metadata=True,
                  limits=['Complete ordinary caller graph for bounded finite PC53 nearest/masked arithmetic, not raw bytes or unrestricted all-profile cosmetic equivalence.','Source public context/live/dependency/environment/duration guards and private legacy-angle/room registry/status paths are explicit adapters.','Source route-due is a separately actual-instruction-validated dependency contract; original inline low16 branch and post-advance stamp execute in this graph. Valid successful-advance domain and stable bound clock only.','Actual phase helper is interpreted; integer RNG helper is interpreted independently for arbitrary uint32 input. Velocity uses separately original93B native-tested retained-X/f32-YZ contract.','Curve/effect/loot/pool/sound/target internals are shared typed dependency contracts, not automatically original-equivalent from this caller proof.','Constructor changes room and neighbor topology; loot changes random and metadata; query/sound changes handle/clock/stamp/duration/controls to audit fresh reads. Unrestricted aliasing/reentrancy remains outside the claim.','PC24/64/transitions, NaN payloads and exception flags remain unproved. Original uninitialized death angles are explicit fixture/source inputs; no original runtime angle values claimed.'])
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing);(out/'phase-source.asm').write_text(hl);print(json.dumps(result))


if __name__=='__main__':main()
