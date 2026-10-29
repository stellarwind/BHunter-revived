"""Static whole-owner guided-selector control, x87 graph and linked-operand proof.

No executable is launched. The x87 graph comparison is scoped to masked PC53;
commutative sums/products compare by value, not NaN payload or exception flags.
"""
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/astra-review'))
from targeted_probes import parse_coff
sys.path.insert(0, str(ROOT / 'tools'))
from compare_ai_center import image_bytes


def digest(data):
    return hashlib.sha256(data).hexdigest()


def instructions(path, symbol):
    text = subprocess.check_output(['llvm-objdump', '-d', '--disassemble-symbols=' + symbol,
                                   '--x86-asm-syntax=intel', str(path)], text=True)
    rows = []
    for line in text.splitlines():
        match = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s+(\w+)\s*(.*)', line)
        if match:
            address, data, mnemonic, operands = match.groups()
            rows.append(dict(address=int(address, 16), data=bytes.fromhex(data),
                             mnemonic=mnemonic.upper(), operands=operands.split(' <')[0].strip()))
    return rows, text


def branches(rows):
    aliases = {'JZ': 'JE', 'JNZ': 'JNE'}
    edges = []
    for row in rows:
        if row['mnemonic'].startswith('J'):
            edges.append((row['address'], aliases.get(row['mnemonic'], row['mnemonic']),
                          int(row['operands'], 16)))
    # Follow each edge to its next decision. This permits the source's pure
    # portal-base LEA to move before the initial count test; it changes no edge.
    def decision(address):
        return next((i for i, edge in enumerate(edges) if edge[0] >= address), len(edges))
    return [(i, op, decision(target)) for i, (_, op, target) in enumerate(edges)]


def guard_templates(rows):
    result = []
    for i, row in enumerate(rows):
        if row['mnemonic'].startswith('J') and row['mnemonic'] != 'JMP':
            previous = rows[i-1]
            operands = re.sub(r'\s+', '', previous['operands'].upper())
            if not result:
                assert previous['mnemonic'] == 'TEST' and operands in ('EAX,EAX', 'ECX,ECX')
                operands = 'COUNT,COUNT'
            result.append((previous['mnemonic'], operands))
    return result


def graph(rows, initial, up_relocations):
    stack, memory, registers, stores = [], dict(initial), {}, []
    esp = 0
    def rounded(width, value):
        if width == 64:  # All operations already have PC53 precision; range bounded by float inputs.
            return value
        return value if isinstance(value, tuple) and value[0] == 'f32' else ('f32', value)
    def operation(op, a, b):
        if op in ('add', 'mul') and repr(a) > repr(b):
            a, b = b, a
        return (op + '53', a, b)
    def location(operand, row):
        found = re.search(r'\[ESP(?:\s*\+\s*(0X[0-9A-F]+))?\]', operand)
        if found:
            return esp + int(found[1] or '0', 16)
        if row['address'] in up_relocations:
            return 'up' + str(up_relocations[row['address']])
        raise AssertionError(('Unknown memory input', row))
    def value(operand, row):
        found = re.fullmatch(r'ST\(?([0-7])?\)?', operand.strip())
        if found:
            return stack[int(found[1] or '0')]
        return memory[location(operand, row)]
    def store(operand, row, item):
        found = re.fullmatch(r'ST\(?([0-7])?\)?', operand.strip())
        if found:
            stack[int(found[1] or '0')] = item
        else:
            address = location(operand, row)
            width = 64 if 'QWORD' in operand or 'DOUBLE' in operand else 32
            memory[address] = rounded(width, item)
            stores.append((width, memory[address]))
    for row in rows:
        op, arg = row['mnemonic'], row['operands'].upper()
        parts = [p.strip() for p in arg.split(',')]
        if op == 'ADD':
            assert parts[0] == 'ESP'
            esp += int(parts[1], 16)
        elif op == 'MOV':
            if '[' in parts[1]:
                registers[parts[0]] = memory[location(parts[1], row)]
            else:
                memory[location(parts[0], row)] = registers[parts[1]]
        elif op == 'FLD':
            stack.insert(0, value(arg, row))
        elif op == 'FXCH':
            index = int(re.search(r'[0-7]', arg)[0]) if re.search(r'[0-7]', arg) else 1
            stack[0], stack[index] = stack[index], stack[0]
        elif op in ('FST', 'FSTP'):
            store(arg, row, stack[0])
            if op == 'FSTP':
                stack.pop(0)
        elif op in ('FADD', 'FMUL', 'FSUB'):
            stack[0] = operation({'FADD': 'add', 'FMUL': 'mul', 'FSUB': 'sub'}[op], stack[0], value(arg, row))
        elif op in ('FADDP', 'FMULP'):
            index = int(re.search(r'[0-7]', parts[0])[0]) if re.search(r'[0-7]', parts[0]) else 1
            stack[index] = operation('add' if op == 'FADDP' else 'mul', stack[index], stack[0])
            stack.pop(0)
        elif op == 'FCOM':
            assert len(stack) == 1, ('Unexpected live x87 tail', stack)
            return dict(score=stack[0], best=value(arg, row), stores=stores)
        else:
            raise AssertionError(('Unreviewed arithmetic instruction', row))
    raise AssertionError('Missing original comparison')


def main():
    out = ROOT / 'META/m4/playable/ai-byte-match/guided-close'
    out.mkdir(exist_ok=True)
    obj_path = ROOT / 'build/game/bh_route_guided.obj'
    obj = obj_path.read_bytes()
    symbols, sections = parse_coff(obj)
    owner = next(s for s in symbols.values() if s['name'] == '_bh_actor_guided_exit_core')
    section = sections[owner['section']]
    source_rows, listing = instructions(obj_path, owner['name'])
    ret = next(i for i, row in enumerate(source_rows) if row['mnemonic'] == 'RET')
    source_rows = source_rows[:ret + 1]
    source_size = source_rows[-1]['address'] + 1
    assert b''.join(r['data'] for r in source_rows) == section['data'][:source_size]
    assert set(section['data'][source_size:]) <= {0x90}
    exe_path = ROOT / 'build/game/BHunter-source.exe'
    exe = exe_path.read_bytes()
    map_text = (ROOT / 'build/game/BHunter-source.map').read_text()
    def address(name):
        found = re.findall(r'^\s+\w+:\w+\s+' + re.escape(name) + r'\s+([0-9a-fA-F]{8})\s', map_text, re.M)
        assert len(found) == 1, name
        return int(found[0], 16)
    linked_va = address(owner['name'])
    linked = bytearray(image_bytes(exe, linked_va, source_size))
    reference = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(reference) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original = []
    for row in json.loads((ROOT / 'META/m4/playable/ai-byte-match/00434650-asm.json').read_text()):
        va = int(row['address'], 16)
        if va >= 0x434808:
            continue
        data = bytes.fromhex(row['bytes'])
        assert data == image_bytes(reference, va, len(data))
        operands = ', '.join(row['operands'])
        if row['mnemonic'].startswith('J'):
            operands = hex(int(operands, 16) - 0x434650)
        original.append(dict(address=va - 0x434650, data=data, mnemonic=row['mnemonic'], operands=operands))
    assert b''.join(r['data'] for r in original) == image_bytes(reference, 0x434650, 440)
    bindings, source_up, calls = [], {}, []
    call_targets = {'_bh_actor_portal_center_core': 0x433f70,
                    '_bh_actor_portal_normal_core': 0x434040, '_bh_vec3_cross': 0x4011a0}
    for relocation in section['relocations']:
        offset, symbol = relocation['offset'], relocation['symbol']
        target = struct.unpack_from('<I', linked, offset)[0]
        addend = struct.unpack_from('<I', section['data'], offset)[0]
        if relocation['kind'] == 20:
            assert symbol['name'] in call_targets and addend == 0
            target = linked_va + offset + 4 + struct.unpack_from('<i', linked, offset)[0]
            assert target == address(symbol['name'])
            calls.append(call_targets[symbol['name']])
            bindings.append(dict(offset=offset, kind='real source callee', symbol=symbol['name'], linked_va=hex(target)))
        elif symbol['name'].startswith('_bh_actor_guided_up$'):
            data = sections[symbol['section']]['data'][symbol['value']:symbol['value'] + 12]
            assert data == image_bytes(reference, 0x48c070, 12)
            assert addend in (0, 4, 8)
            assert image_bytes(exe, target - addend, 12) == data
            if section['data'][offset-1] != 0x68:
                assert section['data'][offset-2:offset] == bytes.fromhex('d80d')
                source_up[offset-2] = addend // 4
            bindings.append(dict(offset=offset, kind='verified complete up vector', addend=addend))
        else:
            assert symbol['name'] == '_bh_actor_exit_counts' and addend == 0
            assert target == address(symbol['name'])
            table = image_bytes(exe, target, 7 * 0x174)
            assert [struct.unpack_from('<i', table, i*0x174+0x18)[0] for i in range(7)] == list(range(7))
            bindings.append(dict(offset=offset, kind='existing seven-record count adapter', original_table='0x497230'))
        linked[offset:offset+4] = section['data'][offset:offset+4]
    assert linked == section['data'][:source_size]
    original_calls = [0x434650 + r['address'] + 5 + struct.unpack_from('<i', r['data'], 1)[0]
                      for r in original if r['mnemonic'] == 'CALL']
    assert calls == original_calls == [0x433f70, 0x434040, 0x4011a0, 0x433f70]
    assert branches(source_rows) == branches(original), (branches(source_rows), branches(original))
    assert guard_templates(source_rows) == guard_templates(original), 'Decision guard templates differ'
    wrapper = next(s for s in symbols.values() if s['name'] == '_bh_route_guided_exit')
    wrapper_section = sections[wrapper['section']]
    wrapper_va = address(wrapper['name'])
    wrapper_linked = bytearray(image_bytes(exe, wrapper_va, len(wrapper_section['data'])))
    assert len(wrapper_section['relocations']) == 1
    r = wrapper_section['relocations'][0]
    assert r['kind'] == 20 and r['symbol']['name'] == owner['name']
    assert wrapper_va + r['offset'] + 4 + struct.unpack_from('<i', wrapper_linked, r['offset'])[0] == linked_va
    wrapper_linked[r['offset']:r['offset']+4] = wrapper_section['data'][r['offset']:r['offset']+4]
    assert wrapper_linked == wrapper_section['data'], 'Actual source wrapper differs from object'
    def arithmetic(rows):
        start = max(i for i, row in enumerate(rows) if row['mnemonic'] == 'CALL') + 1
        return rows[start:]
    original_up = {r['address']: (int(re.search(r'0x([0-9a-fA-F]+)', r['operands'])[1], 16)-0x48c070)//4
                   for r in original if r['mnemonic'] == 'FMUL' and '0x0048c0' in r['operands']}
    common = {'up0': 'up.x', 'up1': 'up.y', 'up2': 'up.z'}
    def initial(point, anchor, side, best):
        result = dict(common)
        for prefix, at in [('point', point), ('anchor', anchor), ('side', side)]:
            result.update({at+4*i: prefix+'.'+'xyz'[i] for i in range(3)})
        result[best] = 'best'
        return result
    old = graph(arithmetic(original), initial(0x24, 0x30, 0x3c, 0x1c), original_up)
    new = graph(arithmetic(source_rows), initial(0x38, 0x44, 0x50, 0x1c), source_up)
    assert old['score'] == new['score'], 'PC53 score operation/rounding graph differs'
    assert old['best'] == new['best'] == 'best'
    result = dict(original_va='00434650', original_body_bytes=440, source_body_bytes=source_size,
                  byte_match=False, classification='func/close under declared PC53 profile',
                  linked_matches_compared_object=True, all_calls_and_data_verified=bindings,
                  complete_decision_edges=branches(source_rows), decision_guard_templates=guard_templates(source_rows),
                  actual_wrapper_calls_verified_core=True, score_graph=old['score'],
                  original_float_stores=old['stores'], source_float_stores=new['stores'],
                  differences='Register/stack allocation, instruction scheduling, hoisted portal-base LEA, integer bit copies enforcing float stores, and a redundant PC53 double store; no new score operation or decision edge.',
                  limits=['PC53 masked nearest value/decision graph only; NaN payload and exception flags not proved.',
                          'Extra binary64 store is an identity only within the declared PC53 and float-input range contract.',
                          'Count/model/neighbor images are explicit source adapters, not original registry definitions.'],
                  object_sha256=digest(obj), executable_sha256=digest(exe), linked_va=hex(linked_va))
    (out/'linked-close.json').write_text(json.dumps(result, indent=2)+'\n')
    (out/'source.asm').write_text(listing)
    print(json.dumps({k:v for k,v in result.items() if k not in ('score_graph','original_float_stores','source_float_stores','all_calls_and_data_verified','complete_decision_edges','decision_guard_templates')}))


if __name__ == '__main__':
    main()
