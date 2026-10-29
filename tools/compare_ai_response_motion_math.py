"""Static response-tick arithmetic prefix graphs; not a whole tick proof."""
import argparse
import itertools
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT, trace, owner_instructions, parse_coff, digest, image_bytes


def verify_linked(symbols,sections):
    exe=(ROOT/'build/game/BHunter-source.exe').read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text()
    functions={s['name']:s for s in symbols.values() if s['section']>0 and s['type']==32};verified=set();bindings=[]
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(found)==1,name
        return int(found[0],16)
    def linked(symbol,va):
        if (symbol['name'],va) in verified:return
        verified.add((symbol['name'],va));definition=sections[symbol['section']];code=bytearray(image_bytes(exe,va,len(definition['data'])))
        for r in definition['relocations']:
            offset,s=r['offset'],r['symbol'];addend=struct.unpack_from('<I',definition['data'],offset)[0]
            if r['kind']==20:
                assert code[offset-1]==0xe8;target=va+offset+4+struct.unpack_from('<i',code,offset)[0]
                if s['storage']==3:linked(functions[s['name']],target)
                else:assert target==address(s['name'])
                bindings.append(dict(owner=symbol['name'],offset=offset,callee=s['name'],linked_va=hex(target)))
            else:
                assert r['kind']==6;target=struct.unpack_from('<I',code,offset)[0]
                if s['type']==32:
                    assert not addend
                    if s['storage']==3:linked(functions[s['name']],target)
                    else:assert target==address(s['name'])
                else:
                    assert s['section']>0 and definition['data'][offset-2] in (0xd8,0xd9,0xdc,0xdd)
                    width=8 if definition['data'][offset-2] in (0xdc,0xdd) else 4;at=s['value']+addend
                    assert image_bytes(exe,target,width)==sections[s['section']]['data'][at:at+width]
            code[offset:offset+4]=definition['data'][offset:offset+4]
        assert code==definition['data'],('Linked definition differs',symbol['name'])
    tick=functions['_bh_actor_response_tick'];assert [r['symbol']['name'] for r in sections[tick['section']]['relocations']].count('_bh_actor_response_motion')==1
    linked(tick,address(tick['name']));motion=functions['_bh_actor_response_motion'];linked(motion,address(motion['name']))
    if '_bh_actor_response_route_smoke' in functions:
        smoke=functions['_bh_actor_response_route_smoke'];linked(smoke,address(smoke['name']))
    if '_bh_actor_response_finish' in functions:
        finish=functions['_bh_actor_response_finish'];linked(finish,address(finish['name']))
        assert [r['symbol']['name'] for r in sections[tick['section']]['relocations']].count(finish['name'])==1
    return dict(executable_sha256=digest(exe),actual_tick_calls_motion=True,entire_tick_motion_private_helpers_constants_and_real_dependencies_linked=True,verified_definitions=sorted(name for name,va in verified),bindings=bindings,
                limits=['Link proof only; full tick arithmetic/call/store fidelity remains unproved.'])


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_response.obj')
    parser.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-response-motion-match/math-audit')
    a=parser.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_response_motion')
    section=sections[owner['section']];source,listing,end,_=owner_instructions(a.object,symbols,sections,owner['name'])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    start,stop=0x439935,0x439bfe;original=[];old_constants={};old_calls={}
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    for row in json.loads((ROOT/'META/m4/target656/response/original/00439910.disasm.json').read_text()):
        va=int(row['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        if row['mnemonic']=='CALL':assert row['operands']==['0x0047de20'];old_calls[va-start]='ftol'
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:old_constants[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    original.append(dict(address=stop-start,data=b'',mnemonic='RET',operands=''))
    def source_bindings(rows,section):
        constants={};calls={}
        for r in section['relocations']:
            row=next(v for v in rows if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol']
            if r['kind']==20:calls[row['address']]={'_response_query_timing_valid':'timing','_response_tick_phase_valid':'phase-timing','_bh_actor_route_now':'clock','__ftol':'ftol','__finite':'finite'}[s['name']]
            else:
                assert r['kind']==6 and row['mnemonic'].startswith('F');size=8 if 'QWORD' in row['operands'].upper() else 4
                at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];constants[row['address']]=constant(sections[s['section']]['data'][at:at+size])
        return constants,calls
    new_constants,new_calls=source_bindings(source,section);helpers={}
    for role,name in (('timing','_response_query_timing_valid'),('phase-timing','_response_tick_phase_valid')):
        symbol=next(s for s in symbols.values() if s['name']==name);h=sections[symbol['section']];rows,_,_,_=owner_instructions(a.object,symbols,sections,name)
        constants,calls=source_bindings(rows,h);helpers[role]=(rows,constants,calls,{},h)
    proofs=[];ocover=set();ncover=set()
    for unit_scale,fade_equal,kind,bits in itertools.product((False,True),(False,True),(2,6),range(16)):
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,0,bits,0,False,False,special=True,kind=kind,motion=True,unit_scale=unit_scale,fade_equal=fade_equal)
        new,nc,nv=trace(source,new_constants,new_calls,{},section,image,True,0,bits,0,False,False,special=True,kind=kind,motion=True,unit_scale=unit_scale,fade_equal=fade_equal,helpers=helpers)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(unit_scale=unit_scale,fade_equal=fade_equal,kind=kind,bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n')
            raise AssertionError('Response motion graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(unit_scale=unit_scale,fade_equal=fade_equal,kind=kind,bits=bits,graph_sha256=digest(json.dumps((old,oc)).encode())))
    assert ocover=={r['address'] for r in original},('Missing original prefix instructions',set(r['address'] for r in original)-ocover)
    missing_source=[dict(address=r['address'],mnemonic=r['mnemonic'],operands=r['operands']) for r in source if r['address'] not in ncover]
    assert [(r['mnemonic'],r['operands']) for r in missing_source]==[('POP','edi'),('POP','esi'),('ADD','esp, 0x18'),('RET','')]*2
    linked=verify_linked(symbols,sections);(out/'linked.json').write_text(json.dumps(linked,indent=2)+'\n')
    result=dict(original_va='00439910',prefix_start=hex(start),prefix_end_exclusive=hex(stop),original_prefix_bytes=stop-start,source_helper_bytes=end,byte_match=False,valid_prefix_graphs_equal=True,full_tick_proved=False,cases=len(proofs),object_sha256=digest(obj),source_guard_only_unvisited=missing_source,proofs=proofs,
                limits=['PC53 nearest/masked, bounded finite arithmetic and valid signed64 conversions.','Source typed context/clock/finite preflight adapters; stable synchronous clock required.','Attack, pursuit, smoke, death, RNG, loot, callbacks and remainder of tick outside this prefix proof.','Other x87 modes, NaN payloads, exception flags and aliases unproved.'])
    (out/'source.asm').write_text(listing);(out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
