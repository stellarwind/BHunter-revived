"""Ordinary tick probability/smoke instruction graphs, including real phase helper."""
import argparse
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_damage.obj');p.add_argument('--output-directory',type=Path);p.add_argument('--death',action='store_true');a=p.parse_args();out=a.output_directory or ROOT/('build/ai-damage-tick-match/death-audit' if a.death else 'build/ai-damage-tick-match/smoke-audit');out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_damage_tick_core');section=sections[owner['section']]
    rows,listing,_,_=owner_instructions(a.object,symbols,sections,owner['name']);image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    def source_contract(name,roles,begin=0,stop=None):
        s=next(s for s in symbols.values() if s['name']==name);sec=sections[s['section']];rr,ll,end,_=owner_instructions(a.object,symbols,sections,name);cc={};calls={}
        if stop is None:stop=end
        for r in sec['relocations']:
            if not begin<=r['offset']<stop:continue
            row=next(v for v in rr if v['address']<=r['offset']<v['address']+len(v['data']));sym=r['symbol']
            if r['kind']==20:calls[row['address']]=roles[sym['name']]
            else:
                assert r['kind']==6 and row['mnemonic'].startswith('F');width=8 if 'QWORD' in row['operands'].upper() else 4;at=sym['value']+struct.unpack_from('<I',sec['data'],r['offset'])[0];cc[row['address']]=constant(sections[sym['section']]['data'][at:at+width])
        return [r for r in rr if begin<=r['address']<=stop],cc,calls,{},sec,ll
    # First instruction after actual route-due call, through particle call.
    due=next(r for r in section['relocations'] if r['kind']==20 and r['symbol']['name']=='_bh_actor_route_due');i=next(i for i,r in enumerate(rows) if r['address']<=due['offset']<r['address']+len(r['data']));begin=rows[i+1]['address']
    particle=next(r for r in section['relocations'] if r['kind']==20 and r['symbol']['name']=='_bh_script_particle_create');i=next(i for i,r in enumerate(rows) if r['address']<=particle['offset']<r['address']+len(r['data']));stop_new=rows[i+1]['address']
    roles={'_rand':'rand','_damage_tick_phase':'phase-value','_bh_camera_curve':'curve','_bh_script_particle_create':'particle'}
    if a.death:
        begin=stop_new
        sound=next(r for r in section['relocations'] if r['kind']==20 and r['symbol']['name']=='_bh_sound_request');i=next(i for i,r in enumerate(rows) if r['address']<=sound['offset']<r['address']+len(r['data']));stop_new=rows[i+1]['address']
        roles.update({'_bh_entity_set_query':'query','_bh_sound_stop':'sound','_bh_actor_damage_death_velocity':'velocity','_bh_script_debris_create':'debris','_bh_sound_request':'sound-request'})
    source,nc,ncalls,_,_,_=source_contract(owner['name'],roles,begin,stop_new)
    jumps=[r for r in source if r['mnemonic'].startswith('J') and int(r['operands'],16)>stop_new];assert len(jumps)==(2 if a.death else 1);skip_new=int(jumps[0]['operands'],16);assert all(int(r['operands'],16)==skip_new for r in jumps)
    source.append(dict(address=skip_new,data=b'',mnemonic='BOUNDARY',operands=''))
    hr,hc,hcall,hb,hs,hl=source_contract('_damage_tick_phase',{'_bh_actor_route_now':'clock'});helpers={'phase-value':(hr,hc,hcall,hb,hs)}
    start,stop,skip=0x435973,0x435b23,0x435e3e;original=[];oc={};ocalls={}
    if a.death:start,stop=0x435b23,0x435ca0
    for row in json.loads((ROOT/'META/m4/target600/b3/original/004357d0.disasm.json').read_text()):
        va=int(row['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        if row['mnemonic']=='CALL':ocalls[va-start]={0x47ddf0:'rand',0x4341a0:'curve',0x4025c0:'particle',0x476e90:'query',0x47c140:'sound',0x403120:'debris',0x47bd40:'sound-request'}[int(row['operands'][0],16)]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:oc[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    original.extend(dict(address=v-start,data=b'',mnemonic='BOUNDARY',operands='') for v in (stop,skip))
    proofs=[];ocover=set();ncover=set()
    for bits in range(4):
        settings=dict(special=True,smoke=not a.death,finish=a.death,ordinary=True,mode3=bool(bits&2))
        continuing=bits==0 if a.death else bits!=3
        def boundary(source):
            def check(regs,memory):
                expected=(-24 if source else -20) if a.death else (-56 if source else -52)
                assert regs['ESP']==(expected if continuing else 0) and regs['ESI']==('ptr','actor',0)
                if source:assert regs['EDI']==('ptr','context',0)
                if continuing and not a.death:
                    assert regs['EBX']==('ptr','actor',0x12c if source else 0x150)
                    assert regs['EBP']==('ptr','actor',0x150 if source else 0x144)
                    if not source:assert regs['EDI']==('ptr','actor',0x12c)
            return check
        old,old_cmp,ov=trace(original,oc,ocalls,{},None,image,False,0,bits,0,False,False,**settings,stop_address={stop-start,skip-start},boundary_contract=boundary(False))
        new,new_cmp,nv=trace(source,nc,ncalls,{},section,image,True,0,bits,0,False,False,**settings,helpers=helpers,stop_address={stop_new,skip_new},boundary_contract=boundary(True))
        if old!=new or old_cmp!=new_cmp:
            (out/'difference.json').write_text(json.dumps(dict(bits=bits,original=old,source=new,original_comparisons=old_cmp,source_comparisons=new_cmp),indent=2)+'\n');raise AssertionError('Ordinary smoke graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(bits=bits,continues_to_next_stage=continuing,pending_stack_and_actor_context_handoff_verified=True,graph_sha256=digest(json.dumps((old,old_cmp)).encode())))
    assert ocover=={r['address'] for r in original if r['data']}
    missing=[r for r in source if begin<=r['address']<stop_new and r['address'] not in ncover]
    if a.death:
        assert [(r['mnemonic'],r['operands']) for r in missing]==[('MOV','dword ptr [edi + 0x2c], 0x1'),('POP','edi'),('POP','esi'),('POP','ebp'),('POP','ebx'),('ADD','esp, 0x80'),('RET','')]
    else:assert not missing
    assert {v[1] for v in ncover if isinstance(v,tuple)}=={r['address'] for r in hr}
    result=dict(original_va='004357d0',window_start=hex(start),window_end_exclusive=hex(stop),original_window_bytes=stop-start,source_begin=begin,source_end_exclusive=stop_new,all_original_and_source_window_instructions_covered=True,actual_phase_helper_all_instructions_interpreted=True,valid_probability_smoke_graphs_equal=True,cases=4,byte_match=False,whole_owner_graph_proved=False,object_sha256=digest(obj),interpreter_sha256=digest((ROOT/'tools/compare_ai_damage_query_math.py').read_bytes()),proofs=proofs,
                limits=['PC53 nearest/masked bounded finite arithmetic and synchronous clock binding.','Route-due is the preceding stage; post-particle death/return stages and whole-owner composition remain open.','Actual source phase helper is interpreted; real curve/particle dependencies are compared at the caller boundary.','Source typed context/clock/registry/status/legacy-angle adapters and broader FP modes remain outside the claim.'])
    if a.death:
        del result['valid_probability_smoke_graphs_equal'];result.update(valid_death_effect_graphs_equal=True,source_adapter_only_unvisited=[r['address'] for r in missing],all_original_and_source_window_instructions_covered=False,all_original_and_valid_source_window_instructions_covered=True)
        result['limits']=['PC53 nearest/masked bounded finite arithmetic; source legacy-angle guard is an explicit adapter.','Actual phase helper interpreted after symbolic sound callback changes clock/stamp/duration/controls; query changes the sound handle to check fresh reads.','Death velocity uses the separately original93B-validated retained-X/f32-YZ helper contract; helper internals are not included in this owner graph.','Curve/effect/sound caller contracts only; RNG/spin, constructor/unlink/return and whole-owner composition remain outside this window.']
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing);(out/'phase-source.asm').write_text(hl);print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
