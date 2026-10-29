"""Actual linked ordinary damage-tick candidate; no arithmetic graph claim."""
import json
import re
import struct
from compare_ai_damage_query_math import ROOT,owner_instructions,parse_coff,digest,image_bytes


def main():
    out=ROOT/'build/ai-damage-tick-match/link-audit';out.mkdir(parents=True,exist_ok=True)
    path=ROOT/'build/game/bh_actor_damage.obj';obj=path.read_bytes();symbols,sections=parse_coff(obj);functions={s['name']:s for s in symbols.values() if s['section']>0 and s['type']==32}
    exe=(ROOT/'build/game/BHunter-source.exe').read_bytes();map_text=(ROOT/'build/game/BHunter-source.map').read_text();visited=set();bindings=[]
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(found)==1,name
        return int(found[0],16)
    def linked(symbol,va):
        if symbol['name'] in visited:return
        visited.add(symbol['name']);section=sections[symbol['section']];code=bytearray(image_bytes(exe,va,len(section['data'])))
        for r in section['relocations']:
            at,s=r['offset'],r['symbol'];addend=struct.unpack_from('<I',section['data'],at)[0]
            if r['kind']==20:
                assert code[at-1]==0xe8;target=va+at+4+struct.unpack_from('<i',code,at)[0]
                if s['storage']==3:linked(functions[s['name']],target)
                else:assert target==address(s['name'])
                bindings.append(dict(owner=symbol['name'],offset=at,callee=s['name'],linked_va=hex(target)))
            else:
                assert r['kind']==6;target=struct.unpack_from('<I',code,at)[0]
                if s['type']==32:
                    assert not addend
                    if s['storage']==3:linked(functions[s['name']],target)
                    else:assert target==address(s['name'])
                else:
                    assert s['section']>0 and section['data'][at-2] in (0xd8,0xd9,0xdc,0xdd)
                    width=8 if section['data'][at-2] in (0xdc,0xdd) else 4;offset=s['value']+addend
                    assert image_bytes(exe,target,width)==sections[s['section']]['data'][offset:offset+width]
            code[at:at+4]=section['data'][at:at+4]
        assert code==section['data'],symbol['name']
    core=functions['_bh_actor_damage_tick_core'];wrapper=functions['_bh_actor_damage_tick'];linked(core,address(core['name']));linked(wrapper,address(wrapper['name']))
    assert [r['symbol']['name'] for r in sections[wrapper['section']]['relocations']].count(core['name'])==1
    rows,listing,end,_=owner_instructions(path,symbols,sections,core['name'])
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=json.loads((ROOT/'META/m4/target600/b3/original/004357d0.disasm.json').read_text());assert b''.join(bytes.fromhex(r['bytes']) for r in original)==image_bytes(image,0x4357d0,1709)
    result=dict(original_va='004357d0',original_owner_bytes=1709,source_core_bytes=end,byte_match=False,whole_owner_graph_proved=False,actual_wrapper_calls_core=True,entire_core_wrapper_private_helpers_callbacks_constants_and_real_dependencies_linked=True,object_sha256=digest(obj),executable_sha256=digest(exe),verified_definitions=sorted(visited),bindings=bindings,
                limits=['Actual linked definitions only; complete x87/integer-RNG/branch/store graph remains WIP.','Source context/clock/registry/legacy-angle and status adapters are explicit.'])
    (out/'linked.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing);print(json.dumps({k:v for k,v in result.items() if k not in ('bindings','verified_definitions')}))


if __name__=='__main__':main()
