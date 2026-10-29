"""Static ordinary-tick decay/damping prefix; not a complete tick proof."""
import argparse
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_damage.obj');p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-damage-tick-match/motion-audit');a=p.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_damage_tick_core');section=sections[owner['section']]
    source,listing,_,_=owner_instructions(a.object,symbols,sections,owner['name']);calls=[r for r in section['relocations'] if r['kind']==20];assert calls[0]['symbol']['name']=='_bh_actor_route_due'
    first_call=next(r['address'] for r in source if r['address']<=calls[0]['offset']<r['address']+len(r['data']))
    # Stop before loading route context/pushing the pending due-call argument.
    boundary=next(i for i,r in enumerate(source) if r['address']==first_call)-2
    assert [(r['mnemonic'],r['operands']) for r in source[boundary:boundary+2]]==[('MOV','ecx, dword ptr [edi + 0x4]'),('PUSH','ecx')]
    def due_arguments(registers,memory):
        assert registers['ESI']==('ptr','actor',0) and registers['EDI']==('ptr','context',0)
        assert memory[registers['ESP']]==('ptr','actor',0)
        assert memory[('context',4)]==('ptr','route',0)
    stop_new=source[boundary]['address'];new_constants={}
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    for r in section['relocations']:
        if r['offset']>=stop_new:continue
        row=next(v for v in source if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol'];assert r['kind']==6 and row['mnemonic'].startswith('F')
        width=8 if 'QWORD' in row['operands'].upper() else 4;at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];new_constants[row['address']]=constant(sections[s['section']]['data'][at:at+width])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    start,stop=0x4357e1,0x435950;original=[];old_constants={}
    for row in json.loads((ROOT/'META/m4/target600/b3/original/004357d0.disasm.json').read_text()):
        va=int(row['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        if row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:old_constants[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    # Original has already formed clock-duration in DI; that register is the
    # due-stage handoff. FSTP drops its unused displacement before this boundary.
    original.append(dict(address=stop-start,data=b'',mnemonic='BOUNDARY',operands=''))
    proofs=[];ocover=set();ncover=set()
    for bits in range(2):
        old,oc,ov=trace(original,old_constants,{}, {},None,image,False,0,bits,0,False,False,special=True,motion=True,stop_address=stop-start)
        new,nc,nv=trace(source,new_constants,{}, {},section,image,True,0,bits,0,False,False,special=True,motion=True,stop_address=stop_new,boundary_contract=due_arguments)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Ordinary motion graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(bits=bits,graph_sha256=digest(json.dumps((old,oc)).encode())))
    assert ocover=={r['address'] for r in original[:-1]} and ncover=={r['address'] for r in source if r['address']<stop_new}
    result=dict(original_va='004357d0',prefix_start=hex(start),prefix_end_exclusive=hex(stop),original_prefix_bytes=stop-start,source_prefix_bytes=stop_new,valid_prefix_graphs_equal=True,all_original_and_source_prefix_instructions_covered=True,cases=2,byte_match=False,whole_owner_graph_proved=False,object_sha256=digest(obj),proofs=proofs,
                limits=['PC53 nearest/masked, bounded finite arithmetic; sum/product commutation and identity double spills only.','Original clock-duration DI setup is part of the next due-stage contract; no clock store is claimed by this prefix.','Clock branch/route advancement, smoke, per-actor integer RNG, death and whole-owner composition remain outside this graph.'])
    result['actual_due_actor_argument_and_route_context_handoff_verified']=True
    (out/'source.asm').write_text(listing);(out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
