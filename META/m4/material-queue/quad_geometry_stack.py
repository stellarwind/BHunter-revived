"""Symbolic retained/spilled x87 geometry review; no original execution."""
import json,re
from pathlib import Path
here=Path(__file__).resolve().parent
rows=json.loads((here/'original/00457590.disasm.json').read_text())
memory={0x40:'ox',0x44:'oy',0x48:'oz',0x30:'rx',0x34:'ry',0x38:'rz',0x14:'tx',0x18:'ty',0x1c:'tz'}
stack=[];esp=0;stores=[]
for row in rows:
 address=int(row['address'],16)
 if not 0x457a63<=address<0x457ba5:continue
 op=row['mnemonic'];args=row['operands']
 if op=='PUSH':esp-=4;continue
 if op=='FXCH':
  i=int(args[0][2:]) if args else 1
  stack[0],stack[i]=stack[i],stack[0];continue
 if op not in ('FLD','FADD','FSUB','FST','FSTP'):continue
 operand=args[0]
 if operand.startswith('ST'):value=stack[int(operand[2:])]
 else:
  offset=int(re.search(r'0x([0-9a-f]+)',operand)[1],16)+esp
  if op.startswith('FST'):
   memory[offset]='f32('+stack[0]+')';stores.append((row['address'],hex(offset),stack[0]))
   if op=='FSTP':stack.pop(0)
   continue
  value=memory[offset]
 if op=='FLD':stack.insert(0,value)
 else:stack[0]='('+stack[0]+('+' if op=='FADD' else '-')+value+')'
for row in stores:print(*row)
