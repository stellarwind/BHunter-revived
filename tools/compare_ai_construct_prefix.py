"""Static constructor prefix and RNG primitive audit; NOT a whole-owner proof."""
import argparse
import json
import re
import struct
import subprocess
from functools import lru_cache
from pathlib import Path

from compare_ai_receive_graphs import Machine, packed
from compare_ai_launch_graphs import LaunchMachine, Program, rounded
from compare_ai_damage_query_math import ROOT, digest, image_bytes, owner_instructions, parse_coff
from compare_ai_damage_random_math import bv, helper

WORLD, ENTITY, ROOM, BINDING, CONTEXT, PREVIOUS = 0x30000, 0x40000, 0x50000, 0x60000, 0x70000, 0x80000


class Window:
    def __init__(self, path, start, stop):
        self.path, self.name = path, '_constructor-prefix'
        self.data = path.read_bytes()
        symbols, sections = parse_coff(self.data)
        owner = next(v for v in symbols.values() if v['name'] == '_construct')
        rows, self.listing, _, _ = owner_instructions(path, symbols, sections, owner['name'])
        self.rows = [r for r in rows if start <= r['address'] < stop]
        section = sections[owner['section']]
        assert b''.join(r['data'] for r in self.rows) == section['data'][start:stop]
        self.calls, self.constants, self.bindings = {}, {}, {}
        for relocation in section['relocations']:
            at, symbol = relocation['offset'], relocation['symbol']
            if not start <= at < stop:
                continue
            row = next(r for r in self.rows if r['address'] <= at < r['address'] + len(r['data']))
            if relocation['kind'] == 20:
                self.calls[row['address']] = symbol['name']
            else:
                assert relocation['kind'] == 6
                if not row['mnemonic'].startswith('F'):
                    assert symbol['name'] == '_draw_actor' and row['mnemonic'] == 'MOV'
                    self.bindings[row['address']] = ('callback', 'draw-actor')
                    continue
                offset = symbol['value'] + struct.unpack_from('<I', section['data'], at)[0]
                width = 8 if 'QWORD' in row['operands'].upper() else 4
                value = struct.unpack('<d' if width == 8 else '<f', sections[symbol['section']]['data'][offset:offset + width])[0]
                self.constants[row['address']] = ('constant', value.hex())
        self.rows.append(dict(address=stop, mnemonic='RET', operands='', data=b''))


class Prefix(Machine):
    def __init__(self, source, previous, programs, image):
        super().__init__(source, 0, True, True, True, False, programs)
        self.image = image
        self.memory.clear()
        self.reg['ESP'] = 0x90000
        if source:
            self.reg.update(EBX=ENTITY, ESI=WORLD, EDI=CONTEXT)
            for at in (0x90024, 0x90028, 0x9002c):
                self.store(at, 0, 4)
            self.store(0x90010, BINDING, 4)
            self.store(0x900bc, ('input32', 'seed'), 4)
            self.store(BINDING, ROOM, 4)
        else:
            self.reg.update(ESI=ENTITY, EBP=WORLD, EBX=0)
            self.store(0x90034, ('input32', 'seed'), 4)
        self.store(ROOM + 4, PREVIOUS if previous else 0, 4)
        for axis in range(3):
            self.store(WORLD + 0x1e8 + 4 * axis, ('float-input', 'minimum.' + str(axis)), 4)
            self.store(WORLD + 0x1f4 + 4 * axis, ('float-input', 'maximum.' + str(axis)), 4)
            self.store(0x495e30 + 4 * axis, 0, 4)
        for at in range(0x340):
            self.memory[ENTITY + at] = ('old-entity-byte', at)
        self.store(ENTITY, ('input32', 'flags'), 4)
        self.writes.clear()

    def mapped(self, at):
        if not self.source:
            if at == WORLD + 0x128:
                return ROOM + 4
            if WORLD + 0x1b0 <= at < WORLD + 0x1c8:
                return at + 0x38
        return at

    @staticmethod
    def width(operand):
        return 8 if 'QWORD' in operand else Machine.width(operand)

    def load(self, at, width):
        if width != 8:
            return super().load(at, width)
        parts = tuple(self.memory[self.mapped(at) + i] for i in range(8))
        if all(isinstance(x, tuple) and x[0] == 'slice' for x in parts):
            value = parts[0][1]
            if parts == tuple(('slice', value, i) for i in range(8)):
                return value
        return packed(parts)

    def floating(self, row, name, operand):
        return LaunchMachine.floating(self, row, name, operand)

    def instruction(self, row, name, operands):
        mnemonic = row['mnemonic']
        if mnemonic in ('FST', 'FSTP'):
            value = self.fp[0] if 'QWORD' in operands[0] else rounded(self.fp[0])
            self.write(operands[0], value)
            if mnemonic == 'FSTP':
                self.fp.pop(0)
            return True
        if mnemonic == 'OR' and operands[0] in ('AH', 'DH'):
            value = self.read(operands[0])
            assert operands[1] == '0X1' and value[0] == 'bytes' and len(value[1]) == 1
            self.write(operands[0], ('bytes', (('or-byte', value[1][0], 1),)))
            return True
        return LaunchMachine.instruction(self, row, name, operands)

    def observation(self):
        # Only this initializer prefix is normalized: source route-state scratch
        # is published later by the real route_write adapter, outside this proof.
        fields = {at: self.load(ENTITY + at, 4) for at in
                  [0, 0xc, 0x10, 0x14, 0x3c, 0x40, 0x44, 0xc4, 0xf4, 0xf8, 0x194] + list(range(0x15c, 0x174, 4))}
        if self.source:
            fields[0x1a4], fields[0x1a8] = self.load(0x90030, 4), self.load(0x90034, 4)
            for i in range(12):
                fields[0x120 + 4 * i] = self.load(0x90038 + 4 * i, 4)
            for i in range(3):
                fields[0x150 + 4 * i] = self.load(0x90068 + 4 * i, 4)
            fields[0x180], fields[0x184] = self.load(0x90074, 4), self.load(0x90078, 4)
            fields[0x1e8] = self.load(0x900bc, 4)
        else:
            for at in [0x1a4, 0x1a8, 0x180, 0x184, 0x1e8] + list(range(0x120, 0x15c, 4)):
                fields[at] = self.load(ENTITY + at, 4)
        return fields, self.load(ROOM + 4, 4), self.load(PREVIOUS + 0xf8, 4) if self.load(ENTITY + 0xf4, 4) else None


def inline_steps(rows):
    proofs = []
    @lru_cache(None)
    def value(register, before):
        for i in range(before - 1, -1, -1):
            row = rows[i]
            mnemonic, parts = row['mnemonic'], row['operands']
            if mnemonic.startswith('J') or mnemonic == 'CALL':
                return ('boundary-register', register, i)
            aliases = {'EAX': ('AX', 'AL', 'AH'), 'ECX': ('CX', 'CL', 'CH'),
                       'EDX': ('DX', 'DL', 'DH'), 'EBX': ('BX', 'BL', 'BH'),
                       'ESI': ('SI',), 'EDI': ('DI',), 'EBP': ('BP',)}
            if parts and parts[0] in aliases.get(register, ()) and mnemonic not in ('CMP', 'TEST', 'PUSH'):
                return ('partial-register-definition', register, i)
            if not parts or parts[0] != register:
                continue
            if mnemonic == 'MOV':
                return value(parts[1], i) if parts[1] in ('EAX', 'ECX', 'EDX', 'EDI', 'EBX', 'ESI', 'EBP') else ('memory-or-input', tuple(parts[1:]), i)
            if mnemonic == 'IMUL':
                return bv('mul', value(parts[1], i), int(parts[2], 0) & 0xffffffff)
            if mnemonic in ('SHR', 'AND', 'OR', 'XOR'):
                b = value(parts[1], i) if parts[1] in ('EAX', 'ECX', 'EDX', 'EDI', 'EBX', 'ESI', 'EBP') else int(parts[1], 0)
                return bv(mnemonic.lower(), value(register, i), b)
            return ('other-definition', register, i)
        return ('entry-register', register)
    for i, row in enumerate(rows):
        if row['mnemonic'] != 'XOR' or row['operands'][0] == row['operands'][1]:
            continue
        a, b = row['operands']
        seed = value(b, i)
        expression = bv('xor', value(a, i), seed)
        if expression == bv('step', seed, None):
            proofs.append(dict(address=row['address'], arbitrary_input_step_proved=True))
    assert len(proofs) == 11, ('Unexpected original inline RNG partition', len(proofs))
    return proofs


def query_publication(image, out):
    """Establish only that registering a query cannot invoke that query."""
    listing = subprocess.check_output(['llvm-objdump', '-d', '--x86-asm-syntax=intel',
        '--start-address=0x476e90', '--stop-address=0x476f1b', str(ROOT / 'GAME_DATA/BHUNTER.EXE')], text=True)
    original = []
    for line in listing.splitlines():
        match = re.match(r'\s*([0-9a-f]+):\s+((?:[0-9a-f]{2}\s+)+)(\w+)\s*(.*)', line)
        if match:
            address, data, mnemonic, operands = match.groups()
            original.append(dict(address=int(address, 16), data=bytes.fromhex(data), mnemonic=mnemonic.upper(), operands=operands))
    assert b''.join(r['data'] for r in original) == image_bytes(image, 0x476e90, 139)
    allowed = {'MOV', 'CMP', 'TEST', 'PUSH', 'POP', 'JE', 'JNE', 'JMP', 'RET'}
    assert {r['mnemonic'] for r in original} <= allowed
    assert all(re.match(r'0x[0-9a-f]+', r['operands']) for r in original if r['mnemonic'].startswith('J'))
    path = ROOT / 'build/game/bh_entities.obj'
    symbols, sections = parse_coff(path.read_bytes())
    pending, visited = ['_bh_entity_set_query'], {}
    while pending:
        name = pending.pop()
        if name in visited:
            continue
        symbol = next(v for v in symbols.values() if v['name'] == name)
        rows, _, end, _ = owner_instructions(path, symbols, sections, name)
        relocations = sections[symbol['section']]['relocations']
        calls = [r for r in rows if r['mnemonic'] == 'CALL']
        pure = {'MOV', 'LEA', 'CMP', 'TEST', 'PUSH', 'POP', 'RET', 'CALL',
                'JE', 'JNE', 'JA', 'JAE', 'JB', 'JBE', 'JMP', 'JZ', 'JNZ',
                'XOR', 'AND', 'OR', 'ADD', 'SUB', 'INC', 'DEC', 'NEG', 'SBB',
                'MUL', 'IMUL', 'DIV', 'SHR', 'SHL', 'SAR', 'SETE', 'SETNE'}
        assert {r['mnemonic'] for r in rows} <= pure, name
        assert all(r['kind'] == 20 and r['symbol']['section'] > 0 for r in relocations)
        assert len(calls) == len(relocations)
        for call in calls:
            matches = [r for r in relocations if r['offset'] == call['address'] + 1]
            assert len(matches) == 1 and call['data'][0] == 0xe8
            pending.append(matches[0]['symbol']['name'])
        assert all(re.match(r'0x[0-9a-f]+', r['operands']) and int(r['operands'], 0) in {v['address'] for v in rows}
                   for r in rows if r['mnemonic'].startswith('J'))
        visited[name] = dict(bytes=end, calls=[r['symbol']['name'] for r in relocations])
    assert set(visited) == {'_bh_entity_set_query', '_active', '_membership', '_fail'}
    (out / 'original-query-setter.asm').write_text(listing)
    return dict(original_query_setter_va='00476e90', original_bytes=139,
                original_has_no_calls_or_indirect_branches=True, source_call_graph=visited,
                source_object_sha256=digest(path.read_bytes()), source_has_no_external_calls_or_indirect_branches=True,
                scope='Neither setter invokes registered callbacks or external functions. This does not prove the complete list adapter or constructor; publication-order equivalence still requires complete caller composition and stable disjoint lifetimes.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=Path, default=ROOT / 'build/game/bh_actor_construct.obj')
    parser.add_argument('--output-directory', type=Path, default=ROOT / 'build/ai-construct-match/prefix-audit')
    args = parser.parse_args()
    args.output_directory.mkdir(parents=True, exist_ok=True)
    image = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    assert image_bytes(image, 0x495e30, 12) == bytes(12)
    source = Window(args.object, 0x141, 0x2a6)
    helpers = [Program(args.object, name) for name in ('_rf', '_word', '_real')]
    helpers.append(Program(ROOT / 'build/game/bh_round.obj', '_bh_round_float'))
    programs = {p.name: p for p in [source] + helpers}
    symbols, sections = parse_coff(args.object.read_bytes())
    rng, _, _, _ = owner_instructions(args.object, symbols, sections, '_next_random')
    rng_proof = helper(rng)
    records, source_coverage = [], set()
    for owner, length, start, stop in [(0x43a920, 2443, 0x43a987, 0x43ab12), (0x43b2d0, 2483, 0x43b359, 0x43b4e4)]:
        rows = json.loads((ROOT / f'META/m4/target656/construct/original/{owner:08x}.disasm.json').read_text())
        assert b''.join(bytes.fromhex(r['bytes']) for r in rows) == image_bytes(image, owner, length)
        original_steps = inline_steps(rows)
        selected = [r for r in rows if start <= int(r['address'], 16) < stop]
        assert b''.join(bytes.fromhex(r['bytes']) for r in selected) == image_bytes(image, start, stop - start)
        original = [dict(address=int(r['address'], 16), mnemonic=r['mnemonic'], operands=', '.join(r['operands'])) for r in selected]
        original.append(dict(address=stop, mnemonic='RET', operands=''))
        paths = []
        for previous in (False, True):
            old, new = Prefix(False, previous, programs, image), Prefix(True, previous, programs, image)
            old.run(original, 'original-prefix', {})
            new.run(source.rows, source.name, source.calls)
            a, b = old.observation(), new.observation()
            if a != b:
                (args.output_directory / 'difference.json').write_text(json.dumps(dict(owner=hex(owner), previous=previous, original=a, source=b), indent=2) + '\n')
                raise AssertionError(('Constructor prefix graph differs', hex(owner), previous))
            assert not old.fp and not new.fp and old.reg['ESP'] == new.reg['ESP'] == 0x90000
            assert old.covered['original-prefix'] == {r['address'] for r in original} - ({0x43aa43 if owner == 0x43a920 else 0x43b415} if not previous else set())
            source_coverage.update(new.covered[source.name])
            for p in helpers:
                assert new.covered[p.name] == {r['address'] for r in p.rows}
            paths.append(dict(previous=previous, graph_sha256=digest(repr(a).encode())))
        records.append(dict(original_va=f'{owner:08x}', original_prefix_start=hex(start), original_prefix_end_exclusive=hex(stop), original_prefix_bytes=stop - start,
                            complete_prefix_graphs_equal=True, paths=paths, inline_rng_primitive_proofs=original_steps))
    assert source_coverage == {r['address'] for r in source.rows}
    result = dict(whole_owner_graph_proved=False, byte_match=False, implementation='func', matching='wip',
                  object_sha256=digest(args.object.read_bytes()), source_prefix_start='0x141', source_prefix_end_exclusive='0x2a6',
                  all_original_and_source_prefix_instructions_covered=True,
                  rf_round_word_real_actual_helper_instructions_covered=True,
                  source_rng_primitive=rng_proof, owners=records,
                  query_publication_audit=query_publication(image, args.output_directory),
                  limits=['Prefix and per-step RNG primitive audit only: complete RNG consumption/branch/resource/query/return composition remains unproved.',
                          'Finite nearest masked PC53 value graphs; binary64 scratch stores are identity, f32 publications retained.',
                          'Source route scratch is a named field-view adapter; actual later publication is outside this prefix proof.',
                          'Original image zero-origin bytes verified; runtime zero-origin invariant and valid disjoint room/entity bindings explicit. No unrestricted aliases/reentrancy/exception-status claim.'])
    (args.output_directory / 'graphs.json').write_text(json.dumps(result, indent=2) + '\n')
    (args.output_directory / 'source.asm').write_text(source.listing)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
