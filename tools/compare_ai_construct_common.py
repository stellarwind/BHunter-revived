"""Static constructor graphs through common model selection; not whole owners."""
import argparse
import json
import re
import struct
from pathlib import Path

from compare_ai_construct_prefix import Prefix, Window, WORLD, ENTITY, ROOM, BINDING, CONTEXT
from compare_ai_launch_graphs import Program, rounded
from compare_ai_receive_graphs import add, multiply, packed
from compare_ai_damage_random_math import bv
from compare_ai_damage_query_math import ROOT, digest, image_bytes

RESPONSE, DAMAGE, ROUTE, CLOCK, MODELS, SOUNDS = 0x71000, 0x72000, 0x73000, 0x74000, 0x81000, 0x82000


def word(value):
    if isinstance(value, int):
        return value & 65535
    if value[0] == 'add':
        return ('add16', word(value[1]), value[2] & 65535)
    if value[0] == 'bytes':
        parts = value[1]
        if len(parts) == 4 and parts[2:] == (0, 0):
            return word(packed(parts[:2]))
        if len(parts) == 2 and all(isinstance(p, tuple) and p[0] == 'slice' for p in parts):
            base = parts[0][1]
            if parts == (('slice', base, 0), ('slice', base, 1)):
                return word(base)
    return ('word16', value)


class Common(Prefix):
    def __init__(self, source, previous, zero_speed, zero_token, raw_index, live, programs, image, kind=('input32', 'normalized-class')):
        super().__init__(source, previous, programs, image)
        self.zero_speed, self.raw_index = zero_speed, raw_index
        self.index_expression, self.index_comparisons, self.fp_compare = None, [], None
        self.query_events = []
        speed = ('constant', float(0).hex()) if zero_speed else ('float-input', 'speed')
        token = 0 if zero_token else ('input32-nonzero', 'token')
        tag = ('input32', 'death-tag')
        self.store(CLOCK, ('input32', 'clock'), 4)
        self.store(ROUTE + 24, CLOCK if live else 0, 4)
        self.store(ROUTE + 16, self.load(CLOCK, 2), 2)
        self.store(BINDING + 4, ('input32', 'bound-resource'), 4)
        self.store(WORLD + 0x11c, ('input32', 'bound-resource'), 4)
        if source:
            self.reg['EBP'] = kind
            for at, value in ((CONTEXT, RESPONSE), (RESPONSE, DAMAGE), (DAMAGE + 4, ROUTE),
                              (CONTEXT + 24, MODELS), (CONTEXT + 28, SOUNDS),
                              (0x90020, RESPONSE), (0x900a4, CONTEXT), (0x900b0, token),
                              (0x900b4, speed), (0x900b8, tag)):
                self.store(at, value, 4)
        else:
            self.reg['EDI'] = kind
            for at, value in ((0x90028, token), (0x9002c, speed), (0x90030, tag)):
                self.store(at, value, 4)

    def mapped(self, at):
        if not self.source and at == 0x4dfac4:
            return CLOCK
        return super().mapped(at)

    def load(self, at, width):
        if isinstance(at, tuple):
            assert width == 4 and at[0] == 'add'
            offset, base = at[1], at[2]
            kinds = {MODELS: 'model', SOUNDS: 'sound', 0x4d1920: 'model', 0x4d1a00: 'sound'}
            assert base in kinds and offset[:2] == ('mul', 4), ('Unreviewed indexed resource', at)
            return ('resource', kinds[base], offset[2])
        return super().load(at, width)

    def floating(self, row, name, operand):
        if not self.source and re.search(r'\[0x[0-9a-f]+\]', operand.lower()):
            address = int(re.search(r'\[0x([0-9a-f]+)\]', operand.lower())[1], 16)
            width = 8 if 'DOUBLE' in operand else 4
            return ('constant', struct.unpack('<d' if width == 8 else '<f', image_bytes(self.image, address, width))[0].hex())
        return super().floating(row, name, operand)

    def index_value(self, value):
        if value == self.index_expression:
            return self.raw_index
        assert value[0] == 'add' and value[1] == self.index_expression
        return self.raw_index + value[2]

    def instruction(self, row, name, operands):
        m = row['mnemonic']
        if self.source and row['address'] in getattr(self.programs[name], 'bindings', {}):
            self.write(operands[0], self.programs[name].bindings[row['address']])
            return True
        if m == 'IMUL':
            assert len(operands) == 3
            self.write(operands[0], bv('mul', self.read(operands[1]), self.read(operands[2])))
            return True
        if m in ('AND', 'OR', 'XOR') and operands[0].startswith('E'):
            a, b = map(self.read, operands)
            if m == 'XOR' and a == b:
                value = 0
            elif m == 'OR' and (a == 0xffffffff or b == 0xffffffff):
                value = 0xffffffff
            else:
                value = bv(m.lower(), a, b)
            self.write(operands[0], value)
            self.zero, self.carry = value == 0, False
            return True
        if m == 'SHR':
            value, count = self.read(operands[0]), self.read(operands[1])
            result = bv('shr', value, count)
            self.write(operands[0], result)
            if count == 15:
                assert value[0:2] == ('mul', 23)
                assert value[2][0] == 'and32' and 32767 in value[2][1:]
                assert 23 * 32767 >> 15 == 22
                self.index_expression = result
            return True
        if m == 'FILD':
            assert self.width(operands[0]) == 8
            address = self.address(operands[0])
            low, high = self.load(address, 4), self.load(address + 4, 4)
            assert high == 0 and low[0] == 'and32' and 32767 in low[1:]
            self.fp.insert(0, ('unsigned15-to-float', low))
            return True
        if m in ('FSUB', 'FSUBR'):
            a, b = self.fp[0], self.floating(row, name, operands[0])
            self.fp[0] = ('sub-PC53', b, a) if m == 'FSUBR' else ('sub-PC53', a, b)
            return True
        if m in ('FCOM', 'FCOMP'):
            a, b = self.fp[0], self.floating(row, name, operands[0])
            assert b == ('constant', float(0).hex())
            assert a == (('constant', float(0).hex()) if self.zero_speed else ('float-input', 'speed'))
            self.fp_compare = ('speed-equal-zero', a, b)
            if m == 'FCOMP':
                self.fp.pop(0)
            return True
        if m == 'FNSTSW':
            assert operands == ['AX'] and self.fp_compare is not None
            self.reg_write('AX', ('x87-status', self.fp_compare))
            return True
        if m == 'TEST' and operands == ['AH', '0X40']:
            assert self.fp_compare is not None
            self.zero, self.carry = not self.zero_speed, False
            return True
        if m == 'FSTP' and operands == ['ST(0)']:
            assert self.zero_speed
            self.fp.pop(0)
            return True
        if m == 'CMP':
            a, b = map(self.read, operands)
            if a == ('input32', 'normalized-class'):
                # Only the final original class comparison occurs here. Its
                # flags are not consumed before this region's boundary. The
                # later class stage must execute it with its actual class.
                assert not self.source and row['address'] in (0x43acf3, 0x43b6c5) and b in (7, 12)
                self.pending_class_predicate = ('class-equals', a, b)
                return True
            if isinstance(a, tuple) and a[0] == 'input32-nonzero' and b == 0:
                self.zero, self.carry = False, False
                return True
            if isinstance(a, tuple) and b in (7, 8, 14, 15):
                concrete = self.index_value(a)
                self.zero, self.carry = concrete == b, concrete < b
                self.signed_less = concrete < b
                threshold = b + 1 if self.source else b
                self.index_comparisons.append(('model-index-less-than', a, threshold))
                return True
        if m == 'JL':
            # The model index is nonnegative and at most24 after skipping
            # resource holes, so signed<8/15 equals unsigned<=7/14.
            assert not self.source and self.index_expression is not None
            assert isinstance(self.signed_less, bool)
            return False
        return super().instruction(row, name, operands)

    def dependency(self, target):
        assert not self.source and target == '0x00476e90'
        entity, callback = self.args(2)
        assert (entity, callback) == (ENTITY, 0x435e80)
        self.query_events.append(('query-install', entity, callback))
        # Pure list publication, independently audited. Volatile returns are
        # deliberately destroyed: the caller must reload every value it uses.
        self.store(ENTITY + 0xd0, callback, 4)
        self.reg.update(EAX=('call-clobber', 'eax'), ECX=('call-clobber', 'ecx'), EDX=('call-clobber', 'edx'))

    def observation(self):
        fields, head, back = super().observation()
        for at in (0x100, 0x11c, 0x19c, 0x1ac, 0x1b4, 0x1b8, 0x110, 0xfc, 0x118, 0x10c, 0x114):
            fields[at] = self.load(ENTITY + at, 4)
        if self.source:
            for target, local in ((0x188, 0x7c), (0x18c, 0x80), (0x190, 0x84), (0x198, 0x88),
                                  (0x1b0, 0x8c), (0x104, 0x90), (0x108, 0x94)):
                fields[target] = self.load(0x90000 + local, 4)
            fields[0x1e8] = self.load(0x90010, 4)
            fields[0x1e4], fields[0x1e6] = word(self.load(0x9009c, 2)), word(self.load(0x9009e, 2))
            draw = self.load(ENTITY + 0xe8, 4)
        else:
            for at in (0x188, 0x18c, 0x190, 0x198, 0x1b0, 0x104, 0x108):
                fields[at] = self.load(ENTITY + at, 4)
            fields[0x1e4], fields[0x1e6] = word(self.load(ENTITY + 0x1e4, 2)), word(self.load(ENTITY + 0x1e6, 2))
            assert self.load(ENTITY + 0xe8, 4) == 0x434920
            draw = ('callback', 'draw-actor')
        assert draw == ('callback', 'draw-actor')
        return fields, head, back, draw, self.fp_compare, self.index_comparisons


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--object', type=Path, default=ROOT / 'build/game/bh_actor_construct.obj')
    p.add_argument('--output-directory', type=Path, default=ROOT / 'build/ai-construct-match/common-audit')
    args = p.parse_args()
    args.output_directory.mkdir(parents=True, exist_ok=True)
    image = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    source = Window(args.object, 0x141, 0x4eb)
    helpers = [Program(args.object, name) for name in ('_rf', '_word', '_real', '_next_random')]
    helpers += [Program(ROOT / 'build/game/bh_round.obj', '_bh_round_float'),
                Program(ROOT / 'build/game/bh_actor_route.obj', '_bh_actor_route_now')]
    programs = {v.name: v for v in [source] + helpers}
    results, source_coverage = [], set()
    for owner, length, start, stop in ((0x43a920, 2443, 0x43a987, 0x43ad02), (0x43b2d0, 2483, 0x43b359, 0x43b6d4)):
        rows = json.loads((ROOT / f'META/m4/target656/construct/original/{owner:08x}.disasm.json').read_text())
        assert b''.join(bytes.fromhex(r['bytes']) for r in rows) == image_bytes(image, owner, length)
        selected = [r for r in rows if start <= int(r['address'], 16) < stop]
        assert b''.join(bytes.fromhex(r['bytes']) for r in selected) == image_bytes(image, start, stop - start)
        original = [dict(address=int(r['address'], 16), mnemonic=r['mnemonic'], operands=', '.join(r['operands'])) for r in selected]
        original.append(dict(address=stop, mnemonic='RET', operands=''))
        coverage, paths = set(), []
        for previous in (False, True):
            for zero_speed in (False, True):
                for zero_token in (False, True):
                    for raw_index in range(23):
                        for live in (False, True):
                            settings = previous, zero_speed, zero_token, raw_index, live
                            old = Common(False, *settings, programs, image)
                            new = Common(True, *settings, programs, image)
                            old.run(original, 'original-common', {})
                            new.run(source.rows, source.name, source.calls)
                            a, b = old.observation(), new.observation()
                            if a != b:
                                (args.output_directory / 'difference.json').write_text(json.dumps(dict(owner=hex(owner), inputs=settings, original=a, source=b), indent=2) + '\n')
                                raise AssertionError(('Constructor common graph differs', hex(owner), settings))
                            assert not old.fp and not new.fp and old.reg['ESP'] == new.reg['ESP'] == 0x90000
                            assert len(old.query_events) == 1
                            coverage.update(old.covered['original-common'])
                            source_coverage.update(new.covered[source.name])
                            paths.append(digest(repr(a).encode()))
        assert coverage == {r['address'] for r in original}
        results.append(dict(original_va=f'{owner:08x}', original_start=hex(start), original_end_exclusive=hex(stop),
                            original_region_bytes=stop - start, graph_paths=len(paths), graph_digest=digest(json.dumps(paths).encode()),
                            all_region_instructions_covered=True))
    assert source_coverage == {r['address'] for r in source.rows}
    result = dict(whole_owner_graph_proved=False, byte_match=False, implementation='func', matching='wip',
                  object_sha256=digest(args.object.read_bytes()), source_start='0x141', source_end_exclusive='0x4eb',
                  all_original_source_region_instructions_covered=True, owners=results,
                  limits=['Common initializer through model lookup only; class/weapon/query/publication/return composition remains unproved.',
                          'Masked nearest PC53 finite arithmetic; original zero-origin and disjoint stable typed bindings remain explicit.',
                          'All23 raw model indices are compared as branch partitions of the same arbitrary-seed expression; no sampled-seed fidelity claim.',
                          'Source route scratch and deferred query/step publication are explicit boundary adapters, not proofs of later route_write/bind behavior.',
                          'Only x87 C3 from speed comparison is observed; exception flags/payloads/other precision profiles unproved.'])
    (args.output_directory / 'graphs.json').write_text(json.dumps(result, indent=2) + '\n')
    (args.output_directory / 'source.asm').write_text(source.listing)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
