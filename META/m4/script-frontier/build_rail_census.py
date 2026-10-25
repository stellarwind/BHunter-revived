import pathlib,struct,json,collections,hashlib
root=pathlib.Path('.');data=(root/'GAME_DATA/Bhunter.bin').read_bytes();n=struct.unpack_from('<I',data)[0]
for i in range(n):
 at=4+i*256;name=data[at:at+248].split(b'\0')[0].decode('latin1')
 if name.upper()=='TEST.LEV':
  start,end=struct.unpack_from('<II',data,at+248);lev=data[start:end];break
count=struct.unpack_from('<I',lev)[0];names=collections.Counter();modes=collections.Counter();yaw=collections.Counter()
for i in range(count):
 name=lev[4+i*32:20+i*32].split(b'\0')[0].decode('latin1')
 if name.upper().startswith('RC_'):
  names[name]+=1
  if len(name)>3 and name[3] in '0123456789':modes[name[3]]+=1;yaw[str(struct.unpack_from('<I',lev,4+i*32+28)[0]&3)]+=1
report=dict(scope='Independent raw archive directory and raw LEV placement census only; no source implementation or original code executed',level_sha256=hashlib.sha256(lev).hexdigest(),placements=count,prefix_count=sum(names.values()),recognized=sum(modes.values()),segments=sum(v*(3 if k in '789' else 1) for k,v in modes.items()),modes=dict(sorted(modes.items())),yaw=dict(sorted(yaw.items())),names=dict(sorted(names.items())))
(root/'META/m4/script-frontier/rail-level-census.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
