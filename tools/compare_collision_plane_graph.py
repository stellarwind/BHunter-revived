"""Static original/production destination-plane x87 expression comparison."""
import argparse
import json
import re
from pathlib import Path
from compare_ai_damage_query import digest, image_bytes
from compare_collision_cut_graph import op
from prove_collision_iteration import read_owner

ROOT = Path(__file__).resolve().parents[1]


def expression(rows, original):
    sp, node = 0x10000, 0x20000
    registers = dict(ESP=sp, EDI=node, EAX=node, ECX=0, ESI=0)
    memory = {node+24+4*i: ('input32', 'normal.'+str(i)) for i in range(3)}
    memory[node+36] = ('input32', 'plane')
    for i in range(3):
        memory[sp+(0xd0 if original else 0x48)+4*i] = ('input32', 'center.'+str(i))
    # Integer-only radius-address setup interleaved with the original sum.
    memory[0x4e0fd0] = 0x30000
    memory[0x30070] = 0x40000
    fp = []
    writes = []

    def address(arg):
        value = 0
        for term in re.search(r'\[([^]]+)\]', arg)[1].upper().replace(' ', '').split('+'):
            value += registers[term] if term in registers else int(term, 0)
        return value

    def read(arg):
        if '[' in arg:
            return memory[address(arg)]
        return registers[arg] if arg in registers else int(arg, 0)

    for row in rows:
        m, arg = row['mnemonic'], row['operands'].upper()
        parts = [a.strip() for a in arg.split(',')]
        if m == 'FLD':
            fp.insert(0, read(arg))
        elif m == 'FXCH':
            i = row['data'][1] & 7
            fp[0], fp[i] = fp[i], fp[0]
        elif m in ('FMUL', 'FADD'):
            fp[0] = op('mul' if m == 'FMUL' else 'add', fp[0], read(arg))
        elif m == 'FADDP':
            i = row['data'][1] & 7
            fp[i] = op('add', fp[i], fp[0])
            fp.pop(0)
        elif m == 'MOV':
            registers[parts[0]] = read(parts[1])
        elif m == 'ADD':
            registers[parts[0]] += read(parts[1])
        elif m == 'FSTP':
            assert 'QWORD' in arg  # PC53 binary64 store is exact in this profile.
            writes.append(fp.pop(0))
        else:
            raise AssertionError(row)
    assert (len(fp), len(writes)) == ((1, 0) if original else (0, 1))
    return fp[0] if original else writes[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--expect-mismatch', action='store_true')
    args = parser.parse_args()
    image = (ROOT/'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original = json.loads((ROOT/'META/m4/collision-traversal/original/0044c200.disassembly.json').read_text())
    original = [dict(address=int(r['address'], 16), data=bytes.fromhex(r['bytes']),
                     mnemonic=r['mnemonic'], operands=', '.join(r['operands']))
                for r in original if 0x44c301 <= int(r['address'], 16) < 0x44c33d]
    assert b''.join(r['data'] for r in original) == image_bytes(image, 0x44c301, 0x3c)
    source, _, _, _ = read_owner(args.object, '_node_contacts')
    source = [r for r in source if 0x71 <= r['address'] < 0x91]
    assert source[0]['address'] == 0x71 and source[-1]['address'] == 0x8d
    a, b = expression(original, True), expression(source, False)
    matches = a == b
    result = dict(passed=matches != args.expect_mismatch, expressions_match=matches,
                  expected_mismatch=args.expect_mismatch, static_only=True,
                  original_expression=a, source_expression=b,
                  original_sha256=digest(image), source_object=str(args.object),
                  source_object_sha256=digest(args.object.read_bytes()), candidate_read=False,
                  original_slice='0044c301..0044c33d', source_slice='node_contacts+71..91',
                  scope='Finite PC53 nearest masked products/sums with no overflow/underflow; individual commutation only, no reassociation. Binary64 spill exact in this profile. Transform and comparison remain separate contracts.')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    assert result['passed'], result
    print('Destination plane graphs ' + ('agree' if matches else 'reject old production grouping'))


if __name__ == '__main__':
    main()
