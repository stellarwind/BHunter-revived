"""Build/check the M1 role catalog and first-slice dependency graph from evidence."""
import argparse
from collections import Counter, deque
import csv
import hashlib
import json
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'META/m1'


def read_tsv(path):
    with (ROOT/path).open(encoding='utf-8-sig',newline='') as f:
        return list(csv.DictReader(f,delimiter='\t'))


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def import_thunks(exe):
    pe=struct.unpack_from('<I',exe,60)[0];opt=pe+24
    assert struct.unpack_from('<H',exe,opt)[0]==0x10b
    base=struct.unpack_from('<I',exe,opt+28)[0]
    sections=[]
    for i in range(struct.unpack_from('<H',exe,pe+6)[0]):
        at=opt+struct.unpack_from('<H',exe,pe+20)[0]+40*i
        size,rva,rawsize,raw=struct.unpack_from('<IIII',exe,at+8)
        sections.append((rva,rawsize,raw))
    def offset(rva):return next(raw+rva-start for start,size,raw in sections if start<=rva<start+size)
    def string(rva):return exe[offset(rva):].split(b'\0',1)[0].decode('ascii')
    descriptor=offset(struct.unpack_from('<I',exe,opt+104)[0]);imports={}
    while True:
        lookup,_,_,name,iat=struct.unpack_from('<IIIII',exe,descriptor)
        if not any((lookup,name,iat)):break
        library=string(name);i=0
        while True:
            item=struct.unpack_from('<I',exe,offset((lookup or iat)+4*i))[0]
            if not item:break
            imports[base+iat+4*i]=(library,'ordinal:'+str(item&0xffff) if item&0x80000000 else string(item+2))
            i+=1
        descriptor+=20
    thunks=[]
    for va in (0x47d360,0x47d366,0x47d36c,0x47d658):
        at=offset(va-base)
        assert exe[at:at+2]==b'\xff\x25',hex(va)
        iat=struct.unpack_from('<I',exe,at+2)[0]
        library,name=imports[iat]
        thunks.append(dict(address=f'{va:08x}',end=f'{va+5:08x}',role='import_thunk',owner='system_imports',
                           boundary_review='verified six-byte JMP [IAT] and PE import descriptor',library=library,name=name,iat=f'{iat:08x}'))
    return thunks


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    decisions=json.loads((OUT/'review-decisions.json').read_text())
    exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(exe).hexdigest()==decisions['reference_exe_sha256']
    boundaries=read_tsv('META/m1/review/boundaries.tsv')
    observed={r['address']:r for r in read_tsv('analysis/function-map.tsv')}
    manual={r['address']:r for r in decisions['decisions']}
    assert len(manual)==len(decisions['decisions'])
    thunks=import_thunks(exe);thunk_by_va={r['address']:r for r in thunks}
    records=[]
    for b in boundaries:
        address=b['address'];old=observed[address];tags=b['tags'].split('|')
        role='unknown';owner='unassigned_dependency_review';review='observed_only'
        evidence=['analysis/function-map.tsv','META/m1/review/boundaries.tsv']
        confidence='unreviewed attribution; mapped body is an observation, not an admission'
        behavior='Unknown; may include proprietary engine, compiler/runtime or shared behavior; inspect before assignment'
        if 'library-crt-verified' in tags:
            role='library_verified';owner='crt_runtime';review='prior_library_comparison'
            evidence+=['analysis/crt-comparison.json','analysis/crt-symbols.tsv'];confidence='prior COFF section comparison; relocation operands masked'
            behavior='Verified CRT section; reuse library rather than reconstruct gameplay'
            if any(token in b['name'] for token in ('unwind','except_handler','__ftol')):
                role='compiler_support';owner='compiler_runtime';behavior='Compiler-generated conversion/SEH support; exclude gameplay assignments'
        if 'recovered-math-finite-tested' in tags:
            role='engine_candidate';owner='math_foundations';review='prior_math_behavior_and_layout'
            evidence+=['analysis/math/RECOVERY.md','META/function-status.json'];confidence='finite-fixture behavior checked; historical ownership unresolved'
            behavior='Recovered vector/matrix routine; numerical limitations remain in implementation ledger'
        if address in thunk_by_va:
            role='import_thunk';owner='system_imports';review='pe_iat_verified';confidence='exact JMP encoding and import target'
            behavior=thunk_by_va[address]['library']+'!'+thunk_by_va[address]['name']
        if address in manual:
            d=manual[address];role=d['role'];owner=d['owner'];behavior=d['behavior'];review='frontier_role_and_boundary_review';confidence=d['confidence']
            evidence+=['META/m1/review-decisions.json']
            folder=next(name for name in ('review','supplement','runtime-extra') if (OUT/name/f'{address}.asm.txt').exists())
            assert (OUT/folder/f'{address}.asm.txt').exists()
            evidence+=[f'META/m1/{folder}/{address}.asm.txt',f'META/m1/{folder}/{address}.c.txt']
            if role=='library_verified':evidence+=['META/m1/crt-frontier.json' if address!='0047e730' else 'analysis/crt-comparison.json']
        records.append(dict(address=address,name=b['name'],body_ranges=b['ranges'].split('|'),role=role,owner=owner,
                            provenance_confidence=confidence,boundary_review=review,behavior=behavior,
                            abi=manual.get(address,{}).get('abi','provisional inferred signature: '+b['signature']),
                            assigned_next=manual.get(address,{}).get('assigned_next',False),
                            callback_review=('registration_and_body_reviewed' if address in {'00471ff0','00472270','00472330','00471e00','00479090','00479140','004791f0','00473c40'} else 'candidate_only') if 'candidate-function-code-pointer' in tags else 'not_tagged_candidate',
                            evidence=evidence))
    by_va={r['address']:r for r in records}
    assert set(manual)<=set(by_va)
    assigned=[r for r in records if r['assigned_next']]
    assert len(assigned)==9 and all(r['role']=='engine_candidate' and r['boundary_review']=='frontier_role_and_boundary_review' for r in assigned)
    edges=[dict(source=r['caller'],site=r['callsite'],target=r['target'],name=r['name'],kind='observed_call') for r in read_tsv('analysis/call-edges.tsv')]
    callbacks=[('004723c0','00471ff0','WNDCLASSEXA.lpfnWndProc'),('004723c0','00472270','DirectDrawEnumerateA gathering'),
               ('004723c0','00472330','DirectDrawEnumerateA selection'),('004723c0','00471e00','CreateThread'),
               ('00479120','00479090','renderer registration via 00471df0'),('00471e00','00479090','render slot 00491880'),
               ('00479200','004791f0','archive progress slot 004f9618 during initialization'),('0047cbd0','004791f0','optional first-access callback installed by bootstrap'),
               ('00479200','00479140','simulation registration via 00471c00'),('004723c0','00479140','main loop slot 00491884'),
               ('00471ff0','00473c40','application message hook slot 00491888'),('00471ff0','00471be0','initial default message hook')]
    edges += [dict(source=a,target=b,kind='reviewed_callback_path',evidence=why) for a,b,why in callbacks]
    for edge in edges:
        if edge['source'] in {r['address'] for r in assigned}:
            assert edge['target'] in by_va and by_va[edge['target']]['role']!='unknown','Unowned archive dependency: '+edge['target']
    adjacency={}
    for e in edges:adjacency.setdefault(e['source'],set()).add(e['target'])
    roots=['004723c0'];reachable=set(roots);queue=deque(roots)
    while queue:
        for target in adjacency.get(queue.popleft(),[]):
            if target not in reachable:reachable.add(target);queue.append(target)
    unresolved=[r for r in read_tsv('META/m1/review/control-flow.tsv') if r['computed']=='true' and not r['to'] and r['function'] in reachable]
    dependencies=[dict(address=a,role=by_va[a]['role'] if a in by_va else 'system_import' if a.startswith('EXTERNAL:') else 'unknown',
                       owner=by_va[a]['owner'] if a in by_va else 'system_imports' if a.startswith('EXTERNAL:') else 'unassigned_dependency_review',
                       assigned_next=a in {r['address'] for r in assigned}) for a in sorted(reachable)]
    auxiliary=[r for r in thunks if r['address'] not in by_va]
    auxiliary.append(dict(address='00450930',end='00450978',role='shared_tail',owner='00479090',boundary_review='single observed JMP at 00479111; included in owning function body',independent_recovery_assignment=False))
    data_format=struct.unpack_from('<IIIIII',exe,0x47d640-0x400c00)
    assert data_format==(24,16,1,80,44,0x47d380)
    auxiliary.append(dict(address='0047d380',end='0047d657',role='data',owner='window_input',
                          boundary_review='44 x 16-byte DIOBJECTDATAFORMAT entries followed by 24-byte DIDATAFORMAT; referenced by 004712b0 SetDataFormat call',
                          data_format=list(data_format),independent_recovery_assignment=False,
                          sha256=hashlib.sha256(exe[0x47d380-0x400c00:0x47d658-0x400c00]).hexdigest()))
    catalog=dict(schema_version=1,reference_exe_sha256=decisions['reference_exe_sha256'],
                 scope='Role/provenance catalog, not an implementation count. Unknowns and callback hypotheses remain explicit.',records=records,auxiliary_records=auxiliary)
    graph=dict(schema_version=1,checkpoint='startup -> archive -> test.lev/test.pth world -> fixed-step simulation/render; actual playable acceptance scenario still needs M3 capture',
               roots=roots,closure_limit='static direct calls plus specifically reviewed callback paths; COM, entity callbacks and jump tables are not a complete dynamic closure',
               dependencies=dependencies,edges=[e for e in edges if e['source'] in reachable],unresolved_indirect_sites=unresolved,
               next_family=[r['address'] for r in assigned])
    summary=dict(mapped_function_records=len(records),roles=dict(sorted(Counter(r['role'] for r in records).items())),
                 observed_callback_candidates=sum('candidate-function-code-pointer' in r['tags'] for r in boundaries),
                 reviewed_function_decisions=len(manual),verified_import_thunk_slots=len(thunks),additional_thunks_not_separate_ghidra_functions=sum(r['address'] not in by_va for r in thunks),
                 assigned_archive_functions=len(assigned),reachable_nodes_with_reviewed_callback_paths=len(reachable),
                 reachable_unknown_functions=sum(r['role']=='unknown' for r in dependencies),unresolved_indirect_sites=len(unresolved),
                 no_new_game_implementation=True,
                 inputs={str(p.relative_to(ROOT)).replace('\\','/'):sha(p) for p in [ROOT/'analysis/function-map.tsv',ROOT/'analysis/call-edges.tsv',ROOT/'analysis/crt-comparison.json',OUT/'review-decisions.json',OUT/'review/boundaries.tsv',OUT/'review/control-flow.tsv',OUT/'crt-frontier.json']})
    for name,value in [('role-catalog.json',catalog),('first-slice-dependencies.json',graph),('catalog-summary.json',summary)]:
        text=json.dumps(value,indent=2)+'\n';path=OUT/name
        if args.check:assert path.read_text()==text,'Catalog drift: '+str(path)
        else:path.write_text(text,encoding='utf-8')
    print(json.dumps(summary,indent=2))


if __name__=='__main__':main()
