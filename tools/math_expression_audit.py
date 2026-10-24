"""Read-only symbolic grouping audit of straight-line x87 math listings.

This is not an instruction matcher or runtime loader. It prints expression
trees for the reviewed arithmetic subset and fails on unknown FP operations.
"""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]


def expressions(address,registers):
    stack=[];stores={};rows=[]
    def operand(text):
        if text.startswith('ST'):return stack[int(text[2:] or '0')]
        match=re.search(r'\[([A-Z]+)(?: \+ (0x[0-9a-f]+))?\]',text)
        if not match:raise ValueError(text)
        reg,offset=match.groups();offset=int(offset or '0',16)
        if reg=='ESP':name={4:'x',8:'y',12:'z'}[offset]
        else:
            root=registers[reg];name=f'{root}[{offset//16}][{offset%16//4}]'
        return stores.get(name,name)
    def binary(a,operator,b):return f'({a} {operator} {b})'
    for line in (ROOT/f'META/m2/math/original/{address}.asm.txt').read_text().splitlines():
        if line.startswith('#'):continue
        at,op,*rest=line.split(maxsplit=2);arg=rest[0] if rest else ''
        if op=='FLD':stack.insert(0,operand(arg))
        elif op=='FXCH':
            n=int(arg[2:] or '1') if arg else 1;stack[0],stack[n]=stack[n],stack[0]
        elif op in ('FMUL','FADD','FSUB','FSUBR'):
            other=operand(arg);operator={'FMUL':'*','FADD':'+','FSUB':'-','FSUBR':'-'}[op]
            stack[0]=binary(other,operator,stack[0]) if op=='FSUBR' else binary(stack[0],operator,other)
        elif op in ('FMULP','FADDP','FSUBP'):
            n=int(arg.split(',')[0][2:] or '1') if arg else 1
            stack[n]=binary(stack[n],{'FMULP':'*','FADDP':'+','FSUBP':'-'}[op],stack[0]);stack.pop(0)
        elif op=='FCHS':stack[0]='(-'+stack[0]+')'
        elif op=='FSTP':
            if arg.startswith('ST'):
                n=int(arg[2:] or '0');stack[n]=stack[0];stack.pop(0)
            else:
                match=re.search(r'\[([A-Z]+)(?: \+ (0x[0-9a-f]+))?\]',arg)
                reg,offset=match.groups();offset=int(offset or '0',16)
                name=f'{registers[reg]}[{offset//16}][{offset%16//4}]'
                expression=stack.pop(0);rows.append(name+' = '+expression)
                stores[name]=expression
        elif op.startswith('F'):raise ValueError((at,op,arg))
    assert not stack,(address,stack)
    return rows


if __name__=='__main__':
    for address,registers in [('00401000',dict(EAX='a',ECX='b',EDX='out')),
                              ('00401490',dict(EAX='m'))]:
        print(address)
        print('\n'.join(expressions(address,registers)))
