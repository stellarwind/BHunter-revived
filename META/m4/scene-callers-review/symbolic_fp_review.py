"""Diagnostic symbolic x87 dataflow of reviewed straight-line paths only."""
import csv,re,sys,json
from pathlib import Path
folder=Path(__file__).resolve().parent.parent/'camera-modes/original'
name=sys.argv[1];start=sys.argv[2] if len(sys.argv)>2 else ''
if (folder/(name+'.tsv')).exists():rows=list(csv.DictReader((folder/(name+'.tsv')).open(),delimiter='\t'))
else:
 rows=json.loads((Path(__file__).resolve().parent/'original'/(name+'.disasm.json')).read_text(encoding='utf-8-sig'))
 for row in rows:row['operands']=', '.join(row['operands'])
regs={};mem={};stack=[];delta=0
args={4:'state',8:'target',12:'angles',16:'basisX',20:'basisY',24:'basisZ'}
if name=='004341a0':
 args={4:'t',8:'out',12:'p0',16:'p1',20:'p2',24:'p3'}
 if start:rows=[r for r in rows if r['address']>=start];delta=-24
def ref(op):
 match=re.search(r'\[(.*?)\]',op)
 if not match:return op
 val=match[1];m=re.fullmatch(r'(E[A-Z]+)(?: \+ (0x[0-9a-f]+))?',val)
 if m:
  reg,offset=m.groups();offset=int(offset or '0',16)
  if reg=='ESP':return 'stack'+str(delta+offset)
  return regs.get(reg,reg)+'+'+str(offset)
 return val
def value(op):
 if op.startswith('ST'):return stack[int(op[2:] or '0')]
 key=ref(op)
 return mem.get(key,key)
def write(op,val,where):
 key=ref(op);mem[key]='r'+where;print(where,key,'=',val)
def arg(op):
 k=ref(op)
 if k.startswith('stack'):return args.get(int(k[5:]),k)
 return k
def binary(op,a,b):return '('+a+op+b+')'
for row in rows:
 at=row['address'];mn=row['mnemonic'];ops=row['operands'].split(', ') if row['operands'] else []
 if mn=='PUSH':delta-=4
 elif mn=='SUB' and ops[0]=='ESP':delta-=int(ops[1],16)
 elif mn=='ADD' and ops[0]=='ESP':delta+=int(ops[1],16)
 elif mn=='MOV' and ops[0] in ('EAX','ECX','EDX','ESI','EDI','EBX'):
  regs[ops[0]]=arg(ops[1]) if '[' in ops[1] else regs.get(ops[1],ops[1])
 elif mn=='LEA':regs[ops[0]]=ref(ops[1])
 if start and at<start:continue
 if mn=='CALL':print('CALL',row['operands'],'STACK',stack);break
 if mn=='FLD':stack.insert(0,value(ops[0]))
 elif mn=='FXCH':
  j=int(ops[0][2:] or '0') if ops else 1;stack[0],stack[j]=stack[j],stack[0]
 elif mn in ('FST','FSTP'):
  dest=ops[0]
  if dest.startswith('ST'):stack[int(dest[2:] or '0')]=stack[0]
  else:
   width='f' if dest.startswith('float') else 'd';write(dest,width+'('+stack[0]+')',at)
  if mn=='FSTP':stack.pop(0)
 elif mn in ('FADD','FMUL','FSUB','FSUBR','FDIV','FDIVR'):
  v=value(ops[-1]);a=stack[0];b=v
  if mn.endswith('R'):a,b=b,a
  stack[0]=binary({'FADD':'+','FMUL':'*','FSUB':'-','FSUBR':'-','FDIV':'/','FDIVR':'/'}[mn],a,b)
 elif mn in ('FADDP','FMULP','FSUBP','FSUBRP','FDIVP'):
  j=int(ops[0][2:] or '0') if ops else 1;a=stack[j];b=stack[0]
  if mn=='FSUBRP':a,b=b,a
  stack[j]=binary({'FADDP':'+','FMULP':'*','FSUBP':'-','FSUBRP':'-','FDIVP':'/'}[mn],a,b);stack.pop(0)
 elif mn in ('FSQRT','FSIN','FCOS'):stack[0]=mn[1:].lower()+'('+stack[0]+')'
 elif mn=='FCHS':stack[0]='(-'+stack[0]+')'
 elif mn in ('FCOMP','FCOMPP'):
  stack.pop(0)
  if mn=='FCOMPP':stack.pop(0)
