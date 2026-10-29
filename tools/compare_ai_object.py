"""Static VC5 COFF/reference instruction comparison; never launches code."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/astra-review'))
from targeted_probes import parse_coff


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('object', type=Path)
    p.add_argument('symbol')
    p.add_argument('address', type=lambda v: int(v, 0))
    p.add_argument('length', type=lambda v: int(v, 0))
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--bind', action='append', default=[], metavar='SYMBOL=VA',
                   help='Explicit original callee address for a COFF REL32 call')
    p.add_argument('--data-bind', action='append', default=[], metavar='SYMBOL=VA',
                   help='Reviewed source data binding for an original indexed LEA')
    p.add_argument('--switch-table', action='append', default=[], metavar='VA=COUNT',
                   help='Original jump table; verify every COFF entry against its owner-relative target')
    p.add_argument('--constant-bind', action='append', default=[], metavar='SYMBOL=VA:SIZE',
                   help='Verify an entire referenced initialized constant/vector, including PUSH operands')
    a = p.parse_args()
    bindings = {name: int(va, 0) for name, va in (v.split('=', 1) for v in a.bind)}
    data_bindings = {name: int(va, 0) for name, va in (v.split('=', 1) for v in a.data_bind)}
    switch_tables = {int(va, 0): int(count, 0) for va, count in (v.split('=', 1) for v in a.switch_table)}
    constant_bindings = {name: tuple(int(n, 0) for n in spec.split(':'))
                         for name, spec in (v.split('=', 1) for v in a.constant_bind)}
    reference = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(reference).hexdigest() == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    def raw(va, n):
        pe = struct.unpack_from('<I', reference, 0x3c)[0]
        sections_at = pe + 24 + struct.unpack_from('<H', reference, pe + 20)[0]
        for i in range(struct.unpack_from('<H', reference, pe + 6)[0]):
            at = sections_at + 40*i
            size, rva, disk_size, disk = struct.unpack_from('<IIII', reference, at+8)
            offset = va - 0x400000 - rva
            if 0 <= offset and offset+n <= disk_size:
                return reference[disk+offset:disk+offset+n]
        raise ValueError(hex(va))
    obj = a.object.read_bytes()
    symbols, sections = parse_coff(obj)
    symbol = next(s for s in symbols.values() if s['name'] == a.symbol)
    section = sections[symbol['section']]
    generated = bytearray(section['data'][symbol['value']:])
    original = raw(a.address, a.length)
    relocations = []
    associated_tables = []
    # Relocations are accepted only if the original instruction operand has the
    # same constant bytes. Unresolved calls/globals remain mismatches.
    for r in section['relocations']:
        offset = r['offset']-symbol['value']
        if offset < 0 or offset+4 > len(original):
            continue
        s = r['symbol']
        row = dict(offset=offset, kind=r['kind'], symbol=s['name'], verified=False)
        operand = struct.unpack_from('<I', original, offset)[0]
        if (r['kind'] == 6 and operand in switch_tables and s['section'] > 0
                and offset >= 3 and original[offset-3:offset] == b'\xff\x24\x85'):
            table_section = sections[s['section']]
            table_start = s['value'] + struct.unpack_from('<I', generated, offset)[0]
            count = switch_tables[operand]
            proof = []
            for index in range(count):
                at = table_start + index*4
                entry_relocations = [v for v in table_section['relocations'] if v['offset'] == at]
                entry = dict(index=index, verified=False)
                if len(entry_relocations) == 1:
                    er = entry_relocations[0]
                    target_symbol = er['symbol']
                    target = target_symbol['value'] + struct.unpack_from('<I', table_section['data'], at)[0]
                    relative_target = target - symbol['value']
                    original_target = struct.unpack('<I', raw(operand+index*4, 4))[0]
                    entry.update(source_relative_target=relative_target, original_target=hex(original_target))
                    entry['verified'] = (er['kind'] == 6 and target_symbol['section'] == symbol['section']
                                         and 0 <= relative_target < a.length
                                         and original_target == a.address + relative_target)
                proof.append(entry)
            row['verified'] = (count > 0 and len(proof) == count and all(v['verified'] for v in proof)
                               and (s['section'] != symbol['section'] or table_start-symbol['value'] >= a.length))
            row.update(binding_kind='verified complete switch table', entries=proof, original_table=hex(operand))
            if row['verified']:
                generated[offset:offset+4] = original[offset:offset+4]
                if s['section'] == symbol['section']:
                    associated_tables.append((table_start-symbol['value'], count*4))
        elif r['kind'] == 6 and s['name'] in constant_bindings and s['section'] > 0:
            base, size = constant_bindings[s['name']]
            addend = struct.unpack_from('<I', generated, offset)[0]
            constant = sections[s['section']]['data'][s['value']:s['value']+size]
            opcode = original[offset-2] if offset >= 2 else 0
            modrm = original[offset-1] if offset >= 1 else 0
            push_address = offset > 0 and original[offset-1] == 0x68 and addend == 0
            float_read = opcode in (0xd8,0xd9,0xdc,0xdd) and modrm & 0xc7 == 5
            row.update(binding_kind='verified complete initialized vector/constant',
                       original_base=hex(base), storage_bytes=size, addend=addend, constant_hex=constant.hex())
            row['verified'] = (size > 0 and len(constant) == size and constant == raw(base,size)
                               and 0 <= addend < size and operand == base+addend and (push_address or float_read))
            if row['verified']:
                generated[offset:offset+4] = original[offset:offset+4]
        elif r['kind'] == 6 and s['name'] in data_bindings:
            operand = struct.unpack_from('<I', original, offset)[0]
            addend = struct.unpack_from('<I', generated, offset)[0]
            indexed_lea = (offset >= 3 and original[offset-3] == 0x8d
                           and original[offset-2] & 0xc7 == 4
                           and original[offset-1] & 7 == 5)
            row.update(original_operand=hex(operand), bound_data=hex(data_bindings[s['name']]),
                       binding_kind='reviewed source data adapter', defined_in_object=s['section'] > 0)
            row['verified'] = indexed_lea and operand == data_bindings[s['name']] and addend == 0 and s['section'] >= 0
            if row['verified']:
                generated[offset:offset+4] = original[offset:offset+4]
        elif r['kind'] == 6 and s['section'] > 0:
            operand = struct.unpack_from('<I', original, offset)[0]
            addend = struct.unpack_from('<I', generated, offset)[0]
            data = sections[s['section']]['data']
            start = s['value']+addend
            # Decode only x87 absolute-memory operands used by these functions.
            # Symbol names do not reliably encode the constant's storage width.
            opcode = original[offset-2] if offset >= 2 else 0
            modrm = original[offset-1] if offset >= 1 else 0
            width = 8 if opcode in (0xdc, 0xdd) else 4
            constant = data[start:start+width]
            try:
                row['verified'] = (opcode in (0xd8, 0xd9, 0xdc, 0xdd)
                                   and modrm & 0xc7 == 5 and len(constant) == width
                                   and constant == raw(operand, width))
            except ValueError:
                pass
            row.update(original_operand=hex(operand), constant_hex=constant.hex())
            if row['verified']:
                generated[offset:offset+4] = original[offset:offset+4]
        elif r['kind'] == 20 and s['name'] in bindings:
            target = a.address+offset+4+struct.unpack_from('<i', original, offset)[0]
            addend = struct.unpack_from('<i', generated, offset)[0]
            row.update(original_callee=hex(target), bound_callee=hex(bindings[s['name']]))
            row['verified'] = offset > 0 and original[offset-1] == 0xe8 and target == bindings[s['name']] and addend == 0
            if row['verified']:
                generated[offset:offset+4] = original[offset:offset+4]
        relocations.append(row)
    mismatch = [i for i, (x,y) in enumerate(zip(generated,original)) if x != y]
    trailing = bytearray(generated[len(original):])
    for start, size in associated_tables:
        trailing[start-len(original):start-len(original)+size] = b'\x90'*size
    def padding_only(data):
        while data:
            for nop in (b'\x8b\xff', b'\x8d\x49\x00', b'\x90', b'\xcc', b'\x00'):
                if data.startswith(nop):
                    data = data[len(nop):]
                    break
            else:
                return False
        return True
    match = (len(generated) >= len(original) and not mismatch
             and padding_only(trailing)
             and all(r['verified'] for r in relocations))
    result = dict(symbol=a.symbol, original_va=hex(a.address), original_length=a.length,
                  object=str(a.object), object_sha256=hashlib.sha256(obj).hexdigest(),
                  generated_length=len(generated), original_hex=original.hex(),
                  generated_hex=generated.hex(), relocations=relocations,
                  verified_associated_tables=associated_tables,
                  matched=match, mismatch_count=len(mismatch), first_mismatch=mismatch[:12])
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('original_hex','generated_hex','relocations')}))


if __name__ == '__main__':
    main()
