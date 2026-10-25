from pathlib import Path
import subprocess,json,hashlib
root=Path('D:/Decomp/Bhunter');out=root/'META/m4/collision-cache';original=out/'original';original.mkdir(exist_ok=True)
base=['ghidra','--projects-dir',str(root/'ghidra-projects'),'--project','Bhunter','--program','BHUNTER.EXE']
for address in ('0044b130',):
 for op,args in [('function-get',['function','get']),('decompile',['decompile']),('xrefs',['function','xrefs']),('disasm',['disasm'])]:
  argv=base+args+['0x'+address]
  if op=='disasm':argv+=['-n','600']
  p=subprocess.run(argv,capture_output=True,text=True,check=True);value=json.loads(p.stdout)
  if op=='function-get':size=value[0]['size']
  if op=='disasm':value=[r for r in value if int(address,16)<=int(r['address'],16)<int(address,16)+size]
  (original/(address+'.'+op+'.json')).write_text(json.dumps(value,indent=2)+'\n',encoding='utf-8')
print('Exported collision initialization and horizon blocker reviews')
