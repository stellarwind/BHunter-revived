"""Compose original response-tick graphs and verify actual source dispatch."""
import json
import re
import struct
import subprocess
import sys
from compare_ai_damage_query_math import ROOT,trace,owner_instructions,parse_coff,digest,image_bytes
from compare_ai_response_motion_math import verify_linked


def main():
    out=ROOT/'build/ai-response-whole-match/composition';out.mkdir(parents=True,exist_ok=True)
    analyses=[('compare_ai_response_motion_math.py','build/ai-response-motion-match/math-audit/graphs.json',0x439935,0x439bfe,'valid_prefix_graphs_equal'),('compare_ai_response_smoke_math.py','build/ai-response-smoke-match/math-audit/graphs.json',0x439bfe,0x439dcd,'valid_window_graphs_equal'),('compare_ai_response_finish_math.py','build/ai-response-finish-match/math-audit/graphs.json',0x439dcd,0x43a1c7,'valid_tail_graphs_equal')]
    obj=(ROOT/'build/game/bh_actor_response.obj').read_bytes();symbols,sections=parse_coff(obj);owner=next(s for s in symbols.values() if s['name']=='_bh_actor_response_tick');section=sections[owner['section']];rows,listing,end,_=owner_instructions(ROOT/'build/game/bh_actor_response.obj',symbols,sections,owner['name'])
    contracts=[]
    for tool,path,start,stop,key in analyses:
        run=subprocess.run([sys.executable,str(ROOT/'tools'/tool)],capture_output=True,text=True);assert run.returncode==0,(tool,run.stderr)
        result=json.loads((ROOT/path).read_text());assert result[key] and result['object_sha256']==digest(obj)
        contracts.append(dict(analyzer=tool,analyzer_sha256=digest((ROOT/'tools'/tool).read_bytes()),original_start=hex(start),original_end_exclusive=hex(stop),original_bytes=stop-start,paths=result['cases'],graph_sha256=digest((ROOT/path).read_bytes()),limits=result['limits']))
    image=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert digest(image)=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original=json.loads((ROOT/'META/m4/target656/response/original/00439910.disasm.json').read_text());assert b''.join(bytes.fromhex(r['bytes']) for r in original)==image_bytes(image,0x439910,2231)
    prefix=[r for r in original if int(r['address'],16)<0x439935]
    expected=[('SUB',['ESP','0xac']),('PUSH',['EBX']),('PUSH',['EBP']),('PUSH',['ESI']),('MOV',['ESI','dword ptr [ESP + 0xbc]']),('PUSH',['EDI']),('PUSH',['ESI']),('CALL',['0x00437ae0']),('MOV',['EAX','dword ptr [ESI + 0x198]']),('MOV',['EDI','0x3f800000']),('ADD',['ESP','0x4'])]
    assert [(r['mnemonic'],r['operands']) for r in prefix]==expected
    assert sum(len(bytes.fromhex(r['bytes'])) for r in prefix)==37 and 37+sum(c['original_bytes'] for c in contracts)==2231
    constants={};calls={}
    for r in section['relocations']:
        row=next(v for v in rows if v['address']<=r['offset']<v['address']+len(v['data']));s=r['symbol']
        if r['kind']==20:calls[row['address']]={'_live':'live','_bh_math_environment_supported':'environment','_bh_actor_attack':'attack','_bh_actor_response_motion':'motion-stage','_bh_actor_response_route_smoke':'smoke-stage','_bh_actor_response_finish':'finish-stage'}[s['name']]
        else:
            assert r['kind']==6 and row['mnemonic'].startswith('F');at=s['value']+struct.unpack_from('<I',section['data'],r['offset'])[0];width=8 if 'QWORD' in row['operands'].upper() else 4
            constants[row['address']]=('constant',struct.unpack('<d' if width==8 else '<f',sections[s['section']]['data'][at:at+width])[0].hex())
    dispatch=[];covered=set()
    for smoked in (False,True):
        events,comparisons,coverage=trace(rows,constants,calls,{},section,image,True,0,0,0,False,False,special=True,tick=True,smoked=smoked)
        assert events==[('attack',('ptr','actor',0)),('stage','motion'),('stage','smoke'),('stage','finish',int(smoked))] and not comparisons
        covered|=coverage;dispatch.append(dict(smoked=smoked,events=events))
    linked=verify_linked(symbols,sections);(out/'linked.json').write_text(json.dumps(linked,indent=2)+'\n');(out/'source-wrapper.asm').write_text(listing)
    result=dict(original_va='00439910',original_owner_bytes=2231,original_prologue_attack_and_scale_setup_bytes=37,all_original_instruction_regions_composed=True,source_actual_valid_dispatch_verified=True,valid_PC53_owner_graph_composition=True,byte_match=False,strict_all_profile_cosmetic_only_proved=False,object_sha256=digest(obj),executable_sha256=linked['executable_sha256'],interpreter_sha256=digest((ROOT/'tools/compare_ai_damage_query_math.py').read_bytes()),contracts=contracts,dispatch=dispatch,source_wrapper_guard_only_unvisited=[r['address'] for r in rows if r['address'] not in covered],
                boundaries=['Original prologue forwards actor to attack and reloads scale; source typed attack success and owner/context checks are adapters.','Motion exits with empty x87 and stored fields; smoke reads those live fields and publishes the matching saved death position.','Smoke emission continues at439dcd with52 pending argument bytes; its original stack adjustment is included in finish proof.','Smoke skip jumps to43a18f; source forwards smoked0 and the finish proof bypasses the unused death-position input.','Finish resamples live phase/duration after query/sound and re-reads room/list ownership after notifications.'],
                limits=['Closed valid finite PC53 nearest/masked domain; all stage guards and typed dependencies must hold.','Source context/status/finite/registry/legacy-angle/mission bindings remain explicit adapters.','Attack/pursuit/effect/loot/curve/pool/sound/target implementations are shared caller contracts, not independent original-inner-body proofs.','No claim for other FP modes, NaN payloads, exception flags, arbitrary aliases or unrestricted reentrant mutation.'])
    (out/'composition.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('contracts','dispatch','boundaries','source_wrapper_guard_only_unvisited')}))


if __name__=='__main__':main()
