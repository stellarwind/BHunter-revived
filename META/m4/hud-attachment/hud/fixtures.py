"""Generic PC53 stack-operation evaluator creates fixed HUD precision fixtures.

This interprets reviewed instruction records as data, without mapping or
executing the reference EXE. It is a static contract check, not differential
original execution. Python binary64 operations implement the current PC53
profile; explicit FST(P) memory stores round binary32.
"""
from pathlib import Path
import json, math, random, re, struct

ROOT=Path(__file__).resolve().parents[4]
HERE=Path(__file__).resolve().parent
rows=json.loads((ROOT/'META/m4/radar/original/0041b2d0.disasm.json').read_text())
constants=json.loads((HERE/'raw-review.json').read_text())['constants']
def f32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def word(x): return struct.unpack('<I',struct.pack('<f',x))[0]
outputs=[0x4df1b8,0x4df1bc,0x4df1c0,0x4df218,0x4df21c,0x4df220,
         0x4df228,0x4df22c,0x4df230,0x4df1a8,0x4df1ac,0x4df1b0,
         0x4d18f8,0x4d18fc,0x4d1900,0x4d18e8,0x4d18ec,0x4d18f0,
         0x4df1c8,0x4df1cc,0x4df1d0]
def evaluate(inputs):
    memory={int(c['address'],16):bytes.fromhex(c['raw']) for c in constants}
    memory.update({0x10000+12+i*4:struct.pack('<f',x) for i,x in enumerate(inputs)})
    registers={'ESI':0x10000,'EDI':0x1000c}; stack=[]
    def address(arg):
        inner=arg[arg.index('[')+1:arg.index(']')]
        terms=[t.strip() for t in inner.split('+')]
        return sum(registers[t] if t in registers else int(t,0) for t in terms)
    def value(arg):
        at=address(arg);return struct.unpack('<d' if arg.startswith('double') else '<f',memory[at])[0]
    for row in rows:
        va=int(row['address'],16)
        if va<0x41b2d6 or va>=0x41b537:continue
        op=row['mnemonic'];args=row['operands']
        if op=='FLD': stack.insert(0,value(args[0]))
        elif op=='FADD': stack[0]+=value(args[0])
        elif op=='FMUL': stack[0]*=value(args[0])
        elif op=='FST' or op=='FSTP':
            memory[address(args[0])]=struct.pack('<f',stack[0])
            if op=='FSTP':stack.pop(0)
        elif op=='FXCH':
            i=int(args[0][2:]) if args else 1;stack[0],stack[i]=stack[i],stack[0]
        elif op=='FSUBP':
            i=int(args[0][2:]) if args else 1;stack[i]-=stack[0];stack.pop(0)
        elif op=='FCHS':stack[0]=-stack[0]
        elif op=='FCOS':stack[0]=math.cos(stack[0])
        elif op=='FSIN':stack[0]=math.sin(stack[0])
        elif op=='LEA':registers[args[0]]=address(args[1])
        elif op=='MOV':
            dst,src=args
            if dst in ('EAX','ECX','EDX'):
                registers[dst]=struct.unpack('<I',memory[address(src)])[0]
            else:
                memory[address(dst)]=struct.pack('<I',registers[src] if src in registers else int(src,0))
        else:raise ValueError((row,stack))
    assert not stack
    return [struct.unpack('<I',memory[va])[0] for va in outputs]

rng=random.Random(0x41b2d0)
samples=[[0,0,0,1,0,0,0,1,0,0,0,1,0],
         [12.5,-3.75,40,0,0,1,1,0,0,0,1,0,1.5707963267948966],
         [1000000,-1000000,1000000,1,-1,.125,.75,-.875,.625,.5,-.25,.375,-1.5707963267948966]]
for i in range(29):
    samples.append([rng.uniform(-100,100) for _ in range(3)]+[rng.uniform(-1,1) for _ in range(9)]+[rng.uniform(-3.2,3.2)])
samples=[[f32(x) for x in sample] for sample in samples]
fixtures=[dict(input=[word(x) for x in sample],expected=evaluate(sample)) for sample in samples]
(HERE/'fixtures.json').write_text(json.dumps(dict(profile='PC53 nearest; instruction records evaluated as data',original_code_executed=False,output_addresses=[f'{a:08x}' for a in outputs],fixtures=fixtures),indent=2)+'\n')
lines=['/* Generated static instruction-contract fixtures; see hud/fixtures.py. */',
       'static const struct { unsigned long input[13],expected[21]; } hud_fixtures[]={']
for fixture in fixtures:
    lines.append('    {{'+','.join(f'0x{x:08x}UL' for x in fixture['input'])+'},{'+','.join(f'0x{x:08x}UL' for x in fixture['expected'])+'}},')
lines.append('};')
(ROOT/'tests/hud_fixtures.h').write_text('\n'.join(lines)+'\n')
print(f'{len(fixtures)} independent static HUD instruction fixtures')
