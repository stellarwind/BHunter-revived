"""Refresh static-review provenance without editing frozen implementation files."""
import hashlib,json,re
from pathlib import Path
root=Path(__file__).resolve().parents[3];here=Path(__file__).resolve().parent
variants=json.loads((root/'META/m4/frontier-90min/line-ending-variants.json').read_text())['variants']
names=json.loads((here/'key-names.json').read_text())
header=(root/'src/bh_input_key_names.h').read_bytes()
literals=[]
for line in header.decode('ascii').splitlines():
 if '/* VK' not in line:continue
 item=line.strip().split(', /*')[0]
 literals.append(None if item=='NULL' else bytes(int(v,8) for v in re.findall(r'\\([0-7]{3})',item)))
assert len(literals)==255
for literal,entry in zip(literals,names['entries']):assert literal==(bytes.fromhex(entry['bytes']) if entry['bytes'] is not None else None)
relevant=['src/bh_input_messages.c','src/bh_input_messages.h','src/bh_input_collection.c','src/bh_input_collection.h','src/bh_input_devices.c','src/bh_input_devices.h','src/bh_input_key_names.h','tests/input_collection_cases.h','tests/sprite_quad_cases.h']
audit=[]
for p in relevant:
 b=(root/p).read_bytes();item=dict(source=p,sha256=hashlib.sha256(b).hexdigest(),crlf_count=b.count(b'\r\n'))
 variant=next((v for v in variants if v['source']==p),None)
 if variant:
  prior=(root/variant['prior_snapshot']).read_bytes()
  assert hashlib.sha256(prior).hexdigest()==variant['prior_sha256']
  assert hashlib.sha256(b).hexdigest()==variant['current_sha256']
  assert prior.replace(b'\r\n',b'\n')==b
  item.update(prior_snapshot=variant['prior_snapshot'],prior_sha256=variant['prior_sha256'],change='CRLF to LF only; token/static data unchanged')
 audit.append(item)
docs=['SOURCE_REVIEW.md','DEVICE_SOURCE_REVIEW.md','PRE_REVIEW.md','DEVICE_PRE_REVIEW.md']
out=dict(scope='Current frozen source byte hashes and static-data verification only; execution evidence stays with named build variants',sources=audit,review_documents={p:hashlib.sha256((here/p).read_bytes()).hexdigest() for p in docs},key_names=dict(entries_verified=255,manifest_sha256=hashlib.sha256((here/'key-names.json').read_bytes()).hexdigest(),header_sha256=hashlib.sha256(header).hexdigest()),original_code_executed=False)
(here/'current-byte-audit.json').write_text(json.dumps(out,indent=2)+'\n',newline='\n')
material=root/'META/m4/material-queue';prior=json.loads((material/'source-hashes-prior-candidate.json').read_text())
current={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in prior}
(material/'source-hashes.json').write_text(json.dumps(current,indent=2)+'\n',newline='\n')
print('Verified current review sources; key defaults255 entries identical; material current hashes refreshed')
