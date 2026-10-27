"""Read-only audit of every bootstrap sound-load call against immutable bytes.

No game execution or input. Equal filenames remain distinct resource identities.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from record_pickup_basic import reference, sha

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'src/bh_bootstrap_data.h'
ROW = re.compile(r'\{2,0x([0-9a-f]+)UL,("[^"\n]+"),([^,]+),([^,]+),([^,]+),([^}]+)\}')


def floating(bits):
    return struct.unpack('<f', struct.pack('<I', bits))[0]


def source_rows(text):
    return {int(identity, 16): dict(name=json.loads(name), range=float(ran),
            level=float(level), streaming=int(stream), denominator=float(den))
            for identity, name, ran, level, stream, den in ROW.findall(text)}


def contracts(raw):
    result = {}
    references = {}
    policies = []
    for owner, folder in [('004474c0', 'game-bootstrap'), ('00479200', 'startup-bootstrap')]:
        path = ROOT / f'META/m4/finish/{folder}/original/{owner}.disasm.json'
        rows = json.loads(path.read_text())
        for row in rows:
            data = bytes.fromhex(row['bytes'])
            assert data == raw(int(row['address'], 16), len(data)), row['address']
        references[path.relative_to(ROOT).as_posix()] = sha(path)

        def value(push_index):
            operand = rows[push_index]['operands'][0]
            if operand.startswith('0x'):
                return int(operand, 16)
            # Only verified nonvolatile constant registers appear in
            # these call arguments. Prove their reaching assignments rather
            # than assuming EBX/ESI are constants throughout both functions.
            assert operand in ('EBX', 'ESI', 'EBP'), rows[push_index]
            writes = [row for row in rows[:push_index] if row['operands'] and
                      row['operands'][0] == operand and
                      row['mnemonic'] not in ('PUSH', 'POP', 'CMP', 'TEST')]
            row = writes[-1]
            if row['mnemonic'] == 'XOR' and row['operands'] == [operand, operand]:
                return 0
            assert row['mnemonic'] == 'MOV' and row['operands'][1].startswith('0x'), row
            return int(row['operands'][1], 16)

        for i, row in enumerate(rows):
            if row['mnemonic'] != 'CALL':
                continue
            if row['operands'] == ['0x0047b920']:
                push = next(j for j in range(i - 1, -1, -1) if rows[j]['mnemonic'] == 'PUSH')
                policies.append(value(push))
                continue
            if row['operands'] != ['0x0047b950']:
                continue
            pushes = [j for j in range(i) if rows[j]['mnemonic'] == 'PUSH'][-5:]
            den, stream, level, ran, name = [value(j) for j in pushes]
            end = next((j for j in range(i + 1, len(rows)) if rows[j]['mnemonic'] == 'CALL'), len(rows))
            stores = [r for r in rows[i + 1:end] if r['mnemonic'] == 'MOV' and
                      len(r['operands']) == 2 and r['operands'][1] == 'EAX' and
                      re.fullmatch(r'\[0x[0-9a-f]+\]', r['operands'][0])]
            assert stores, row
            expected = dict(name=raw(name, 128).split(b'\0')[0].decode('ascii'),
                            range=floating(ran), level=floating(level),
                            streaming=stream, denominator=floating(den))
            for store in stores:
                identity = int(store['operands'][0][1:-1], 16)
                assert identity not in result, hex(identity)
                result[identity] = dict(expected, call=row['address'])
    return result, references, policies


def verify(text, expected):
    errors = []
    parsed = source_rows(text)
    for identity, entry in parsed.items():
        wanted = expected.get(identity)
        if wanted is None or any(entry[key] != wanted[key] for key in entry):
            errors.append(f'{identity:08x}: {entry} != {wanted}')
    # All load calls, including destinations whose EAX is first stored into
    # aliases, must have one actual source load row with its own metadata.
    calls = {row['call'] for row in expected.values()}
    observed = {expected[identity]['call'] for identity in parsed if identity in expected}
    if observed != calls:
        errors.append(f'Missing load calls: {sorted(calls - observed)}')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    _, raw = reference()
    expected, references, policies = contracts(raw)
    text = SOURCE.read_text()
    errors = verify(text, expected)
    assert not errors, '\n'.join(errors)
    source_policy = [int(p) for p in re.findall(r'\{6,(\d+),0,0,0,0,0\}', text)]
    assert source_policy == policies, (source_policy, policies)
    # Reproduce the old duplicate-filename collapse entirely in memory.
    wrong = text.replace('{2,0x004cdb88UL,"sound\\\\blip.wav",10,0.5,0,2}',
                         '{2,0x004cdb88UL,"sound\\\\blip.wav",10,0.5,1,2}')
    assert wrong != text and any(error.startswith('004cdb88:') for error in verify(wrong, expected))
    missing = ROW.sub(lambda m: '' if m.group(1) == '004cdb88' else m.group(0), text)
    assert any(error.startswith('Missing load calls:') for error in verify(missing, expected))
    result = dict(passed=True, source=SOURCE.relative_to(ROOT).as_posix(),
                  source_sha256=sha(SOURCE), references=references,
                  load_calls=len(source_rows(text)), voice_policy_requests=policies,
                  rejection_controls=2, original_code_executed=False,
                  game_launches=0, contracts={f'{key:08x}': value for key, value in expected.items()})
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f'Bootstrap sound audit passed: {result["load_calls"]} load calls, '
          f'{len(policies)} voice policy requests, 2 rejection controls; immutable instruction bytes verified.')


if __name__ == '__main__':
    main()
