"""Read-only symbolic x87 trace of the first player aiming quad."""
import json
import re
import sys

src = 'META/m4/hud-attachment/next-player-review/0041c860.disasm.json'
ins = json.load(open(src))
mem = {}
fp = [f'F{i}' for i in range(8)] if len(sys.argv)>3 else []
esp = 0
start = int(sys.argv[1],16) if len(sys.argv)>1 else 0x41ca68
stop = int(sys.argv[2],16) if len(sys.argv)>2 else 0x41cc45

def loc(op):
    global esp
    if '[ESI +' in op:
        n = int(re.search(r'\[ESI \+ 0x([0-9a-f]+)\]', op).group(1), 16)
        return f'p{n:x}'
    if '[ESP' in op:
        m = re.search(r'\[ESP(?: \+ 0x([0-9a-f]+))?\]', op)
        off = int(m.group(1), 16) if m.group(1) else 0
        return f's{esp+off:+x}'
    if '[0x' in op:
        return 'c' + re.search(r'\[0x([0-9a-f]+)\]', op).group(1)
    return op

def value(op):
    name = loc(op)
    if name.startswith('ST') and name[2:].isdigit():
        return fp[int(name[2:])]
    return mem.get(name, name)

def bop(kind, a, b):
    return f'({a}{kind}{b})'

for x in ins:
    a = int(x['address'], 16)
    if a < start or a > stop:
        continue
    m, ops = x['mnemonic'], x['operands']
    op = ops[0] if ops else ''
    if m == 'PUSH':
        esp -= 4
        mem[f's{esp:+x}'] = op
    elif m == 'POP':
        esp += 4
    elif m == 'ADD' and op == 'ESP':
        esp += int(ops[1], 16)
    elif m == 'SUB' and op == 'ESP':
        esp -= int(ops[1], 16)
    elif m == 'FLD':
        fp.insert(0, value(op))
    elif m == 'FSTP':
        if op.startswith('ST'):
            idx = int(op[2:])
            fp[idx] = fp[0]
        else:
            mem[loc(op)] = fp[0]
            if op.startswith('float ptr [ESP'):
                print(x['address'], 'STORE', loc(op), '=', fp[0])
        fp.pop(0)
    elif m == 'FST':
        mem[loc(op)] = fp[0]
    elif m == 'FXCH':
        idx = int(op[2:]) if op.startswith('ST') else 1
        fp[0], fp[idx] = fp[idx], fp[0]
    elif m == 'FMUL':
        fp[0] = bop('*', fp[0], value(op))
    elif m == 'FMULP':
        idx = int(op[2:]) if op.startswith('ST') else 1
        fp[idx] = bop('*', fp[idx], fp[0])
        fp.pop(0)
    elif m == 'FADD':
        fp[0] = bop('+', fp[0], value(op))
    elif m == 'FADDP':
        idx = int(op[2:]) if op.startswith('ST') else 1
        fp[idx] = bop('+', fp[idx], fp[0])
        fp.pop(0)
    elif m == 'FSUB':
        fp[0] = bop('-', fp[0], value(op))
    elif m == 'FSUBR':
        fp[0] = bop('-', value(op), fp[0])
    elif m == 'FSUBP':
        idx = int(op[2:]) if op.startswith('ST') else 1
        fp[idx] = bop('-', fp[idx], fp[0])
        fp.pop(0)
    elif m == 'FCHS':
        fp[0] = f'(-{fp[0]})'
    if m == 'CALL':
        print('CALL', x['address'], op, 'ESP', esp, 'FP', len(fp))
