"""Verify production/observed COFF relation and actual native linked definitions."""
import json
import argparse
import re
import struct
from pathlib import Path
from compare_ai_damage_query_math import ROOT,parse_coff,digest,image_bytes


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=ROOT/'build/ai-response-finish-match');p.add_argument('--stem',default='bh_actor_response',choices=['bh_actor_response','bh_actor_damage']);a=p.parse_args();base=a.directory
    prod=(ROOT/('build/game/'+a.stem+'.obj')).read_bytes();observed=(base/'response-observed.obj').read_bytes()
    ps,pc=parse_coff(prod);ns,nc=parse_coff(observed)
    aliases={'_'+a:'_'+b for a,b in [('rand','tail_rand'),('bh_entity_set_query','tail_set_query'),('bh_sound_stop','tail_stop'),('bh_camera_curve','tail_curve'),('bh_script_debris_create','tail_debris'),('bh_sound_request','tail_sound'),('bh_remote_choose_slot','tail_loot'),('bh_entity_acquire','tail_acquire'),('bh_target_construct_moving','tail_construct'),('bh_entities_query','tail_broadcast'),('bh_entity_release','tail_release')]}
    pf={s['name']:s for s in ps.values() if s['type']==32 and s['section']>0};nf={s['name']:s for s in ns.values() if s['type']==32 and s['section']>0};assert pf.keys()==nf.keys()
    exe=(base/'ai-response-finish-match.exe').read_bytes();map_text=(base/'ai-response-finish-match.map').read_text();proofs=[];used_aliases=set()
    def address(name):
        found=re.findall(r'^\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{8})\s',map_text,re.M);assert len(found)==1,name
        return int(found[0],16)
    # Private functions are located from the actual native CALL/callback, since
    # VC5's map omits their names. Public roots anchor every recursive check.
    visited=set();linked_aliases=set()
    def linked(symbol,va):
        if symbol['name'] in visited:return
        visited.add(symbol['name']);section=nc[symbol['section']];code=bytearray(image_bytes(exe,va,len(section['data'])))
        for r in section['relocations']:
            at,s=r['offset'],r['symbol'];addend=struct.unpack_from('<I',section['data'],at)[0]
            if r['kind']==20:
                assert code[at-1]==0xe8;target=va+at+4+struct.unpack_from('<i',code,at)[0]
                if s['storage']==3:linked(nf[s['name']],target)
                else:
                    assert target==address(s['name'])
                    if s['name'] in aliases.values():linked_aliases.add(s['name'])
            else:
                assert r['kind']==6;target=struct.unpack_from('<I',code,at)[0]
                if s['section']==symbol['section']:assert target==va+s['value']+addend
                elif s['type']==32:
                    assert not addend
                    if s['storage']==3:linked(nf[s['name']],target)
                    else:assert target==address(s['name'])
                else:
                    assert s['section']>0 and section['data'][at-2] in (0xd8,0xd9,0xdc,0xdd)
                    width=8 if section['data'][at-2] in (0xdc,0xdd) else 4;offset=s['value']+addend
                    assert image_bytes(exe,target,width)==nc[s['section']]['data'][offset:offset+width]
            code[at:at+4]=section['data'][at:at+4]
        assert code==section['data'],symbol['name']
    for name,s in pf.items():
        a,b=pc[s['section']],nc[nf[name]['section']];assert a['data']==b['data'],name
        def relocations(section,rename):
            return [(r['offset'],r['kind'],rename.get(r['symbol']['name'],r['symbol']['name']),r['symbol']['value'],r['symbol']['type'],r['symbol']['storage']) for r in section['relocations']]
        assert relocations(a,aliases)==relocations(b,{}),name
        used_aliases|={aliases[r['symbol']['name']] for r in a['relocations'] if r['symbol']['name'] in aliases}
        proofs.append(dict(function=name,owned_bytes=len(a['data']),unrelocated_section_sha256=digest(a['data'])))
        if s['storage']!=3:linked(nf[name],address(name))
    assert visited==set(nf) and linked_aliases==used_aliases
    result=dict(production_object_sha256=digest(prod),observed_object_sha256=digest(observed),native_executable_sha256=digest(exe),all_owner_function_and_attached_table_bytes_identical=True,only_declared_relocation_aliases_differ=True,all_observed_owner_definitions_linked=True,aliases=aliases,aliases_used=sorted(used_aliases),functions=proofs,
                limits=['Raw section equality relates these two source variants; it is not a byte match against the original.','Trace wrappers are native-test-only and delegate to real typed dependencies. Shared dependency internals are not independently original-executed by this comparison.'])
    (base/'observed-relation.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('aliases','functions')}))


if __name__=='__main__':main()
