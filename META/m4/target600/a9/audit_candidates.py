import json,hashlib,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[4];sys.path.insert(0,str(ROOT/'tools'))
from record_pickup_basic import active,read
H=ROOT/'META/m4/target600';ledger=read(ROOT/'META/function-status.json');baseline=active(ledger);assert len(baseline)==550
exe=(ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(exe).hexdigest()==ledger['reference_exe_sha256'];pe=struct.unpack_from('<I',exe,60)[0];s=pe+24+struct.unpack_from('<H',exe,pe+20)[0];ss=[struct.unpack_from('<IIII',exe,s+i*40+8) for i in range(struct.unpack_from('<H',exe,pe+6)[0])]
def raw(a,n):
 for vs,va,sz,o in ss:
  if va<=a-0x400000 and a-0x400000+n<=va+sz:return exe[o+a-0x400000-va:o+a-0x400000-va+n]
 raise ValueError(hex(a))
roles={x['address']:x['role'] for x in read(ROOT/'META/m1/role-catalog.json')['records']};inventory={}
for line in (ROOT/'analysis/function-map.tsv').read_text(encoding='utf-8-sig').splitlines():
 x=line.split('\t')
 if len(x)>2 and len(x[0])==8:
  try:inventory[x[0]]=int(x[2])
  except ValueError:pass
families={};candidates=[]
for p in sorted(H.glob('*/owners.json')):
 rows=read(p);families[p.parent.name]=len(rows)
 for row in rows:row=dict(row);row['family']=p.parent.name;candidates.append(row)
addresses=[r['original_va'] for r in candidates];dupes=sorted(a for a in set(addresses) if addresses.count(a)>1);baseaddr={r['original_va'] for r in baseline};overlap=sorted(baseaddr&set(addresses));boundaryerrors=[];badroles=[];spanrows=[];evidence=[]
for row in candidates:
 a=row['original_va'];bs=row['boundary'];bs=bs if isinstance(bs,list) else [bs]
 if row['role'] not in ['game','engine_candidate'] or roles.get(a) in ['library_verified','compiler_support','import_thunk','shared_tail','data']:badroles.append(dict(address=a,claim=row['role'],old_catalog=roles.get(a)))
 for b in bs:
  start=int(b['start'],16);end=int(b['end_exclusive'],16);body=raw(start,end-start);digest=hashlib.sha256(body).hexdigest()
  if len(body)!=b['bytes'] or digest!=b['sha256']:boundaryerrors.append(dict(address=a,reason='raw byte/hash mismatch'))
  files=list((H/row['family']).rglob(a+'.disasm.json'))
  if not files:files=list(H.rglob(a+'.disasm.json'))
  matching=[]
  for p in files:
   data=read(p)
   if isinstance(data,list) and data and all('bytes' in x for x in data) and b''.join(bytes.fromhex(x['bytes']) for x in data)==body:matching.append((p,data))
  if not matching:boundaryerrors.append(dict(address=a,reason='no matching complete raw disassembly'))
  else:
   p,data=matching[0]
   terminal=data[-1]['mnemonic'];owning_tail=(terminal=='JMP' and data[-1].get('operands') and start<=int(data[-1]['operands'][0],16)<end)
   if terminal!='RET' and not owning_tail:boundaryerrors.append(dict(address=a,reason='terminal instruction is neither RET nor jump to own body'))
   position=start
   for d in data:
    if int(d['address'],16)!=position:boundaryerrors.append(dict(address=a,reason='noncontiguous instruction list'));break
    position+=len(bytes.fromhex(d['bytes']))
   if position!=end or len(data)!=b['instructions']:boundaryerrors.append(dict(address=a,reason='end/instruction count mismatch'))
   evidence.append(dict(address=a,raw_disassembly=p.relative_to(ROOT).as_posix(),bytes=len(body),instructions=len(data),sha256=digest,terminal=terminal,owning_internal_tail=bool(owning_tail),role=row['role'],old_catalog_role=roles.get(a,'absent')))
  spanrows.append((start,end,a))
spanoverlap=[]
for i,(a,z,owner) in enumerate(spanrows):
 for b,y,other in spanrows[i+1:]:
  if a<y and b<z:spanoverlap.append([owner,other])
basespans=[];exact=0;fallback=0
for row in baseline:
 bs=row.get('validation',{}).get('boundary');bs=bs if isinstance(bs,list) else [bs] if isinstance(bs,dict) else []
 parsed=False
 for b in bs:
  if 'start' in b and 'end_exclusive' in b:basespans.append((int(b['start'],16),int(b['end_exclusive'],16),row['original_va']));parsed=True
 if parsed:exact+=1
 elif row['original_va'] in inventory:
  a=int(row['original_va'],16);basespans.append((a,a+inventory[row['original_va']],row['original_va']));fallback+=1
basebyteoverlap=[]
for a,z,owner in spanrows:
 for b,y,other in basespans:
  if a<y and b<z:basebyteoverlap.append([owner,other])
result=dict(audit='Candidate owner identity/boundary/role exclusion audit; not admission or full integration',baseline_active_owners=len(baseline),baseline_unique_addresses=len(baseaddr),family_counts=families,present_candidate_records=len(candidates),present_unique_addresses=len(set(addresses)),projection_after_present=len(baseaddr|set(addresses)),projected_missing_to600=600-len(baseaddr|set(addresses)),duplicate_candidate_addresses=dupes,baseline_address_overlaps=overlap,candidate_byte_overlaps=spanoverlap,baseline_byte_overlaps=basebyteoverlap,raw_boundary_errors=boundaryerrors,excluded_role_conflicts=badroles,complete_raw_owner_bodies_verified=len(evidence),baseline_span_sources=dict(exact_metadata=exact,function_inventory_fallback=fallback,without_span=550-exact-fallback),owned_data_function_counts=0,owned_tail_notes=['00441d80 body includes 00442313..0044232d miss-copy tail after its RET. Last instruction 00442329 jumps back to 00441fc5 inside same owner. This is recovered within 441d80, no separate tail/function count.'],passed=not(dupes or overlap or spanoverlap or basebyteoverlap or boundaryerrors or badroles),owners=evidence,limits=['Only current immediate family owners.json files counted; pending source/body candidates without owner metadata are excluded.','Active baseline uses current550 ledger admission predicate; byte overlap uses exact metadata when present, otherwise historical function inventory lengths. No source completion inferred from inventory.','Ordinary complete original bodies (RET exits or internal owned jump tail) and family-reviewed game/engine roles count once. Shared countzero composition/helpers, originaldata/tables, CRT/compiler helpers and original-image adapters excluded. No source original/game execution performed by audit.','Parent fresh full/clean integration and missing-module controls remain required; pending actors attack/tick must independently pass and supply metadata to reach600.'])
(H/'a9/candidate-owner-audit.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps({k:v for k,v in result.items() if k not in ['owners','limits']},indent=2))
