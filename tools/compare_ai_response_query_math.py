"""Static complete special response-query instruction graphs; no game execution."""
import argparse
import itertools
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT, trace, owner_instructions, parse_coff, digest, image_bytes


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_response.obj')
    parser.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-query-match/response-math-audit')
    a=parser.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj)
    owner=next(s for s in symbols.values() if s['name']=='_bh_actor_response_query_core');section=sections[owner['section']]
    source,listing,end,_=owner_instructions(a.object,symbols,sections,owner['name'])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[]
    for row in json.loads((ROOT/'META/m4/target656/response/original/0043a1d0.disasm.json').read_text()):
        va=int(row['address'],16);data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J') and '[' not in arg:arg=hex(int(arg,16)-0x43a1d0)
        original.append(dict(address=va-0x43a1d0,data=data,mnemonic=row['mnemonic'],operands=arg))
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    old_constants={};new_constants={};old_calls={};new_calls={};bindings={}
    for row in original:
        if row['mnemonic']=='CALL':old_calls[row['address']]={0x4341a0:'curve',0x421610:'select',0x476e90:'query',0x47c140:'sound',0x477070:'pool',0x47de20:'ftol'}[0x43a1d0+row['address']+5+struct.unpack_from('<i',row['data'],1)[0]]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',row['operands'])
            if m:old_constants[row['address']]=constant(image_bytes(image,int(m[1],16),8 if 'double' in row['operands'] else 4))
    for r in section['relocations']:
        if r['offset']>=end:continue
        row=next(v for v in source if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol'];at=row['address']
        if r['kind']==20:new_calls[at]={'_bh_camera_curve':'curve','_response_query_select':'select','_unlink_actor':'unlink','_response_query_timing_valid':'timing','__ftol':'ftol','_bh_actor_route_now':'clock','_bh_entity_set_query':'query','_bh_sound_stop':'sound','_bh_entity_release':'pool'}[s['name']]
        elif s['section']==owner['section']:bindings[at]=dict(kind='local',target=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0])
        elif s['name'] in ('_release_tick','_bh_actor_response_tick'):bindings[at]=dict(kind='callback',value=('callback','release-tick' if s['name']=='_release_tick' else 'response-tick'))
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F')
            size=8 if 'QWORD' in row['operands'].upper() else 4;data_at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];new_constants[at]=constant(sections[s['section']]['data'][data_at:data_at+size])
    proofs=[];ocover=set();ncover=set()
    helper=next(s for s in symbols.values() if s['name']=='_unlink_actor');helper_section=sections[helper['section']]
    helper_rows,_,_,_=owner_instructions(a.object,symbols,sections,helper['name'])
    assert [(r['symbol']['name'],r['kind']) for r in helper_section['relocations']]==[('_bh_actor_route_read',20)]
    helpers={'unlink':(helper_rows,{}, {next(row['address'] for row in helper_rows if row['mnemonic']=='CALL'):'state'}, {}, helper_section)}
    for role,name in (('select','_response_query_select'),('timing','_response_query_timing_valid')):
        symbol=next(s for s in symbols.values() if s['name']==name);h=sections[symbol['section']]
        rows,_,_,_=owner_instructions(a.object,symbols,sections,name);hc={};hf={}
        for r in h['relocations']:
            row=next(v for v in rows if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol']
            if r['kind']==20:hf[row['address']]={'_bh_target_select':'selector','_bh_actor_route_now':'clock','__finite':'finite'}[s['name']]
            else:
                assert r['kind']==6 and row['mnemonic'].startswith('F');size=8 if 'QWORD' in row['operands'].upper() else 4
                start=s['value']+struct.unpack_from('<I',h['data'],r['offset'])[0];hc[row['address']]=constant(sections[s['section']]['data'][start:start+size])
        helpers[role]=(rows,hc,hf,{},h)
    for event,bits,match,marked in itertools.product([e for e in range(22) if e not in (0,4)]+[0xffffffff],range(4),range(4),(False,True)):
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,event,bits,match,marked,False,special=True)
        new,nc,nv=trace(source,new_constants,new_calls,bindings,section,image,True,event,bits,match,marked,False,special=True,helpers=helpers)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(event=event,bits=bits,match=match,marked=marked,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n')
            raise AssertionError('Response query graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(event=event,bits=bits,match=match,marked=marked,graph_sha256=digest(json.dumps((old,oc)).encode())))
    for kind,sender,bits in itertools.product((1,2,6,11),('null','self','player','other'),range(16)):
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,0,bits,0,False,False,special=True,kind=kind,sender=sender)
        new,nc,nv=trace(source,new_constants,new_calls,bindings,section,image,True,0,bits,0,False,False,special=True,kind=kind,sender=sender,helpers=helpers)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(event=0,kind=kind,sender=sender,bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n')
            raise AssertionError('Response reaction graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(event=0,kind=kind,sender=sender,bits=bits,graph_sha256=digest(json.dumps((old,oc)).encode())))
    for match,previous,next_ in itertools.product(range(4),(False,True),(False,True)):
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,4,0,match,previous,next_,special=True)
        new,nc,nv=trace(source,new_constants,new_calls,bindings,section,image,True,4,0,match,previous,next_,special=True,helpers=helpers)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(event=4,match=match,previous=previous,next=next_,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n')
            raise AssertionError('Response lifecycle graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(event=4,match=match,previous=previous,next=next_,graph_sha256=digest(json.dumps((old,oc)).encode())))
    assert ocover=={r['address'] for r in original},('Original instructions missing',set(r['address'] for r in original)-ocover)
    assert {r['address'] for r in source}=={v for v in ncover if isinstance(v,int)},('Source core instructions missing',set(r['address'] for r in source)-ncover)
    result=dict(original_va='0043a1d0',original_body_bytes=1793,source_body_bytes=end,byte_match=False,valid_path_graphs_equal=True,all_original_and_source_core_instructions_covered=True,cases=len(proofs),object_sha256=digest(obj),
                original_covered_instructions=len(ocover),source_covered_instructions=len(ncover),proofs=proofs,
                helper_coverage={role:dict(covered=len({v for v in ncover if isinstance(v,tuple) and v[0]==role}),total=len(spec[0])) for role,spec in helpers.items()},
                limits=['PC53 nearest/masked; sum/product commutation and bounded identity double spills only.','Valid nonzero-speed, finite signed64 conversion domain; timing preflight and fresh field-view/registry/reciprocal-list checks are source adapters.','Stable synchronous clock binding required across timing preflight and actual conversion.','Selection helper real target-select arguments audited; selector, curve/pool/query/sound dependencies compared by caller arguments rather than internal original instructions.','Source-only adapter failure paths not original-equivalent; PC24/64, NaN payloads, exception flags and arbitrary aliasing unproved.'])
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
