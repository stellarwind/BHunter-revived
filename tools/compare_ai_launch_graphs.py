"""Static complete launch-owner graphs; executes no game or original code."""
import argparse
import json
import re
import struct
from pathlib import Path

from compare_ai_receive_graphs import (Machine, REGISTERS, CONTEXT, ACTORS, RESPONSE, DAMAGE, ROUTE,
                                     NAV, CLOCK, PACKET, WORLD, add, multiply, packed, octets, truncate)
from compare_ai_damage_query_math import ROOT, digest, image_bytes, owner_instructions, parse_coff

WRITER_CURSOR, TRANSPORT, NODES = 0x19008, 0x40000, 0x50000
MAGIC = 0x68db8bad


def division_lemma():
    # M/2^43 exceeds1/5000 by delta/(5000*2^43). Over signed32,
    # its accumulated error is strictly less than1/5000, the smallest
    # nonzero remainder fraction. Positive floor and negative floor+1
    # therefore implement truncation toward zero, including exact multiples.
    delta = MAGIC * 5000 - (1 << 43)
    assert delta > 0 and (1 << 31) * delta < (1 << 43)
    assert ((1 << 31) * MAGIC >> 43) < (1 << 31)
    # Q*5000 cannot overflow signed32, so dividing the product recovers Q.
    assert (0x7fffffff // 5000) * 5000 <= 0x7fffffff
    assert (0x80000000 // 5000) * 5000 <= 0x80000000
    return dict(multiplier=MAGIC, shift=43, positive_error_numerator=delta,
                maximum_error_less_than_one_remainder_step=True,
                quantized_signed32_product_no_overflow=True)


def quotient(value):
    if isinstance(value, int):
        signed = value - (1 << 32) if value & 0x80000000 else value
        return ((abs(signed) // 5000) * (-1 if signed < 0 else 1)) & 0xffffffff
    if value[0] == 'mul' and value[1] == 5000 and value[2][0] == 'quotient5000':
        return value[2]
    return ('quotient5000', value)


def rounded(value):
    return value if value[0] in ('f32', 'float-input') else ('f32', value)


def fp_binary(operation, a, b):
    a, b = sorted((a, b), key=repr)
    return (operation + '-PC53', a, b)


class Program:
    def __init__(self, path, name):
        self.path, self.name = path, name
        self.data = path.read_bytes()
        symbols, sections = parse_coff(self.data)
        owner = next(v for v in symbols.values() if v['name'] == name)
        self.rows, self.listing, self.end, _ = owner_instructions(path, symbols, sections, name)
        self.calls, self.constants = {}, {}
        section = sections[owner['section']]
        for relocation in section['relocations']:
            at, symbol = relocation['offset'], relocation['symbol']
            row = next(r for r in self.rows if r['address'] <= at < r['address'] + len(r['data']))
            if relocation['kind'] == 20:
                self.calls[row['address']] = symbol['name']
            else:
                assert relocation['kind'] == 6 and row['mnemonic'].startswith('F')
                offset = symbol['value'] + struct.unpack_from('<I', section['data'], at)[0]
                width = 8 if 'QWORD' in row['operands'].upper() else 4
                value = struct.unpack('<d' if width == 8 else '<f', sections[symbol['section']]['data'][offset:offset + width])[0]
                self.constants[row['address']] = ('constant', value.hex())


class LaunchMachine(Machine):
    def __init__(self, source, mode, kind, world, index, live, programs):
        super().__init__(source, 0, True, True, True, live, programs)
        self.mode, self.kind, self.world, self.index = mode, kind, WORLD if world else 0, index
        self.rand_calls, self.sent = 0, False
        self.memory.clear()
        for at in range(CONTEXT, CONTEXT + 112):
            self.memory[self.mapped(at)] = 0
        for at, value in ((CONTEXT, ACTORS), (ACTORS, RESPONSE), (RESPONSE, DAMAGE), (DAMAGE + 4, ROUTE),
                          (CONTEXT + 4, TRANSPORT), (CONTEXT + 8, NAV), (CONTEXT + 20, mode),
                          (ROUTE + 24, CLOCK if live else 0), (NAV, NODES), (NAV + 4, index + 1)):
            self.store(at, value, 4)
        self.clock = CLOCK
        self.store(self.clock, ('input32', 'clock'), 4)
        self.store(CONTEXT + 24, self.load(CLOCK, 2), 2)
        self.store(CONTEXT + 96, ('input32', 'sent'), 4)
        self.store(WORLD + 0x108, index, 2)
        for i in range(index + 1):
            self.store(NODES + i * 16, WORLD if i == index else WORLD + 0x1000 + i * 0x100, 4)
        for axis in range(3):
            self.store(WORLD + 0x1e8 + 4 * axis, ('float-input', 'minimum.' + str(axis)), 4)
            self.store(WORLD + 0x1f4 + 4 * axis, ('float-input', 'maximum.' + str(axis)), 4)
        for i in range(28):
            self.memory[PACKET + i] = ('old-packet', i)
        self.store(0x90000, 0xeeeeeeee, 4)
        arguments = [self.world, kind, ('input32', 'token'), ('float-input', 'speed'), ('input32', 'tag')]
        if source:
            arguments.insert(0, CONTEXT)
        for i, value in enumerate(arguments):
            self.store(0x90004 + 4 * i, value, 4)
        self.writes.clear()

    def mapped(self, at):
        if self.source and CONTEXT + 26 <= at < CONTEXT + 54:
            return PACKET + at - CONTEXT - 26
        if not self.source:
            if 0x4d1490 <= at < 0x4d14ac:
                return PACKET + at - 0x4d1490
            if WORLD + 0x1b0 <= at < WORLD + 0x1c8:
                return at + 0x38
            if at == 0x4dfb08:
                return CONTEXT + 20
            if at == 0x4d1a74:
                return WRITER_CURSOR
            if at == 0x4dfac4:
                return self.clock
        return at

    def floating(self, row, name, operand):
        if self.source and row['address'] in self.programs[name].constants:
            return self.programs[name].constants[row['address']]
        if not self.source and '[0x00489280]' in operand.lower():
            assert image_bytes(self.image, 0x489280, 8) == struct.pack('<d', .5)
            return ('constant', float(.5).hex())
        value = self.read(operand)
        assert not isinstance(value, int), ('Unexpected floating bits', name, row)
        return value

    def instruction(self, row, name, operands):
        m = row['mnemonic']
        if m in ('FLD', 'FST', 'FSTP', 'FILD', 'FADD', 'FMUL', 'FXCH', 'FADDP'):
            if m == 'FLD':
                self.fp.insert(0, self.floating(row, name, operands[0]))
            elif m == 'FILD':
                value = self.read(operands[0])
                assert value[0] == 'ftol-low32'
                self.fp.insert(0, ('signed32-to-float', value))
            elif m in ('FST', 'FSTP'):
                self.write(operands[0], rounded(self.fp[0]))
                if m == 'FSTP':
                    self.fp.pop(0)
            elif m in ('FADD', 'FMUL'):
                self.fp[0] = fp_binary('add' if m == 'FADD' else 'mul', self.fp[0], self.floating(row, name, operands[0]))
            elif m == 'FXCH':
                index = int(re.search(r'[0-7]', operands[0])[0]) if operands else 1
                self.fp[0], self.fp[index] = self.fp[index], self.fp[0]
            else:
                index = int(re.search(r'[0-7]', operands[0])[0])
                self.fp[index] = fp_binary('add', self.fp[index], self.fp[0])
                self.fp.pop(0)
            return True
        if m == 'IMUL':
            assert len(operands) == 1 and self.reg['EAX'] == MAGIC
            value = self.read(operands[0])
            self.reg['EAX'] = ('product-low32', MAGIC, value)
            self.reg['EDX'] = ('signed-product-high32', MAGIC, value)
            return True
        if m == 'SAR':
            value, count = self.read(operands[0]), self.read(operands[1])
            assert count == 11 and value[0:2] == ('signed-product-high32', MAGIC)
            self.write(operands[0], ('magic-floor', value[2]))
            return True
        if m == 'SHR':
            value, count = self.read(operands[0]), self.read(operands[1])
            if isinstance(value, tuple) and value[0] == 'magic-floor':
                assert count == 31
                self.write(operands[0], ('magic-negative-bit', value[1]))
                return True
        if m == 'ADD':
            a, b = map(self.read, operands)
            if isinstance(a, tuple) and a[0] == 'magic-floor':
                assert b == ('magic-negative-bit', a[1])
                self.write(operands[0], quotient(a[1]))
                return True
        return False

    def dependency(self, target):
        if target in ('_rand', '0x0047ddf0'):
            value = ('rand32', self.rand_calls)
            self.events.append(('rand', self.rand_calls))
            self.rand_calls += 1
            self.reg['EAX'] = value
        elif target in ('__ftol', '0x0047de20'):
            value = self.fp.pop(0)
            self.events.append(('ftol64', value))
            self.reg['EAX'], self.reg['EDX'] = ('ftol-low32', value), ('ftol-high32', value)
        elif target in ('_bh_packet_write_position', '0x004234d0'):
            if self.source:
                writer, world, index, point = self.args(4)
                assert writer == CONTEXT + 56
                start = self.load(writer + 8, 4)
            else:
                bound, point = self.args(2)
                world = self.load(bound, 4)
                index = self.load(world + 0x108, 2)
                start = self.load(WRITER_CURSOR, 4) - 0x4d1490
            center = tuple(self.load(point + 4 * i, 4) for i in range(3))
            self.events.append(('position', world, truncate(index, 2), center, start,
                                tuple(self.memory[PACKET + i] for i in (0, 2, 3))))
            assert start == 8
            packet = CONTEXT + 26 if self.source else 0x4d1490
            for i in range(8):
                self.memory[self.mapped(packet + start + i)] = ('encoded-position-byte', i)
            self.store(writer + 8, 16, 4) if self.source else self.store(WRITER_CURSOR, packet + 16, 4)
            self.reg['EAX'] = 1
        elif target in ('_bh_network_send', '0x00476d50'):
            arguments = self.args(5 if self.source else 4)
            if self.source:
                assert arguments.pop(0) == TRANSPORT
            peer, packet, length, flags = arguments
            assert (peer, length, flags) == (0, 25, 0)
            self.events.append(('send', length, flags, tuple(self.memory[self.mapped(packet + i)] for i in range(28))))
            self.reg['EAX'], self.sent = 1, True
        elif target in ('_bh_actor_construct_room', '0x0043a920'):
            arguments = self.args(7 if self.source else 6)
            if self.source:
                assert arguments.pop(0) == ACTORS
            self.events.append(('construct', tuple(arguments)))
            self.reg['EAX'] = 0x60000
        elif target == '_memmove':
            destination, source, count = self.args(3)
            assert count == 9
            data = [self.load(source + i, 1) for i in range(count)]
            for i, byte in enumerate(data):
                self.store(destination + i, byte, 1)
            self.reg['EAX'] = destination
        else:
            raise AssertionError(('Unreviewed launch dependency', target))

    def observation(self):
        return self.events, self.rand_calls


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=Path, default=ROOT / 'build/game/bh_actor_launch.obj')
    parser.add_argument('--output-directory', type=Path, default=ROOT / 'build/ai-launch-match/graphs')
    args = parser.parse_args()
    out = args.output_directory
    out.mkdir(parents=True, exist_ok=True)
    image = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original = json.loads((ROOT / 'META/m4/target656/launch/original/0043bca0.disasm.json').read_text())
    assert b''.join(bytes.fromhex(r['bytes']) for r in original) == image_bytes(image, 0x43bca0, 533)
    original = [dict(address=int(r['address'], 16), mnemonic=r['mnemonic'], operands=', '.join(r['operands'])) for r in original]
    owner = Program(args.object, '_bh_actor_launch')
    helpers = [Program(ROOT / 'build/game/bh_network_codec.obj', name) for name in
               ('_bh_packet_begin', '_write16', '_bh_packet_append', '_space', '_bh_packet_finish', '_read32', '_write32')]
    helpers += [Program(args.object, '_current_tick'), Program(ROOT / 'build/game/bh_actor_route.obj', '_bh_actor_route_now')]
    programs = {p.name: p for p in [owner] + helpers}
    lemma = division_lemma()
    # The complete scan transition has no calls or publications. Given the
    # original ushort index j and a valid unique-world registry of length>j,
    # induction skips0..j-1 and selects j. No other node content is read.
    scan = [(r['mnemonic'], r['operands'].upper()) for r in owner.rows if 0x138 <= r['address'] <= 0x146]
    assert scan == [('MOV', 'EDX, DWORD PTR [EDX]'), ('CMP', 'DWORD PTR [EDX], ESI'),
                    ('JE', '0X148'), ('INC', 'EAX'), ('ADD', 'EDX, 0X10'),
                    ('CMP', 'EAX, ECX'), ('JB', '0X13A'), ('JMP', '0X154')]
    prefix = {r['address']: (r['mnemonic'], r['operands'].upper()) for r in owner.rows}
    assert prefix[0x12e] == ('XOR', 'EAX, EAX') and prefix[0x130] == ('TEST', 'ECX, ECX')
    assert prefix[0x148] == ('MOV', 'DWORD PTR [ESP + 0X14], EAX')
    assert [(at, prefix[at]) for at in (0xb, 0x3d, 0x41, 0x47, 0x49, 0x4c, 0x52, 0x55, 0x58, 0x5e, 0x61)] == [
        (0xb, ('XOR', 'EBP, EBP')), (0x3d, ('MOV', 'EAX, DWORD PTR [ESP + 0X44]')),
        (0x41, ('CMP', 'EAX, EBP')), (0x47, ('JE', '0X52')), (0x49, ('CMP', 'EAX, 0X3')),
        (0x4c, ('JNE', '0X24F')), (0x52, ('MOV', 'ECX, DWORD PTR [EDI + 0X14]')),
        (0x55, ('CMP', 'ECX, 0X3')), (0x58, ('JE', '0X27F')),
        (0x5e, ('CMP', 'ECX, 0X2')), (0x61, ('JNE', '0X24F'))]
    coverage, graphs = {}, []
    for mode in range(4):
        for kind in (0, 3, 1, 6, 0xffffffff):
            for world in (False, True):
                if not world and not (kind in (0, 3) and mode in (2, 3)):
                    continue
                for index in (0, 1, 2, 3):
                    for live in (False, True):
                        old = LaunchMachine(False, mode, kind, world, index, live, programs)
                        new = LaunchMachine(True, mode, kind, world, index, live, programs)
                        old.image = new.image = image
                        old.run(original, 'original', {})
                        new.run(owner.rows, owner.name, owner.calls)
                        a, b = old.observation(), new.observation()
                        if a != b:
                            (out / 'difference.json').write_text(json.dumps(dict(inputs=(mode, kind, world, index, live), original=a, source=b), indent=2) + '\n')
                            raise AssertionError(('Launch graph differs', mode, kind, world, index, live))
                        assert old.rand_calls == new.rand_calls == 2
                        assert new.load(CONTEXT + 96, 4) == add(('input32', 'sent'), int(new.sent))
                        assert new.load(CONTEXT + 108, 4) == 0
                        if new.sent:
                            assert new.load(CONTEXT + 104, 4) == 1
                        for machine in (old, new):
                            assert not machine.fp and machine.reg['ESP'] == machine.saved['ESP']
                            assert all(machine.reg[r] == machine.saved[r] for r in ('EBX', 'ESI', 'EDI', 'EBP'))
                            for name, covered in machine.covered.items():
                                coverage.setdefault(name, set()).update(covered)
                        graphs.append(digest(repr(a).encode()))
    missing = [r['address'] for r in original if r['address'] not in coverage['original']]
    assert not missing, missing
    source_missing = [r['address'] for r in owner.rows if r['address'] not in coverage[owner.name]]
    assert source_missing == [0x146, 0x23e, 0x245, 0x247, 0x248, 0x249, 0x24a, 0x24b, 0x24e, 0x274, 0x276, 0x278]
    result = dict(original_va='0043bca0', original_owner_bytes=533, source_owner_bytes=owner.end,
                  cases=len(graphs), all_original_instructions_covered=True,
                  dependency_boundary_graphs_equal=True, object_sha256=digest(owner.data),
                  division_lemma=lemma, graph_digest=digest(json.dumps(graphs).encode()),
                  source_uncovered=source_missing, all_valid_source_instructions_covered=True,
                  source_exclusion='Only failed scan/context/packet guard paths. Valid fixed28B writer and nonnull finite world make position-write/packet capacity failures unreachable.',
                  registry_scan_induction='Verified complete actual loop transition and initialization; unique valid world binding at original ushort index j;0<=j<=65535 and j<node_count. Prefix i<=j cannot overflow32-bit index. Valid node array/pointer arithmetic required.',
                  type_mode_partition='Actual owner tests only type==0/3 and mode==2/3; all other values follow their respective shared branch.',
                  helpers=[dict(name=p.name, object=str(p.path), sha256=digest(p.data),
                                uncovered=[r['address'] for r in p.rows if r['address'] not in coverage[p.name]]) for p in helpers],
                  limits=['Finite PC53 center arithmetic; commutative sum/product value graphs; no NaN payload/exception-status/PC24/64 claim.',
                          'Position codec, send, constructor and verified CRT rand/ftol are explicit dependency contracts.',
                          'Packet begin/append/finish and their private helpers execute actual compiled source instructions; memmove uses its standard byte-copy contract.',
                          'Source valid contexts/guards, world-index registry, clock API and diagnostics/results are explicit adapters.',
                          'No byte match or all-profile cosmetic-only claim.'])
    (out / 'graphs.json').write_text(json.dumps(result, indent=2) + '\n')
    (out / 'source.asm').write_text(owner.listing)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
