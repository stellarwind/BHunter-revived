"""Current linked launch/receive definitions and immutable CRT conversion; no owner graph claim."""
import json
import re
import struct
from compare_ai_damage_query_math import ROOT,owner_instructions,parse_coff,digest,image_bytes


def main():
    out=ROOT/'build/ai-launch-match/link-audit';out.mkdir(parents=True,exist_ok=True)
    path=ROOT/'build/game/bh_actor_launch.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj);functions={s['name']:s for s in symbols.values() if s['section']>0 and s['type']==32}
    exe=(ROOT/'build/game/BHunter-source.exe').read_bytes();mapping=(ROOT/'build/game/BHunter-source.map').read_text();original=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(original)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',mapping,re.M);assert len(found)==1,name;return int(found[0],16)
    visited=set();bindings=[]
    def linked(s,va):
        if s['name'] in visited:return
        visited.add(s['name']);section=sections[s['section']];code=bytearray(image_bytes(exe,va,len(section['data'])))
        for r in section['relocations']:
            at,target=r['offset'],r['symbol'];addend=struct.unpack_from('<I',section['data'],at)[0]
            if r['kind']==20:
                assert not addend and code[at-1]==0xe8;dest=va+at+4+struct.unpack_from('<i',code,at)[0]
                if target['storage']==3:linked(functions[target['name']],dest)
                else:assert dest==address(target['name'])
                bindings.append(dict(owner=s['name'],offset=at,callee=target['name'],linked_va=hex(dest)))
            else:
                assert r['kind']==6 and target['section']>0
                opcode=section['data'][at-2];assert opcode in (0xd8,0xd9,0xdc,0xdd);width=8 if opcode in (0xdc,0xdd) else 4;offset=target['value']+addend
                dest=struct.unpack_from('<I',code,at)[0];assert image_bytes(exe,dest,width)==sections[target['section']]['data'][offset:offset+width]
            code[at:at+4]=section['data'][at:at+4]
        assert code==section['data'],s['name']
    owners=[]
    for name,va,length in [('_bh_actor_launch',0x43bca0,533),('_bh_actor_launch_receive',0x426d50,349)]:
        linked(functions[name],address(name));rows,listing,end,_=owner_instructions(path,symbols,sections,name);(out/(name[1:]+'.asm')).write_text(listing)
        owners.append(dict(original_va=hex(va),original_bytes=length,symbol=name,source_instruction_bytes=end,actual_linked_section_matches=True))
    crt=image_bytes(original,0x47de20,39);assert image_bytes(exe,address('__ftol'),39)==crt
    result=dict(owners=owners,object_sha256=digest(obj),executable_sha256=digest(exe),all_owner_private_helper_constants_and_real_callees_linked=True,private_definitions=sorted(visited),bindings=bindings,ftol_original39_bytes_raw_match=True,ftol_counted_as_game_owner=False,byte_match=False,whole_owner_graph_proved=False)
    (out/'linked.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('bindings','private_definitions')}))


if __name__=='__main__':main()
