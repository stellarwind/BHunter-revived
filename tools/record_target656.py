"""Admit one complete family toward606+50, using the fresh full source build."""
import argparse,hashlib,re,shutil,copy
from pathlib import Path
from record_pickup_basic import ROOT,read,write,sha,reference,active
HERE=ROOT/'META/m4/target656'
def main():
    parser=argparse.ArgumentParser();parser.add_argument('family');args=parser.parse_args();folder=HERE/args.family
    spec=read(folder/'admission.json');ledger,raw=reference();build=read(ROOT/'build/m0/result.json');current=active(ledger)
    assert len(current)==spec['baseline'] and build['passed'] and not build['original_code_required'] and not build['test']['failures']
    refreshed=[];changed={}
    for r in current:
        source=r['source'];assert sha(ROOT/source)==build['sources'][source]
        if r['source_sha256']==build['sources'][source]:continue
        assert source in spec.get('context_sources',{}),r['id']
        assert r['implementation']=='func'
        previous=read(ROOT/r['evidence'][0]);snapshot=previous['source_snapshots'][source]
        assert sha(ROOT/snapshot)==r['source_sha256']
        for scenario in spec['context_sources'][source]:
            f=next(s for s in build['scenarios'] if s.get('scenario')==scenario);assert f['checks'] and not f['failures']
        new=copy.deepcopy(r);new['id']=r['id']+'-context-'+args.family;new['source_sha256']=build['sources'][source]
        r['historical_variant']=True;r['historical_source_path']=source;r['source']=snapshot;r['validation']['superseded_by']=new['id']
        new['validation']['supersedes']=r['id'];new['validation']['raw_byte_match']=False
        new['validation']['context_fixtures']={name:next(s for s in build['scenarios'] if s.get('scenario')==name) for name in spec['context_sources'][source]}
        new['evidence']=['META/m4/target656/'+args.family+'/validation.json']+new['evidence'];ledger['records'].append(new)
        changed[source]=snapshot;refreshed.append(dict(old=r['id'],new=new['id'],snapshot=snapshot))
    for old in list(ledger['support_implementations']):
        source=old.get('source')
        if source not in changed or old.get('historical_variant') or old.get('validation',{}).get('superseded_by'):continue
        new=copy.deepcopy(old);new['id']=old['id']+'-context-'+args.family;new['source_sha256']=build['sources'][source]
        old['historical_variant']=True;old['historical_source_path']=source;old['source']=changed[source];old.setdefault('validation',{})['superseded_by']=new['id']
        new['validation']['supersedes']=old['id'];new['evidence']=['META/m4/target656/'+args.family+'/validation.json']+new.get('evidence',[]);ledger['support_implementations'].append(new)
    fixture=next(s for s in build['scenarios'] if s.get('scenario')==spec['scenario']);assert fixture['checks'] and not fixture['failures']
    assert build['missing_'+spec['module']+'_control']=='rejected with LNK2001'
    linkmap=(ROOT/'build/m0/bhunter-integration.map').read_text(errors='replace')
    for o in read(folder/'owners.json'):
        a=o['original_va'];assert a not in {r['original_va'] for r in current}
        rs=read(folder/'original'/(a+'.disasm.json'));body=b''.join(bytes.fromhex(r['bytes']) for r in rs)
        assert len(body)==o['bytes'] and raw(int(a,16),len(body))==body and rs[-1]['mnemonic']=='RET'
        assert sha(ROOT/o['source'])==build['sources'][o['source']]
        assert re.search(r'(?<!\w)_'+re.escape(o['function'])+r'(?=\s|$)',linkmap),o['function']
        identifier=a+'-functional-c-target656-'+args.family
        for old in ledger['records']:
            if old['original_va']==a and not old.get('historical_variant') and not old.get('validation',{}).get('superseded_by'):
                assert old['implementation'] in ('wip','not_started');old['historical_variant']=True;old.setdefault('validation',{})['superseded_by']=identifier
        ledger['records'].append(dict(id=identifier,original_va=a,function=o['function'],source=o['source'],source_sha256=build['sources'][o['source']],role=o['role'],provenance_confidence=o['provenance_confidence'],implementation='func',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,validation=dict(boundary=dict(start=a,end_exclusive=f'{int(a,16)+len(body):08x}',bytes=len(body),sha256=hashlib.sha256(body).hexdigest()),fixtures=fixture,raw_byte_match=False,original_code_executed=False,limits=spec['limits']),evidence=['META/m4/target656/'+args.family+'/validation.json','META/m4/target656/'+args.family+'/original/'+a+'.disasm.json']))
    snapshots={}
    for name in spec['sources']+['tests/checkpoint.c','tools/m0.py']:
        digest=sha(ROOT/name);assert build['sources'].get(name,digest)==digest
        dest=folder/'sources'/(digest+'-'+Path(name).name+'.txt');dest.parent.mkdir(exist_ok=True);shutil.copyfile(ROOT/name,dest);snapshots[name]=dest.relative_to(ROOT).as_posix()
    for source in {o['source'] for o in read(folder/'owners.json')}:
        ledger['support_implementations'].append(dict(id='target656-'+args.family+'-'+Path(source).stem+'-bindings',source=source,source_sha256=build['sources'][source],role='unknown',provenance_confidence='Explicit source context/lifetime and bounded malformed-input adaptation; no independent original address.',implementation='adapter',integration='linked_harness',target='build/m0/bhunter-integration.exe',game_admitted=False,original_function_count=0,validation=dict(original_code_executed=False,contract='See source header and admission limits; shared source helpers are not additional original functions.'),evidence=['META/m4/target656/'+args.family+'/validation.json']))
    admitted=active(ledger);assert len(admitted)<=656
    for r in admitted:assert r['source_sha256']==sha(ROOT/r['source'])==build['sources'][r['source']]
    write(folder/'integration-real.json',build);shutil.copyfile(ROOT/'build/m0/bhunter-integration.map',folder/'link-map.txt')
    write(folder/'refreshed-variants.json',refreshed)
    write(folder/'validation.json',dict(passed=True,baseline_functions=len(current),new_owners=len(admitted)-len(current),active_original_functions=len(admitted),real_total=build['test'],fixture=fixture,sources={n:sha(ROOT/n) for n in snapshots},source_snapshots=snapshots,limits=spec['limits'],original_code_executed=False,game_launches=0))
    ledger['updated']='2026-10-26';ledger['current_source_frontier']='META/m4/target656/NEXT.md';ledger['latest_source_session']='META/m4/target656/session.json';write(ROOT/'META/function-status.json',ledger)
    print('Admitted',len(admitted),'whole source owners;',args.family,len(admitted)-len(current),'new')
if __name__=='__main__':main()
