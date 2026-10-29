"""Actual constructor graphs through class remapping, before weapon switch."""
import argparse
import json
from pathlib import Path

from compare_ai_construct_common import Common, Window, Program, ROOT, digest, image_bytes, RESPONSE, MODELS, SOUNDS
from compare_ai_construct_prefix import CONTEXT

CYCLES, PLAYER = 0x83000, 0x84000


class Classes(Common):
    def __init__(self, source, settings, programs, image, kind):
        super().__init__(source, *settings, programs, image, kind=kind)
        self.store(CONTEXT + 36, CYCLES, 4)
        self.store(CONTEXT + 44, ('input32', 'cycle'), 4)
        self.store(CONTEXT + 48, ('resource', 'special-model'), 4)
        self.store(RESPONSE + 16, PLAYER, 4)

    def load(self, at, width):
        bases = {MODELS: ('model', 25), SOUNDS: ('sound', 25), CYCLES: ('cycle-model', 4),
                 0x4d1920: ('model', 25), 0x4d1a00: ('sound', 25), 0x496578: ('cycle-model', 4)}
        if isinstance(at, tuple):
            assert width == 4 and at[0] == 'add'
            offset, base = at[1], at[2]
            assert base in bases and offset[:2] == ('mul', 4)
            return ('resource', bases[base][0], offset[2])
        if not self.source and at == 0x4dfab8:
            return PLAYER
        if not self.source and at == 0x4dfb30:
            return super().load(CONTEXT + 44, width)
        if not self.source and at == 0x4d047c:
            return super().load(CONTEXT + 48, width)
        for base, (kind, count) in bases.items():
            if base <= at < base + count * 4:
                assert width == 4 and (at - base) % 4 == 0
                return ('resource', kind, (at - base) // 4)
        return super().load(at, width)

    def mapped(self, at):
        if not self.source and at == 0x4dfb30:
            return CONTEXT + 44
        return super().mapped(at)

    def instruction(self, row, name, operands):
        if row['mnemonic'] == 'CMP':
            a, b = map(self.read, operands)
            if a == ('other-class',):
                assert b in (3, 7, 10, 11, 12, 13, 14, 15)
                self.zero, self.carry = False, False
                return True
        return super().instruction(row, name, operands)

    def observation(self):
        return super().observation(), self.load(CONTEXT + 44, 4), self.reg['EBP' if self.source else 'EDI']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--object', type=Path, default=ROOT / 'build/game/bh_actor_construct.obj')
    p.add_argument('--output-directory', type=Path, default=ROOT / 'build/ai-construct-match/class-audit')
    args = p.parse_args()
    args.output_directory.mkdir(parents=True, exist_ok=True)
    image = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert digest(image) == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    source = Window(args.object, 0x141, 0x696)
    helpers = [Program(args.object, name) for name in ('_rf', '_word', '_real', '_next_random')]
    helpers += [Program(ROOT / 'build/game/bh_round.obj', '_bh_round_float'),
                Program(ROOT / 'build/game/bh_actor_route.obj', '_bh_actor_route_now')]
    programs = {v.name: v for v in [source] + helpers}
    results, source_coverage = [], set()
    # Other-class is a single arbitrary32 word excluding all named/remapped
    # classes. Its bits remain symbolic in stored fields; predicates are false
    # by that partition's definition, not by a sampled numeric representative.
    kinds = [1, 2, 3, 6, 7, 10, 11, 12, 13, 14, 15, ('other-class',)]
    for owner, length, start, stop in ((0x43a920, 2443, 0x43a987, 0x43ae3e), (0x43b2d0, 2483, 0x43b359, 0x43b816)):
        rows = json.loads((ROOT / f'META/m4/target656/construct/original/{owner:08x}.disasm.json').read_text())
        assert b''.join(bytes.fromhex(r['bytes']) for r in rows) == image_bytes(image, owner, length)
        selected = [r for r in rows if start <= int(r['address'], 16) < stop]
        assert b''.join(bytes.fromhex(r['bytes']) for r in selected) == image_bytes(image, start, stop - start)
        original = [dict(address=int(r['address'], 16), mnemonic=r['mnemonic'], operands=', '.join(r['operands'])) for r in selected]
        original.append(dict(address=stop, mnemonic='RET', operands=''))
        paths, coverage = [], set()
        for kind in kinds:
            for speed_zero in (False, True):
                for token_zero in (False, True):
                    for raw_index in range(23):
                        settings = True, speed_zero, token_zero, raw_index, True
                        old = Classes(False, settings, programs, image, kind)
                        new = Classes(True, settings, programs, image, kind)
                        old.run(original, 'original-classes', {})
                        new.run(source.rows, source.name, source.calls)
                        a, b = old.observation(), new.observation()
                        if a != b:
                            (args.output_directory / 'difference.json').write_text(json.dumps(dict(owner=hex(owner), kind=kind, inputs=settings, original=a, source=b), indent=2) + '\n')
                            raise AssertionError(('Constructor class graph differs', hex(owner), kind, settings))
                        assert not old.fp and not new.fp and old.reg['ESP'] == new.reg['ESP'] == 0x90000
                        coverage.update(old.covered['original-classes'])
                        source_coverage.update(new.covered[source.name])
                        paths.append(digest(repr(a).encode()))
        missing = [r['address'] for r in original if r['address'] not in coverage]
        assert not missing, (hex(owner), missing)
        results.append(dict(original_va=f'{owner:08x}', original_region_bytes=stop - start, graph_paths=len(paths),
                            graph_digest=digest(json.dumps(paths).encode()), uncovered=missing))
    source_missing = [r['address'] for r in source.rows if r['address'] not in source_coverage]
    assert not source_missing, source_missing
    result = dict(whole_owner_graph_proved=False, byte_match=False, implementation='func', matching='wip',
                  object_sha256=digest(args.object.read_bytes()), source_start='0x141', source_end_exclusive='0x696',
                  owners=results, source_uncovered=source_missing, all_original_source_region_instructions_covered=True,
                  limits=['Class remaps/resource/player/cycle publication only; weapon/switch/route/query/return composition remains open.',
                          'Shared common-stage contracts apply; empty room-head and snapshot clock branches are already covered by that separate actual-instruction proof.',
                          'Raw4/5 types are normalized to6 before this region; normalized other-class is an arbitrary word outside the listed classes.',
                          'Finite masked nearest PC53 graphs; stable typed resource/room/context adapters, no unrestricted alias/reentrancy claim.'])
    (args.output_directory / 'graphs.json').write_text(json.dumps(result, indent=2) + '\n')
    (args.output_directory / 'source.asm').write_text(source.listing)
    print(json.dumps(result))


if __name__ == '__main__':
    main()
