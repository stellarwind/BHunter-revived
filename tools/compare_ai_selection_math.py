"""Target-selection caller graphs on actual COFF/original instructions; no execution."""
import itertools
import argparse
import json
import re
from pathlib import Path
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--object',type=Path,default=ROOT/'build/game/bh_target_selection.obj');p.add_argument('--output-directory',type=Path,default=ROOT/'build/ai-selection-match/math-audit');a=p.parse_args();out=a.output_directory;out.mkdir(parents=True,exist_ok=True)
    path=a.object;obj=path.read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_target_select');section=sections[owner['section']];source,listing,end,_=owner_instructions(path,symbols,sections,owner['name']);assert not section['relocations']
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=[];start=0x421610;stop=0x421740
    for row in json.loads((ROOT/'META/m4/playable/ai-aggression/00421610-asm.json').read_text()):
        va=int(row['address'],16)
        if not start<=va<stop:continue
        data=bytes.fromhex(row['bytes']);assert data==image_bytes(image,va,len(data));arg=', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):arg=hex(int(arg,16)-start)
        original.append(dict(address=va-start,data=data,mnemonic=row['mnemonic'],operands=arg))
    assert b''.join(r['data'] for r in original)==image_bytes(image,start,304)
    classes=[(),(1,),(2,),(6,)]+list(itertools.product((1,2,6),repeat=2));proofs=[];ocover=set();ncover=set()
    for player,kinds,excluded,cut,bits in itertools.product(range(3),classes,(-1,0,1),(False,True),range(4)):
        settings=dict(player=player,classes=kinds,excluded=excluded,cut=cut)
        old,oc,ov=trace(original,{}, {},{},None,image,False,0,bits,0,False,False,selection=settings)
        new,nc,nv=trace(source,{}, {},{},section,image,True,0,bits,0,False,False,selection=settings)
        if old!=new or oc!=nc:
            (out/'difference.json').write_text(json.dumps(dict(settings=settings,bits=bits,original=old,source=new,original_comparisons=oc,source_comparisons=nc),indent=2)+'\n');raise AssertionError('Selection instruction graphs differ: '+str(out/'difference.json'))
        ocover|=ov;ncover|=nv;proofs.append(digest(json.dumps((old,oc)).encode()))
    assert ocover=={r['address'] for r in original}
    missing=[r for r in source if r['address'] not in ncover]
    assert [(r['mnemonic'],r['operands']) for r in missing]==[('POP','edi'),('POP','esi'),('XOR','eax, eax'),('POP','ebx'),('MOV','esp, ebp'),('POP','ebp'),('RET','')]
    # Compare one complete loop transition from an arbitrary incumbent score,
    # selected pointer and successor, rather than assuming a two-node bound.
    original_head=0x42169d-start;original_stop=0x42172e-start
    source_head=next(r['address'] for r in source if r['mnemonic']=='MOV' and r['operands']=='eax, dword ptr [edi + 0x8]')
    source_link=next(i for i,r in enumerate(source) if r['mnemonic']=='MOV' and r['operands']=='esi, dword ptr [esi + 0xf0]');source_stop=source[source_link+1]['address']
    transitions=[]
    for kind_,excluded,cut,bits in itertools.product((1,2,6),(-1,0,1),(False,True),range(2)):
        settings=dict(player=0,classes=(kind_,),excluded=excluded,cut=cut,iteration=True);states={}
        def boundary(side):
            def capture(regs,memory):
                states[side]=(regs['ESI'],memory[('selection-result',0)] if side else regs['EBX'],memory[-76 if side else -56])
                if side:assert memory[('selection-result',4)]==memory[-76]
            return capture
        old,oc,_=trace([r for r in original if r['address']>=original_head],{}, {},{},None,image,False,0,bits,0,False,False,selection=settings,stop_address=original_stop,boundary_contract=boundary(False))
        new,nc,_=trace([r for r in source if r['address']>=source_head],{}, {},{},section,image,True,0,bits,0,False,False,selection=settings,stop_address=source_stop,boundary_contract=boundary(True))
        assert old==new and oc==nc and states[False]==states[True],(settings,bits,old,new,states)
        transitions.append(digest(json.dumps((old,oc,states[False])).encode()))
    exe=(ROOT/'build/game/BHunter-source.exe').read_bytes();mapping=(ROOT/'build/game/BHunter-source.map').read_text();va=re.findall(r'^\s+\w+:\w+\s+_bh_target_select\s+([0-9a-fA-F]{8})\s',mapping,re.M);assert len(va)==1;va=int(va[0],16);assert image_bytes(exe,va,len(section['data']))==section['data']
    result=dict(original_va='00421610',original_owner_bytes=304,source_owner_bytes=end,valid_PC53_graphs_equal=True,all_original_instructions_covered=True,cases=len(proofs),object_sha256=digest(obj),executable_sha256=digest(exe),graph_digest=digest(json.dumps(proofs).encode()),byte_match=False,strict_all_profile_cosmetic_only_proved=False,source_unvisited=[(r['address'],r['mnemonic'],r['operands']) for r in missing],actual_linked_bytes_verified=True,
                limits=['Finite PC53 nearest/masked value graphs; no NaN payload/exception flag or runtime precision-transition claim.','Source origin/result/query/budget guards and result score/visited publication are explicit API adapters.','Graph configurations cover zero/one/two actors, player absent/ineligible/eligible, classes, exclusions, comparison decisions and post-query successor/weight mutation. Arbitrary-length loop induction not yet proved.','Position callbacks are typed dependency contracts, not original-inner-body proofs.'])
    result.update(arbitrary_incumbent_score_pointer_successor_transition_paths=len(transitions),loop_transition_digest=digest(json.dumps(transitions).encode()),valid_list_loop_induction=True,all_valid_source_instructions_covered=True)
    result['limits'][2]='Whole entry/return paths plus loop transition from arbitrary incumbent score/pointer and successor establish finite valid-list induction while traversal budget and callback guards hold. Post-query origin/weight/successor mutation is explicit.'
    (out/'graphs.json').write_text(json.dumps(result,indent=2)+'\n');(out/'source.asm').write_text(listing);print(json.dumps(result))


if __name__=='__main__':main()
