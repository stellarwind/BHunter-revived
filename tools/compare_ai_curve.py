"""Static complete path-evaluator graph/constant/linked-code comparison. No execution."""
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from compare_ai_guided import instructions, digest
from compare_ai_center import image_bytes
sys.path.insert(0, str(ROOT / 'analysis/astra-review'))
from targeted_probes import parse_coff


def expression_graph(rows, constants, mode, alias):
    """Follow one of the actual two comparisons, preserving every memory spill."""
    registers = {'ESP': 0, 'ESI': 'saved.esi', 'EDI': 'saved.edi', 'EBX': 'saved.ebx', 'EBP': 'saved.ebp'}
    memory = {4: 't', 8: ('pointer', alias), 12: ('pointer', 'a'),
              16: ('pointer', 'b'), 20: ('pointer', 'c'), 24: ('pointer', 'd')}
    for name in ('a', 'b', 'c', 'd'):
        memory.update({(name, 4*i): name+'.'+'xyz'[i] for i in range(3)})
    stack, outputs, comparisons = [], [], []
    cursor = 0
    by_address = {r['address']: i for i, r in enumerate(rows)}
    decision = 0
    def operation(op, a, b):
        if op in ('add', 'mul') and repr(a) > repr(b):
            a, b = b, a
        return (op+'53', a, b)
    def address(operand):
        inner = re.search(r'\[(.*?)\]', operand)[1].strip()
        found = re.fullmatch(r'([A-Z]{3})(?:\s*\+\s*(0X[0-9A-F]+))?', inner)
        assert found, ('Unreviewed address', operand)
        base = registers[found[1]]
        offset = int(found[2] or '0', 16)
        return (base[1], offset) if isinstance(base, tuple) and base[0] == 'pointer' else base+offset
    def st_index(operand):
        found = re.fullmatch(r'ST\(?([0-7])?\)?', operand.strip())
        return int(found[1] or '0') if found else None
    def value(operand, row, floating=False):
        index = st_index(operand)
        if index is not None:
            return stack[index]
        if '[' in operand:
            if row['address'] in constants:
                return constants[row['address']]
            return memory[address(operand)]
        if operand in registers:
            return registers[operand]
        return int(operand, 16)
    def write(operand, item, row, floating=False):
        index = st_index(operand)
        if index is not None:
            stack[index] = item
            return
        if '[' not in operand:
            registers[operand] = item
            return
        at = address(operand)
        if floating and not ('QWORD' in operand or 'DOUBLE' in operand):
            if not (isinstance(item, tuple) and item[0] == 'f32'):
                item = ('f32', item)
        memory[at] = item
        if floating and isinstance(at, tuple) and at[0] == alias:
            outputs.append((at[1], item))
    while cursor < len(rows):
        row = rows[cursor]
        cursor += 1
        op, arg = row['mnemonic'], row['operands'].upper()
        parts = [p.strip() for p in arg.split(',')]
        if op in ('SUB', 'ADD'):
            assert parts[0] == 'ESP'
            registers['ESP'] += int(parts[1], 16) * (1 if op == 'ADD' else -1)
        elif op == 'MOV':
            write(parts[0], value(parts[1], row), row)
        elif op == 'PUSH':
            item = value(arg, row)
            registers['ESP'] -= 4
            memory[registers['ESP']] = item
        elif op == 'POP':
            registers[arg] = memory[registers['ESP']]
            registers['ESP'] += 4
        elif op == 'FLD':
            stack.insert(0, value(arg, row, True))
        elif op == 'FXCH':
            index = st_index(arg) if arg else 1
            stack[0], stack[index] = stack[index], stack[0]
        elif op in ('FST', 'FSTP'):
            write(arg, stack[0], row, True)
            if op == 'FSTP':
                stack.pop(0)
        elif op in ('FADD', 'FMUL', 'FSUB', 'FSUBR'):
            index = st_index(parts[0]) if len(parts) == 2 else 0
            left, right = stack[index], value(parts[-1], row, True)
            if op == 'FSUBR':
                left, right = right, left
            stack[index] = operation({'FADD':'add','FMUL':'mul','FSUB':'sub','FSUBR':'sub'}[op], left, right)
        elif op in ('FADDP', 'FMULP', 'FSUBP', 'FSUBRP'):
            index = st_index(parts[0]) if arg else 1
            left, right = stack[index], stack[0]
            if op == 'FSUBRP':
                left, right = right, left
            stack[index] = operation({'FADDP':'add','FMULP':'mul','FSUBP':'sub','FSUBRP':'sub'}[op], left, right)
            stack.pop(0)
        elif op in ('FCOM', 'FCOMP'):
            comparisons.append((stack[0], value(arg, row, True)))
            if op == 'FCOMP':
                stack.pop(0)
        elif op == 'FNSTSW':
            assert arg == 'AX'
        elif op == 'TEST':
            assert parts[0] == 'AH' and int(parts[1], 16) == (0x41 if decision == 0 else 1)
        elif op in ('JZ', 'JE', 'JNZ', 'JNE'):
            assert decision < 2
            assert op in (('JZ','JE') if decision == 0 else ('JNZ','JNE'))
            taken = mode != 0 if decision == 0 else mode == 2
            decision += 1
            if taken:
                cursor = by_address[int(arg, 16)]
        elif op == 'RET':
            assert registers['ESP'] == 0 and not stack
            assert [at for at, _ in outputs] == [0, 4, 8]
            assert comparisons == [('t', ('constant', 0.0))] + ([] if mode == 0 else [('t', ('constant', 1.0))])
            return outputs
        else:
            raise AssertionError(('Unreviewed instruction', row))
    raise AssertionError('Missing complete return')


def main():
    out = ROOT / 'META/m4/playable/ai-byte-match/curve-close'
    out.mkdir(exist_ok=True)
    obj_path = ROOT / 'build/game/bh_camera_curve.obj'
    obj = obj_path.read_bytes()
    symbols, sections = parse_coff(obj)
    owner = next(s for s in symbols.values() if s['name'] == '_bh_camera_curve')
    section = sections[owner['section']]
    source, listing = instructions(obj_path, owner['name'])
    end = max(r['address'] + len(r['data']) for r in source if r['mnemonic'] == 'RET')
    source = [r for r in source if r['address'] < end]
    assert b''.join(r['data'] for r in source) == section['data'][:end]
    assert set(section['data'][end:]) <= {0x90}
    reference = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(reference) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original, original_constants = [], {}
    for row in json.loads((ROOT / 'META/m4/playable/ai-motion/curve-original-asm.json').read_text()):
        va = int(row['address'], 16)
        if va >= 0x4343bf:
            continue
        data = bytes.fromhex(row['bytes'])
        assert data == image_bytes(reference, va, len(data))
        operands = ', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):
            operands = hex(int(operands, 16) - 0x4341a0)
        if '[0x00489' in operands:
            address = int(re.search(r'\[0x([0-9a-f]+)\]', operands)[1], 16)
            width = 8 if 'double' in operands else 4
            constant = struct.unpack('<d' if width == 8 else '<f', image_bytes(reference,address,width))[0]
            assert constant in (0.,1.,3.)
            original_constants[va-0x4341a0] = ('constant', constant)
        original.append(dict(address=va-0x4341a0, data=data, mnemonic=row['mnemonic'], operands=operands))
    assert b''.join(r['data'] for r in original) == image_bytes(reference,0x4341a0,543)
    exe_path = ROOT / 'build/game/BHunter-source.exe'
    exe = exe_path.read_bytes()
    map_text = (ROOT / 'build/game/BHunter-source.map').read_text()
    found = re.findall(r'^\s+\w+:\w+\s+_bh_camera_curve\s+([0-9a-fA-F]{8})\s',map_text,re.M)
    assert len(found) == 1
    linked_va = int(found[0],16)
    linked = bytearray(image_bytes(exe,linked_va,end))
    constants, bindings = {}, []
    for relocation in section['relocations']:
        offset, symbol = relocation['offset'], relocation['symbol']
        assert relocation['kind'] == 6 and symbol['section'] > 0
        opcode, modrm = section['data'][offset-2:offset]
        assert opcode in (0xd8,0xd9,0xdc,0xdd) and modrm & 0xc7 == 5
        width = 8 if opcode in (0xdc,0xdd) else 4
        addend = struct.unpack_from('<I', section['data'],offset)[0]
        data = sections[symbol['section']]['data'][symbol['value']+addend:symbol['value']+addend+width]
        number = struct.unpack('<d' if width == 8 else '<f',data)[0]
        assert number in (0.,1.,3.)
        target = struct.unpack_from('<I',linked,offset)[0]
        assert image_bytes(exe,target,width) == data
        constants[offset-2] = ('constant',number)
        bindings.append(dict(offset=offset,storage_bytes=width,constant_hex=data.hex(),value=number))
        linked[offset:offset+4] = section['data'][offset:offset+4]
    assert linked == section['data'][:end]
    proofs = []
    for mode in range(3):
        for alias in ('out','a','b','c','d'):
            old = expression_graph(original, original_constants, mode, alias)
            new = expression_graph(source, constants, mode, alias)
            assert old == new, ('Operation/rounding/publication graph differs',mode,alias,old,new)
            proofs.append(dict(branch=mode,output=alias,outputs=old))
    result = dict(original_va='004341a0',original_body_bytes=543,source_body_bytes=end,
                  byte_match=False,classification='func/close under declared PC53 profile',
                  linked_matches_compared_object=True,calls=0,comparison_and_output_graphs=proofs,
                  verified_linked_constants=bindings,object_sha256=digest(obj),executable_sha256=digest(exe),
                  differences='Register/stack allocation, scheduling, explicit integer bit copies for float spills, and identity PC53 double stores; all three complete branch output graphs agree.',
                  limits=['Masked PC53 nearest value graphs; NaN payload and exception flags not proved.',
                          'Whole-input output aliasing proved; partial vector overlap unsupported.',
                          'Extra binary64 spills rely on PC53 and float-input range contract; runtime transitions remain unproved.'])
    (out/'linked-close.json').write_text(json.dumps(result,indent=2)+'\n')
    (out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('comparison_and_output_graphs','verified_linked_constants')}))


if __name__ == '__main__':
    main()
