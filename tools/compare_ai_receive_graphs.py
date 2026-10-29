"""Static receive-owner composition from actual instructions; no native execution.

Packet bytes/counters/clock are symbolic. The original inline envelope and the
actual compiled source envelope/read32/clock helpers execute in this interpreter.
Position decoding and construction are explicit typed dependency contracts.
"""
import argparse
import json
import re
import struct
from functools import lru_cache
from pathlib import Path

from compare_ai_damage_query_math import ROOT, digest, image_bytes, owner_instructions, parse_coff


def packed(parts):
    parts = tuple(parts)
    if all(isinstance(x, int) for x in parts):
        return sum(x << (8 * i) for i, x in enumerate(parts))
    if len(parts) == 4 and all(isinstance(x, tuple) and x[0] == 'slice' for x in parts):
        value = parts[0][1]
        if parts == tuple(('slice', value, i) for i in range(4)):
            return value
    return ('bytes', parts)


def octets(value, width=4):
    if isinstance(value, int):
        return tuple((value >> (8 * i)) & 255 for i in range(width))
    if value[0] == 'bytes':
        assert len(value[1]) >= width
        return value[1][:width]
    return tuple(('slice', value, i) for i in range(width))


def truncate(value, width):
    return packed(octets(value, width))


def add(a, b):
    if isinstance(a, int) and isinstance(b, int):
        return (a + b) & 0xffffffff
    if isinstance(a, int):
        a, b = b, a
    if isinstance(b, int):
        if not b:
            return a
        if a[0] == 'add':
            return add(a[1], (a[2] + b) & 0xffffffff)
        return ('add', a, b & 0xffffffff)
    ca, aa = (a[1], a[2]) if a[0] == 'mul' else (1, a)
    cb, ab = (b[1], b[2]) if b[0] == 'mul' else (1, b)
    if aa == ab:
        return multiply(aa, ca + cb)
    return ('sum32', tuple(sorted((a, b), key=repr)))


def multiply(a, n):
    n &= 0xffffffff
    if isinstance(a, int):
        return (a * n) & 0xffffffff
    if not n:
        return 0
    if n == 1:
        return a
    if a[0] == 'mul':
        return multiply(a[2], a[1] * n)
    return ('mul', n, a)


@lru_cache(None)
def xor_byte(a, b):
    terms, number = set(), 0
    for value in (a, b):
        if isinstance(value, int):
            number ^= value
            continue
        for term in value[1] if value[0] == 'xor' else (value,):
            if term in terms:
                terms.remove(term)
            else:
                terms.add(term)
    if number:
        terms.add(('constant-byte', number))
    if not terms:
        return 0
    return next(iter(terms)) if len(terms) == 1 else ('xor', tuple(sorted(terms, key=repr)))


def bitwise(op, a, b):
    if isinstance(a, int) and isinstance(b, int):
        return {'and': a & b, 'or': a | b, 'xor': a ^ b}[op] & 0xffffffff
    result = []
    for x, y in zip(octets(a), octets(b)):
        if op == 'xor':
            result.append(xor_byte(x, y))
        elif op == 'or':
            assert x == 0 or y == 0 or x == y, ('Non-disjoint symbolic OR', x, y)
            result.append(y if x == 0 else x)
        else:
            assert isinstance(y, int) and y in (0, 255), ('Unsupported symbolic mask', y)
            result.append(x if y else 0)
    return packed(result)


def sign_extend(value, width):
    value = truncate(value, width)
    if isinstance(value, int):
        bits = width * 8
        return (value - (1 << bits) if value & (1 << (bits - 1)) else value) & 0xffffffff
    return ('signed', width * 8, value)


REGISTERS = ('EAX', 'EBX', 'ECX', 'EDX', 'ESI', 'EDI', 'EBP', 'ESP')
ALIASES = {r[1:]: (r, 0, 2) for r in REGISTERS}
for r in ('EAX', 'EBX', 'ECX', 'EDX'):
    ALIASES[r[1] + 'L'] = (r, 0, 1)
    ALIASES[r[1] + 'H'] = (r, 1, 1)

CONTEXT, ACTORS, RESPONSE, DAMAGE, ROUTE = 0x10000, 0x11000, 0x12000, 0x13000, 0x14000
NAV, STATS, GOOD, CLOCK, PACKET, WORLD = 0x15000, 0x16000, 0x17000, 0x18000, 0x20000, 0x30000
GLOBAL_PACKET, GLOBAL_CURSOR = 0x19000, 0x19004
GLOBAL_MAP = {0x496bd8: GLOBAL_PACKET, 0x4cdba0: GLOBAL_CURSOR, 0x4dfacc: STATS,
              0x4dfad0: STATS + 4, 0x4dfac8: GOOD}


class Program:
    def __init__(self, path, name):
        self.path = path
        self.data = path.read_bytes()
        self.symbols, self.sections = parse_coff(self.data)
        symbol = next(v for v in self.symbols.values() if v['name'] == name)
        self.rows, self.listing, self.end, _ = owner_instructions(path, self.symbols, self.sections, name)
        self.name = name
        self.calls = {}
        for relocation in self.sections[symbol['section']]['relocations']:
            assert relocation['kind'] == 20, ('Unexpected owner relocation', name, relocation)
            self.calls[relocation['offset'] - 1] = relocation['symbol']['name']


class Machine:
    def __init__(self, source, length, checksum_ok, position_ok, constructed, live_clock, programs):
        self.source, self.length = source, length
        self.checksum_ok, self.position_ok = checksum_ok, position_ok
        self.constructed, self.live_clock, self.programs = constructed, live_clock, programs
        self.memory, self.events, self.comparisons, self.covered, self.writes = {}, [], [], {}, []
        self.reg = {r: 0x70000 + i * 0x100 for i, r in enumerate(REGISTERS)}
        self.reg['ESP'] = 0x90000
        self.saved = self.reg.copy()
        self.fp = []
        self.zero, self.carry = False, False
        for at in range(CONTEXT, CONTEXT + 112):
            self.memory[at] = 0
        for at, value in ((CONTEXT, ACTORS), (ACTORS, RESPONSE), (RESPONSE, DAMAGE), (DAMAGE + 4, ROUTE),
                          (CONTEXT + 8, NAV), (CONTEXT + 12, STATS), (CONTEXT + 16, GOOD),
                          (ROUTE + 24, CLOCK if live_clock else 0), (GLOBAL_PACKET, PACKET)):
            self.store(at, value, 4)
        self.clock = CLOCK if live_clock else ROUTE + 16
        for at, label in ((STATS, 'accepted'), (STATS + 4, 'rejected'), (GOOD, 'good'), (self.clock, 'clock')):
            self.store(at, ('input32', label), 4)
        self.initial_clock = self.load(self.clock, 2)
        for at in range(512):
            self.memory[PACKET + at] = ('packet-byte', at)
        self.memory[PACKET + 1] = length
        self.store(0x90000, 0xeeeeeeee, 4)
        if source:
            for i, value in enumerate((CONTEXT, PACKET, length + 8, 512)):
                self.store(0x90004 + i * 4, value, 4)
        self.writes.clear()

    def mapped(self, at):
        return self.clock if at == 0x4dfac4 else GLOBAL_MAP.get(at, at)

    def load(self, at, width):
        assert isinstance(at, int), ('Nonconcrete memory address', at)
        at = self.mapped(at)
        return packed(self.memory[at + i] for i in range(width))

    def store(self, at, value, width):
        assert isinstance(at, int), ('Nonconcrete store address', at)
        at = self.mapped(at)
        for i, byte in enumerate(octets(value, width)):
            self.memory[at + i] = byte
        self.writes.append((at, width, value))

    def reg_read(self, name):
        if name in self.reg:
            return self.reg[name]
        register, start, width = ALIASES[name]
        return packed(octets(self.reg[register])[start:start + width])

    def reg_write(self, name, value):
        if name in self.reg:
            self.reg[name] = value
        else:
            register, start, width = ALIASES[name]
            parts = list(octets(self.reg[register]))
            parts[start:start + width] = octets(value, width)
            self.reg[register] = packed(parts)

    def address(self, operand):
        expr = re.search(r'\[(.*?)\]', operand)[1]
        result = 0
        for term in expr.replace(' - ', ' + -').split('+'):
            term = term.strip()
            if '*' in term:
                a, b = (v.strip() for v in term.split('*'))
                if a in self.reg:
                    value = multiply(self.reg[a], int(b, 0))
                else:
                    value = multiply(self.reg[b], int(a, 0))
            else:
                value = self.reg_read(term) if term in self.reg or term in ALIASES else int(term, 0)
            result = add(result, value)
        return result

    @staticmethod
    def width(operand):
        return 1 if 'BYTE' in operand else 2 if 'WORD' in operand and 'DWORD' not in operand else 4

    def read(self, operand):
        if '[' in operand:
            return self.load(self.address(operand), self.width(operand))
        if operand in self.reg or operand in ALIASES:
            return self.reg_read(operand)
        return int(operand, 0) & 0xffffffff

    def write(self, operand, value):
        if '[' in operand:
            self.store(self.address(operand), value, self.width(operand))
        else:
            self.reg_write(operand, value)

    def args(self, count):
        return [self.load(self.reg['ESP'] + 4 * i, 4) for i in range(count)]

    def dependency(self, target):
        if target in ('_bh_packet_read_position', '0x00423840'):
            if self.source:
                reader, navigation, world, position = self.args(4)
                assert (reader, navigation) == (CONTEXT + 76, NAV)
                cursor = PACKET + self.load(reader + 8, 4)
            else:
                world, position = self.args(2)
                cursor = self.load(GLOBAL_CURSOR, 4)
            self.events.append(('position', cursor - PACKET, self.position_ok,
                                self.load(STATS, 4), self.load(STATS + 4, 4), self.load(GOOD, 4),
                                self.load(self.clock, 4)))
            consumed = 8 if self.position_ok else 2
            if self.source:
                self.store(reader + 8, cursor - PACKET + consumed, 4)
            else:
                self.store(GLOBAL_CURSOR, cursor + consumed, 4)
            if self.position_ok:
                self.store(world, WORLD, 4)
                for i in range(3):
                    self.store(position + 4 * i, ('decoded-position', i), 4)
            else:
                self.store(STATS, add(self.load(STATS, 4), -1), 4)
                self.store(STATS + 4, add(self.load(STATS + 4, 4), 1), 4)
            self.reg['EAX'] = int(self.position_ok)
        elif target in ('_bh_actor_construct_position', '0x0043b2d0'):
            arguments = self.args(7 if self.source else 6)
            if self.source:
                assert arguments.pop(0) == ACTORS
            position, kind, token, speed, tag, seed = arguments
            point = tuple(self.load(position + i * 4, 4) for i in range(3))
            self.events.append(('construct', point, kind, token, speed, tag, seed,
                                self.load(self.clock, 2), self.load(STATS, 4), self.load(GOOD, 4)))
            # The child may change both halves; only the caller's low word restores.
            self.store(self.clock, ('input32', 'constructor-clock'), 4)
            self.reg['EAX'] = 0x40000 if self.constructed else 0
        else:
            raise AssertionError(('Unreviewed dependency', target))

    def run(self, rows, name, calls):
        lookup = {r['address']: i for i, r in enumerate(rows)}
        cursor, steps = 0, 0
        while True:
            row = rows[cursor]
            self.covered.setdefault(name, set()).add(row['address'])
            mnemonic, operand = row['mnemonic'], row['operands'].upper()
            operands = [v.strip() for v in operand.split(',')] if operand else []
            cursor += 1
            steps += 1
            assert steps < 10000, ('Unbounded instruction loop', name)
            if self.instruction(row, name, operands):
                continue
            if mnemonic in ('MOV', 'MOVZX', 'MOVSX'):
                value = self.read(operands[1])
                if mnemonic == 'MOV' and '[' in operands[0] and 'PTR' not in operands[0] and operands[1] in ALIASES:
                    operands[0] = ('BYTE PTR ' if ALIASES[operands[1]][2] == 1 else 'WORD PTR ') + operands[0]
                if mnemonic == 'MOVSX':
                    width = self.width(operands[1]) if '[' in operands[1] else ALIASES[operands[1]][2]
                    value = sign_extend(value, width)
                elif mnemonic == 'MOVZX':
                    width = self.width(operands[1]) if '[' in operands[1] else ALIASES[operands[1]][2]
                    value = packed(octets(value, width) + (0,) * (4 - width))
                self.write(operands[0], value)
            elif mnemonic == 'LEA':
                self.write(operands[0], self.address(operands[1]))
            elif mnemonic in ('ADD', 'SUB', 'INC', 'DEC'):
                a = self.read(operands[0])
                b = self.read(operands[1]) if len(operands) == 2 else 1
                value = add(a, multiply(b, -1) if mnemonic in ('SUB', 'DEC') else b)
                self.write(operands[0], value)
                self.zero = value == 0
            elif mnemonic == 'NEG':
                self.write(operands[0], multiply(self.read(operands[0]), -1))
            elif mnemonic in ('XOR', 'AND', 'OR'):
                value = bitwise(mnemonic.lower(), self.read(operands[0]), self.read(operands[1]))
                self.write(operands[0], value)
                self.zero, self.carry = value == 0, False
            elif mnemonic in ('SHL', 'SHR'):
                value, count = self.read(operands[0]), self.read(operands[1])
                if isinstance(value, int):
                    value = ((value << count) if mnemonic == 'SHL' else (value >> count)) & 0xffffffff
                elif mnemonic == 'SHL' and value[0] != 'bytes':
                    value = multiply(value, 1 << count)
                else:
                    assert count % 8 == 0
                    shift = count // 8
                    parts = octets(value)
                    value = packed((0,) * shift + parts[:4 - shift] if mnemonic == 'SHL' else parts[shift:] + (0,) * shift)
                self.write(operands[0], value)
            elif mnemonic in ('CMP', 'TEST'):
                a, b = map(self.read, operands)
                if mnemonic == 'TEST':
                    assert a == b
                    self.zero, self.carry = a == 0, False
                elif isinstance(a, int) and isinstance(b, int):
                    self.zero, self.carry = a == b, a < b
                else:
                    assert (not self.source and row['address'] == 0x426dc4) or (name == '_bh_packet_receive' and row['address'] == 0xe8)
                    self.comparisons.append(tuple(sorted((a, b), key=repr)))
                    self.zero, self.carry = self.checksum_ok, False
            elif mnemonic in ('PUSH', 'POP'):
                if mnemonic == 'PUSH':
                    value = self.read(operands[0])
                    self.reg['ESP'] -= 4
                    self.store(self.reg['ESP'], value, 4)
                else:
                    value = self.load(self.reg['ESP'], 4)
                    self.reg['ESP'] += 4
                    self.write(operands[0], value)
            elif mnemonic == 'CALL':
                target = calls.get(row['address'], operands[0].lower())
                if target in self.programs:
                    self.reg['ESP'] -= 4
                    self.store(self.reg['ESP'], 0xdddddddd, 4)
                    helper = self.programs[target]
                    self.run(helper.rows, helper.name, helper.calls)
                    self.reg['ESP'] += 4
                else:
                    self.dependency(target)
            elif mnemonic in ('JE', 'JZ', 'JNE', 'JNZ', 'JA', 'JAE', 'JB', 'JBE', 'JMP', 'JL'):
                if mnemonic == 'JL':
                    assert isinstance(getattr(self, 'signed_less', None), bool), 'Signed comparison contract missing'
                take = {'JE': self.zero, 'JZ': self.zero, 'JNE': not self.zero, 'JNZ': not self.zero,
                        'JA': not self.carry and not self.zero, 'JAE': not self.carry, 'JB': self.carry,
                        'JBE': self.carry or self.zero, 'JMP': True, 'JL': getattr(self, 'signed_less', None)}[mnemonic]
                if take:
                    cursor = lookup[int(operands[0], 0)]
            elif mnemonic in ('STOSD.REP', 'STOSB.REP', 'REP'):
                operation = operand.split()[0] if mnemonic == 'REP' else mnemonic.split('.')[0]
                assert operation in ('STOSD', 'STOSB', 'MOVSD')
                width = 1 if operation == 'STOSB' else 4
                for _ in range(self.reg['ECX']):
                    value = self.load(self.reg['ESI'], width) if operation == 'MOVSD' else truncate(self.reg['EAX'], width)
                    self.store(self.reg['EDI'], value, width)
                    self.reg['EDI'] += width
                    if operation == 'MOVSD':
                        self.reg['ESI'] += width
                self.reg['ECX'] = 0
            elif mnemonic == 'FILD':
                value = self.read(operands[0])
                assert value[0:2] == ('signed', 8), ('Unexpected floating conversion domain', value)
                self.fp.insert(0, ('exact-f32-signed-byte', value))
            elif mnemonic == 'FSTP':
                self.write(operands[0], self.fp.pop(0))
            elif mnemonic == 'RET':
                return
            else:
                raise AssertionError(('Unsupported instruction', name, row))

    def instruction(self, row, name, operands):
        """Optional explicit instruction semantics for other static owner audits."""
        return False

    def observation(self):
        # Codec/source diagnostic metadata is an explicit adapter; packet/counters,
        # cursor, clock and construction arguments are the original observables.
        cursor = PACKET + self.load(CONTEXT + 84, 4) if self.source else self.load(GLOBAL_CURSOR, 4)
        return (self.events, self.comparisons, tuple(self.memory[PACKET + i] for i in range(264)),
                self.load(STATS, 4), self.load(STATS + 4, 4), self.load(GOOD, 4), self.load(self.clock, 4), cursor)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=Path, default=ROOT / 'build/game/bh_actor_launch.obj')
    parser.add_argument('--output-directory', type=Path, default=ROOT / 'build/ai-receive-match/graphs')
    args = parser.parse_args()
    out = args.output_directory
    out.mkdir(parents=True, exist_ok=True)
    image = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original = json.loads((ROOT / 'META/m4/target656/launch/original/00426d50.disasm.json').read_text())
    assert b''.join(bytes.fromhex(r['bytes']) for r in original) == image_bytes(image, 0x426d50, 349)
    original = [dict(address=int(r['address'], 16), mnemonic=r['mnemonic'], operands=', '.join(r['operands'])) for r in original]
    owner = Program(args.object, '_bh_actor_launch_receive')
    helpers = [Program(ROOT / 'build/game/bh_network_codec.obj', name) for name in ('_bh_packet_receive', '_read32')]
    helpers += [Program(ROOT / 'build/game/bh_actor_route.obj', name) for name in ('_bh_actor_route_now', '_bh_actor_route_set_tick')]
    programs = {p.name: p for p in helpers}
    coverage, cases, graphs = {}, 0, []
    for length in range(256):
        paths = [(False, False, False)]
        if length >= 2:
            paths.append((True, False, False))
        if length >= 17:
            paths.extend(((True, True, False), (True, True, True)))
        for checksum_ok, position_ok, constructed in paths:
            for live in (False, True):
                old = Machine(False, length, checksum_ok, position_ok, constructed, live, programs)
                new = Machine(True, length, checksum_ok, position_ok, constructed, live, programs)
                old.run(original, 'original', {})
                new.run(owner.rows, owner.name, owner.calls)
                a, b = old.observation(), new.observation()
                if a != b:
                    (out / 'difference.json').write_text(json.dumps(dict(length=length, path=(checksum_ok, position_ok, constructed, live), original=a, source=b), indent=2) + '\n')
                    raise AssertionError(('Receive graph differs', length, checksum_ok, position_ok, constructed, live))
                reached_constructor = checksum_ok and position_ok
                assert new.reg['EAX'] == int(reached_constructor and constructed)
                assert new.load(CONTEXT + 100, 4) == int(reached_constructor)
                assert new.load(CONTEXT + 108, 4) == (0 if reached_constructor and constructed else 3 if reached_constructor else 2)
                for machine in (old, new):
                    assert not machine.fp and machine.reg['ESP'] == machine.saved['ESP']
                    assert all(machine.reg[r] == machine.saved[r] for r in ('EBX', 'ESI', 'EDI', 'EBP'))
                    for name, covered in machine.covered.items():
                        coverage.setdefault(name, set()).update(covered)
                graphs.append(digest(repr(a).encode()))
                cases += 1
    missing_original = [r['address'] for r in original if r['address'] not in coverage['original']]
    assert not missing_original, missing_original
    owner_missing = [r['address'] for r in owner.rows if r['address'] not in coverage[owner.name]]
    assert owner_missing == [0x199, 0x1a0, 0x1a7, 0x1a9, 0x1aa, 0x1ab, 0x1ac, 0x1ad, 0x1b0,
                             0x1c2, 0x1c4, 0x1c6, 0x1cd, 0x1ce, 0x1cf, 0x1d0, 0x1d2, 0x1d3, 0x1d6]
    # Each signed-byte integer is exactly representable in binary32 at every
    # standard x87 precision; the actual FILD/FSTP argument graph uses only it.
    assert all(struct.unpack('<f', struct.pack('<f', float(i)))[0] == i for i in range(-128, 128))
    for helper in helpers:
        missing = [r['address'] for r in helper.rows if r['address'] not in coverage[helper.name]]
        assert missing == ([0x11e, 0x11f, 0x120, 0x121, 0x124, 0x125, 0x128]
                           if helper.name == '_bh_packet_receive' else []), (helper.name, missing)
    result = dict(original_va='00426d50', original_owner_bytes=349, source_owner_bytes=owner.end,
                  cases=cases, all_original_instructions_covered=True, packet_bytes_counters_cursor_clock_and_constructor_graphs_equal=True,
                  graph_digest=digest(json.dumps(graphs).encode()), object_sha256=digest(owner.data),
                  helpers=[dict(name=p.name, object=str(p.path), sha256=digest(p.data), bytes=p.end,
                                uncovered=[r['address'] for r in p.rows if r['address'] not in coverage[p.name]]) for p in helpers],
                  source_uncovered=owner_missing, all_valid_source_instructions_covered=True,
                  source_exclusion='Only malformed-tail/context-argument guard epilogues; helper excludes only invalid-envelope guard return.',
                  signed_byte_to_binary32_exact_domain_values=256,
                  limits=['Typed position-decode and constructor contracts; no original-inner-body proof for those dependencies.',
                          'Valid borrowed disjoint contexts/packet/stats/clock, stable route binding; arbitrary aliases/reentrancy excluded.',
                          'Checksum rejection covers every declared payload length0..255; index rejection needs2 bytes; accepted construction needs17 payload bytes.',
                          'Source diagnostics/result/received counter, bounded malformed-envelope/tail guards and clock ABI are explicit adapters.',
                          'Compare dependency-boundary state/arguments and final state; private/source metadata publication and pure stores between boundaries may be scheduled differently.',
                          'Integer symbolic bytes/clock/counters are arbitrary; signed-byte FILD to binary32 is exact. No other floating arithmetic occurs here.',
                          'No byte match, binary drop-in ABI or all-profile whole-AI claim.'])
    (out / 'graphs.json').write_text(json.dumps(result, indent=2) + '\n')
    (out / 'source.asm').write_text(owner.listing)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
