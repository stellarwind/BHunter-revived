"""Static route/smoke call/RNG/arithmetic/store graphs; not a full tick proof."""
import argparse
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes
from compare_ai_response_motion_math import verify_linked


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_response.obj');parser.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-response-smoke-match/math-audit');a=parser.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_response_route_smoke');section=sections[owner['section']]
    source,listing,end,_=owner_instructions(a.object,symbols,sections,owner['name']);image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    start,stop=0x439bfe,0x439dcd;original=[];old_constants={};old_calls={};new_constants={};new_calls={}
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    for row in json.loads((ROOT/'META/m4/target656/response/original/00439910.disasm.json').read_text()):
        va=int(row['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
        if row['mnemonic']=='CALL':old_calls[va-start]={0x438ff0:'pursuit',0x47ddf0:'rand',0x4341a0:'curve',0x4025c0:'particle'}[int(row['operands'][0],16)]
        elif row['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:old_constants[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    original.extend([dict(address=stop-start,data=b'',mnemonic='ADD',operands='ESP, 0x34'),dict(address=stop-start+1,data=b'',mnemonic='RET',operands=''),dict(address=0x43a18f-start,data=b'',mnemonic='RET',operands='')])
    for r in section['relocations']:
        row=next(v for v in source if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol']
        if r['kind']==20:new_calls[row['address']]={'_bh_actor_pursuit_route':'pursuit','_rand':'rand','_bh_camera_curve':'curve','_bh_script_particle_create':'particle'}[s['name']]
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F');size=8 if 'QWORD' in row['operands'].upper() else 4;at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];new_constants[row['address']]=constant(sections[s['section']]['data'][at:at+size])
    proofs=[];ocover=set();ncover=set()
    for bits in range(8):
        old,oc,ov=trace(original,old_constants,old_calls,{},None,image,False,0,bits,0,False,False,special=True,motion=True,smoke=True)
        new,nc,nv=trace(source,new_constants,new_calls,{},section,image,True,0,bits,0,False,False,special=True,motion=True,smoke=True)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Route/smoke graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(dict(bits=bits,graph_sha256=digest(json.dumps((old,oc)).encode())))
    assert ocover=={r['address'] for r in original}
    missing=[r for r in source if r['address'] not in ncover]
    assert [(r['mnemonic'],r['operands']) for r in missing]==[('MOV','dword ptr [edi + 0x18], 0x2'),('OR','eax, -0x1'),('POP','edi'),('POP','esi'),('ADD','esp, 0x28'),('RET','')]
    linked=verify_linked(symbols,sections);(out/'linked.json').write_text(json.dumps(linked,indent=2)+'\n')
    result=dict(original_va='00439910',window_start=hex(start),window_end_exclusive=hex(stop),original_window_bytes=stop-start,source_helper_bytes=end,byte_match=False,valid_window_graphs_equal=True,full_tick_proved=False,cases=len(proofs),object_sha256=digest(obj),source_guard_only_unvisited=[r['address'] for r in missing],proofs=proofs,
                limits=['PC53 nearest/masked; bounded finite arithmetic.','Pursuit normalized by live actor writes and caller arguments; shared real pursuit native checks do not prove its internal original graph.','Curve and particle calls compared by arguments; RNG draw order explicit, constructor-internal RNG is a shared dependency checked natively.','Source typed context/status/output adapters and signed finite guards; full tick and other FP modes unproved.'])
    (out/'source.asm').write_text(listing);(out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='proofs'}))


if __name__=='__main__':main()
