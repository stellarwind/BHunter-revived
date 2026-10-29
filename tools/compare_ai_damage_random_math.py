"""Unsigned32 instruction proof for ordinary tick RNG/spin publication."""
import argparse
import json
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes


def bv(op,a,b):
    if op=='step':return bv('xor',bv('or',bv('shr',bv('mul',a,0xe357),16),bv('mul',a,0xe3570000)),a)
    if isinstance(a,int) and isinstance(b,int):return {'mul':lambda:a*b,'shr':lambda:a>>(b&31),'or':lambda:a|b,'xor':lambda:a^b,'and':lambda:a&b}[op]()&0xffffffff
    if op in ('mul','or','xor','and') and repr(a)>repr(b):a,b=b,a
    return (op+'32',a,b)


def helper(rows):
    regs={'ESP':0};memory={4:('rng32','initial')}
    def read(arg):return memory[4] if arg.upper()=='DWORD PTR [ESP + 0X4]' else regs[arg.upper()] if arg.upper() in regs else int(arg,16)&0xffffffff
    for r in rows:
        m=r['mnemonic'];p=[s.strip() for s in r['operands'].split(',')]
        if m=='MOV':regs[p[0].upper()]=read(p[1])
        elif m=='IMUL':regs[p[0].upper()]=bv('mul',read(p[1]),read(p[2]))
        elif m in ('SHR','OR','XOR'):regs[p[0].upper()]=bv(m.lower(),read(p[0]),read(p[1]))
        else:assert m=='RET' and r==rows[-1]
    assert regs['EAX']==bv('step',memory[4],None)
    assert set(regs)<={'ESP','EAX','ECX','EDX'} and regs['ESP']==0
    return dict(all_instructions_interpreted=len(rows),returns_expected_modulo32_step=True,only_volatile_registers_changed=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_actor_damage.obj');p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-damage-tick-match/random-audit');a=p.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    obj=a.object.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_damage_tick_core');section=sections[owner['section']];source,listing,_,_=owner_instructions(a.object,symbols,sections,owner['name'])
    hs=next(s for s in symbols.values() if s['name']=='_random_step');hr,_,_,_=owner_instructions(a.object,symbols,sections,hs['name']);assert not sections[hs['section']]['relocations'];helper_proof=helper(hr)
    step_refs=[r for r in section['relocations'] if r['kind']==20 and r['symbol']['name']=='_random_step'];assert len(step_refs)==4
    first=next(i for i,r in enumerate(source) if r['address']<=step_refs[0]['offset']<r['address']+len(r['data']))
    assert [(r['mnemonic'],r['operands']) for r in source[first-3:first]]==[('MOV','edx, dword ptr [esi + 0x1e8]'),('ADD','esp, 0x18'),('PUSH','edx')]
    begin=source[first-3]['address'];publish=next(i for i,r in enumerate(source) if r['address']>step_refs[-1]['offset'] and r['mnemonic']=='MOV' and r['operands'].startswith('dword ptr [esi + 0x1e8],'))
    stop_new=source[publish+1]['address'];new_rows=[r for r in source if begin<=r['address']<=stop_new];calls={};new_constants={}
    def constant(data):return ('constant',struct.unpack('<d' if len(data)==8 else '<f',data)[0].hex())
    for r in section['relocations']:
        if not begin<=r['offset']<stop_new:continue
        row=next(v for v in source if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol']
        if r['kind']==20:assert s['name']=='_random_step';calls[row['address']]='rng-step'
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F');width=8 if 'QWORD' in row['operands'].upper() else 4;at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];new_constants[row['address']]=constant(sections[s['section']]['data'][at:at+width])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    start,stop=0x435ca0,0x435d7e;original=[];old_constants={}
    for r in json.loads((ROOT/'META/m4/target600/b3/original/004357d0.disasm.json').read_text()):
        va=int(r['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(r['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(r['operands']);original.append(dict(address=va-start,data=data,mnemonic=r['mnemonic'],operands=arg));assert r['mnemonic']!='CALL' and not r['mnemonic'].startswith('J')
        if r['mnemonic'].startswith('F'):
            m=re.search(r'\[0x([0-9a-f]+)\]',arg)
            if m:old_constants[va-start]=constant(image_bytes(image,int(m[1],16),8 if 'double' in arg else 4))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,stop-start)
    original.append(dict(address=stop-start,data=b'',mnemonic='BOUNDARY',operands=''))
    old,oc,ov=trace(original,old_constants,{}, {},None,image,False,0,0,0,False,False,rng=True,rng_step=bv,stop_address=stop-start)
    new,nc,nv=trace(new_rows,new_constants,calls,{},section,image,True,0,0,0,False,False,rng=True,rng_step=bv,stop_address=stop_new)
    if old!=new or oc!=nc:
        (out/'difference.json').write_text(json.dumps(dict(original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Ordinary RNG graphs differ: '+str(out/'difference.json'))
    assert ov=={r['address'] for r in original[:-1]} and nv=={r['address'] for r in new_rows if r['address']<stop_new}
    result=dict(original_va='004357d0',rng_start=hex(start),rng_end_exclusive=hex(stop),original_rng_bytes=stop-start,source_start=begin,source_end=stop_new,all_original_and_source_region_instructions_covered=True,unsigned32_arbitrary_input_graphs_equal=True,spin_f32_and_final_state_publication_equal=True,helper=helper_proof,byte_match=False,whole_owner_graph_proved=False,object_sha256=digest(obj),graph_sha256=digest(json.dumps((old,oc)).encode()),
                limits=['Integer multiply is low32, shifts logical, bitwise operations exact; helper AST is interpreted, not presumed.','All QWORD FILD inputs have proven zero high word and masked7fff low word. PC53 spin arithmetic/store order agrees.','Other caller stages, source guards and whole-owner composition remain unproved here.'])
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing);print(json.dumps(result))


if __name__=='__main__':main()
