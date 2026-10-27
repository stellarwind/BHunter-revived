"""Extract literal bootstrap data; original identities are nonexecutable lookup keys."""
from pathlib import Path
import re,json,struct,sys,argparse
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from record_pickup_basic import reference
_,raw=reference()
def string(va):return raw(va,512).split(b'\0')[0].decode('latin1')
def lit(s):return json.dumps(s,ensure_ascii=True)
def fl(n):return format(struct.unpack('<f',struct.pack('<I',n))[0],'.17g')
s=(ROOT/'META/m4/finish/game-bootstrap/original/004474c0.decomp.c.txt').read_text();menus=[(int(a,16),int(b,0)) for a,b in re.findall(r'FUN_0042c300\(&DAT_([0-9a-f]+),(0x[0-9a-f]+|\d+)\);',s)]
rows=[]
for match in re.finditer(r'FUN_0042c4c0\((.*?)\)\s*;',s,re.S):
 args=re.split(r',\s*',match.group(1));assert len(args)==7
 if any('pbVar13' in a for a in args):continue
 def addr(a):
  vs=re.findall(r'(?:DAT_|LAB_|FUN_|s_.*_)([0-9a-f]{8})',a)
  if vs:return int(vs[-1],16)
  a=re.sub(r'\([^)]*\)','',a).strip();return int(a,0) if a else 0
 rows.append([addr(a) for a in args])
sound={}
for owner in ('004474c0','00479200'):
 x=json.load(open(ROOT/'META/m4/finish'/('game-bootstrap' if owner=='004474c0' else 'startup-bootstrap')/'original'/(owner+'.disasm.json')))
 for i,r in enumerate(x):
  if r['mnemonic']=='CALL' and r['operands']==['0x0047b950']:
   pushes=[t['operands'][0] for t in x[:i] if t['mnemonic']=='PUSH'][-5:]
   bits=[0 if a=='EBX' else 0x40000000 if a=='ESI' else int(a,0) for a in pushes]
   den,stream,level,ran,name=bits
   # One file can be loaded as both a one-shot and a persistent voice. Keep
   # each call's destination identity rather than collapsing equal filenames.
   end=next((j for j in range(i+1,len(x)) if x[j]['mnemonic']=='CALL'),len(x))
   for store in x[i+1:end]:
    if store['mnemonic']=='MOV' and len(store['operands'])==2 and store['operands'][1]=='EAX' and re.fullmatch(r'\[0x[0-9a-f]+\]',store['operands'][0]):
     destination=int(store['operands'][0][1:-1],16)
     sound[(owner,destination)]=(ran,level,stream,den)
def ops(text,owner):
 out=[]
 for line in text.splitlines():
  m=re.search(r'_?DAT_([0-9a-f]{8}) = FUN_(004507c0|0047b950|00458140)\(.*_([0-9a-f]{8})\);',line)
  if m:
   dest,fun,name=m.groups();name=int(name,16);kind={'004507c0':1,'0047b950':2,'00458140':3}[fun];pars=(0,0,0,0)
   if kind==2:pars=sound[(owner,int(dest,16))]
   out.append('{%d,0x%sUL,%s,%s,%s,%d,%s}'%(kind,dest,lit(string(name)),fl(pars[0]),fl(pars[1]),pars[2],fl(pars[3])))
  m=re.match(r'\s*_?DAT_([0-9a-f]{8}) = _?DAT_([0-9a-f]{8});',line)
  if m:out.append('{4,0x%sUL,0,0,0,0,0x%sUL}'%m.groups())
  m=re.match(r'\s*_?DAT_([0-9a-f]{8}) = 0;',line)
  if m:out.append('{5,0x%sUL,0,0,0,0,0}'%m.group(1))
  m=re.search(r'FUN_0047b920\((\d+)\)',line)
  if m:out.append('{6,%s,0,0,0,0,0}'%m.group(1))
  if 'FUN_00470e00(DAT_004d047c,3.0,3.0,3.0)' in line:out.append('{9,0x004d047cUL,0,3,3,0,3}')
  m=re.search(r'FUN_0044f4f0\((\d+),(0x[0-9a-f]+|\d+)\)',line)
  if m:out.append('{7,%s,0,0,0,0,%s}'%m.groups())
 return out
out=['/* Literal data recovered from immutable EXE; identities are lookup keys only. */']
stages={'prelude':s[:s.index('FUN_0044e880')],'models':s[s.index('_DAT_004d1920'):s.index('DAT_004d1aa0')],'menu_resources':s[s.index('DAT_004d1a6c'):s.index('iVar6 = 0;',s.index('DAT_004d1a6c'))],'tail':s[s.index('FUN_0047b920(3);'):s.index('FUN_0044e880(s_textures_',s.index('FUN_0047b920(3);'))],'tv':s[s.index('DAT_004df1d8'):s.index('DAT_004d4420')]}
start=(ROOT/'META/m4/finish/startup-bootstrap/original/00479200.decomp.c.txt').read_text();stages['engine_sounds']=start[start.index('DAT_004f5514'):start.index('FUN_00476da0')];stages['engine_images']=start[start.index('DAT_004f004c'):start.index('FUN_004474c0')]
for name,text in stages.items():
 arr=ops(text,'00479200' if name.startswith('engine') else '004474c0');out+=['static const BHBootstrapResource '+name+'[]={',',\n'.join(arr),'};']
out+=['static const BHBootstrapMenuSpec menu_specs[]={',',\n'.join('{0x%08xUL,0x%08xUL}'%(a,b) for a,b in menus),'};']
backs={int(a,16):int(b,0) for a,b in re.findall(r'\*\(undefined4 \*\)\(DAT_([0-9a-f]+) \+ 0x1d4\) = (\d+);',s)}
out+=['static const long menu_backs[]={'+','.join(str(backs[a]) for a,b in menus)+'};','static const BHBootstrapRowSpec row_specs[]={']
for vals in rows:
 a,button,title,body,dest,action,refresh=vals;bodyexpr='0' if not body else 'BH_BOOT_PLAYER_NAME' if body==0x4d0458 else lit(string(body))
 out+=['{0x%08xUL,0x%08xUL,%s,%s,0x%08xUL,0x%08xUL,0x%08xUL},'%(a,button,lit(string(title)),bodyexpr,dest,action,refresh)]
out+=['};']
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
args.output.write_bytes(('\n'.join(out)+'\n').encode())
print('generated',len(menus),'menus',len(rows),'literal rows; actual five-argument sound calls',len(sound))
