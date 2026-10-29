"""Read-only COFF/PE binding audit; executes no game or original instructions."""
import argparse
import json
import re
import struct
from pathlib import Path

from compare_ai_damage_query_math import ROOT, digest, image_bytes, parse_coff


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('objects', nargs='+', type=Path)
    parser.add_argument('--root', action='append', default=[], help='Limit public roots; omitted means every public owner')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--executable', type=Path, default=ROOT / 'build/game/BHunter-source.exe')
    parser.add_argument('--map', type=Path, default=ROOT / 'build/game/BHunter-source.map')
    args = parser.parse_args()
    executable = args.executable
    image = executable.read_bytes()
    mapping = args.map.read_text()
    addresses = {}
    for name, address in re.findall(r'^\s+\w+:\w+\s+(\S+)\s+([0-9a-fA-F]{8})\s', mapping, re.M):
        addresses.setdefault(name, set()).add(int(address, 16))
    results, found_roots = [], set()
    for path in args.objects:
        obj = path.read_bytes()
        symbols, sections = parse_coff(obj)
        visited, bindings = {}, []

        def verify(index, base):
            if index in visited:
                assert visited[index] == base, ('Inconsistent section binding', path, index)
                return
            visited[index] = base
            section = sections[index]
            original = section['data']
            if not original:
                return
            linked = bytearray(image_bytes(image, base, len(original)))
            for relocation in section['relocations']:
                at, target = relocation['offset'], relocation['symbol']
                addend = struct.unpack_from('<I', original, at)[0]
                actual = struct.unpack_from('<I', linked, at)[0]
                if relocation['kind'] == 20:
                    actual = (base + at + 4 + actual) & 0xffffffff
                else:
                    assert relocation['kind'] == 6, relocation
                symbol_address = (actual - addend) & 0xffffffff
                assert target['storage'] in (2, 3, 6), target
                if target['storage'] == 2:
                    assert addresses.get(target['name']) == {symbol_address}, ('Wrong external binding', target['name'])
                if target['section'] > 0:
                    verify(target['section'], symbol_address - target['value'])
                else:
                    assert target['section'] == 0, ('Unsupported absolute symbol', target)
                bindings.append(dict(section=index, offset=at, symbol=target['name'], address=hex(symbol_address)))
                linked[at:at + 4] = original[at:at + 4]
            assert linked == original, ('Linked section differs', path, index)

        roots = []
        for symbol in symbols.values():
            if symbol['type'] == 32 and symbol['storage'] == 2 and symbol['section'] > 0:
                if args.root and symbol['name'] not in args.root:
                    continue
                locations = addresses.get(symbol['name'], set())
                assert len(locations) == 1, ('Missing/nonunique root', symbol['name'])
                verify(symbol['section'], next(iter(locations)) - symbol['value'])
                roots.append(symbol['name'])
                found_roots.add(symbol['name'])
        assert roots, ('No public function roots', path)
        results.append(dict(object=str(path), object_sha256=digest(obj), roots=roots,
                            verified_sections=len(visited), bindings=bindings))
    assert not args.root or found_roots == set(args.root), ('Unresolved selected roots', set(args.root) - found_roots)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(static_only=True, original_equivalence_claim=False,
        executable_sha256=digest(image), objects=results), indent=2) + '\n')
    print(f'{len(results)} production objects: public roots, reached private sections and real bindings verified')


if __name__ == '__main__':
    main()
