import csv,json,hashlib,struct,pathlib
root=pathlib.Path('.');here=root/'META/m4/loading-display/next-review'
b=(root/'GAME_DATA/BHUNTER.EXE').read_bytes();assert hashlib.sha256(b).hexdigest()=='c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
p=struct.unpack_from('<I',b,60)[0];t=p+24+struct.unpack_from('<H',b,p+20)[0]
sec=[struct.unpack_from('<IIII',b,t+i*40+8) for i in range(struct.unpack_from('<H',b,p+6)[0])]
def raw(va,n):
 r=va-0x400000;s=next(s for s in sec if s[1]<=r and r+n<=s[1]+s[2]);return b[s[3]+r-s[1]:s[3]+r-s[1]+n]
m={r['address']:r for r in csv.DictReader((root/'analysis/function-map.tsv').open(),delimiter='\t')};ledger=json.loads((root/'META/function-status.json').read_text())['records']
report=[]
for a in ['004100d0','00412190','004100b0','00479200']:
 rows=json.loads((here/(a+'.disasm.json')).read_text());at=int(a,16);data=b''
 for row in rows:
  assert int(row['address'],16)==at
  q=bytes.fromhex(row['bytes']);data+=q;at+=len(q)
 assert len(data)==int(m[a]['body_bytes']) and len(rows)==int(m[a]['instructions']) and rows[-1]['mnemonic']=='RET' and data==raw(int(a,16),len(data))
 calls=[]
 for call in m[a]['calls'].split('|'):
  ca=call.split(':')[0];rs=[r for r in ledger if r.get('original_va')==ca]
  calls.append(dict(address=ca,name=call.split(':')[-1],variants=[{k:r.get(k) for k in ['id','implementation','game_admitted','source']} for r in rs]))
 report.append(dict(address=a,bytes=len(data),instructions=len(rows),end=f'{at-1:08x}',sha256=hashlib.sha256(data).hexdigest(),direct_calls=calls))
(here/'world-input-review.json').write_text(json.dumps(dict(original_exe_sha256=hashlib.sha256(b).hexdigest(),owners=report,original_code_executed=False,source_or_ledger_changed=False),indent=2)+'\n')
(here/'004100b0.decompile.json').write_text(json.dumps(dict(error='No function at address0x004100b0 in current Ghidra database; no analysis boundary was created during this read-only review',complete_raw='004100b0.disasm.json',bytes=25,instructions=7),indent=2)+'\n')
print(json.dumps(report,indent=2))
