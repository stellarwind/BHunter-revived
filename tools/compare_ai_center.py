"""Static full-body portal-center close-match and linked-operand proof.

Never executes either image. The unequal arithmetic windows are reported in
full, rather than masked or counted as byte matches.
"""
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/astra-review'))
from targeted_probes import parse_coff


def image_bytes(image, va, count):
    pe = struct.unpack_from('<I', image, 0x3c)[0]
    optional = pe + 24
    base = struct.unpack_from('<I', image, optional + 28)[0]
    sections = optional + struct.unpack_from('<H', image, pe + 20)[0]
    for i in range(struct.unpack_from('<H', image, pe + 6)[0]):
        at = sections + i * 40
        rva, size, disk = struct.unpack_from('<III', image, at + 12)
        offset = va - base - rva
        if 0 <= offset and offset + count <= size:
            return image[disk + offset:disk + offset + count]
    raise ValueError(hex(va))


def main():
    original_image = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    assert hashlib.sha256(original_image).hexdigest() == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
    original = image_bytes(original_image, 0x433f70, 165)
    obj_path = ROOT / 'build/game/bh_route_query.obj'
    obj = obj_path.read_bytes()
    symbols, sections = parse_coff(obj)
    owner = next(s for s in symbols.values() if s['name'] == '_bh_actor_portal_center_core')
    assert owner['value'] == 0
    section = sections[owner['section']]
    data = section['data']
    normalized = bytearray(data[:166])
    assert len(data) == 208 and data[165] == 0xc3
    assert data[166:168] == bytes.fromhex('8bff')
    assert set(data[196:]) <= {0x90}
    relocs = section['relocations']
    assert [r['offset'] for r in relocs] == [58, 66, 96, 168, 172, 176, 180, 184, 188, 192]
    assert all(r['kind'] == 6 for r in relocs)
    exe_path = ROOT / 'build/game/BHunter-source.exe'
    exe = exe_path.read_bytes()
    map_text = (ROOT / 'build/game/BHunter-source.map').read_text()
    found = re.findall(r'^\s+\w+:\w+\s+_bh_actor_portal_center_core\s+([0-9a-fA-F]{8})\s', map_text, re.M)
    assert len(found) == 1
    linked_va = int(found[0], 16)
    linked = bytearray(image_bytes(exe, linked_va, 166))
    for relocation in relocs[:2]:
        offset, symbol = relocation['offset'], relocation['symbol']
        addend = struct.unpack_from('<I', data, offset)[0]
        constant = sections[symbol['section']]['data'][symbol['value'] + addend:symbol['value'] + addend + 4]
        assert constant == image_bytes(original_image, 0x489288, 4) == bytes.fromhex('0000003f')
        assert data[offset-2:offset] == bytes.fromhex('d80d')
        assert image_bytes(exe, struct.unpack_from('<I', linked, offset)[0], 4) == constant
        normalized[offset:offset+4] = struct.pack('<I', 0x489288)
        linked[offset:offset+4] = data[offset:offset+4]
    table_symbol = relocs[2]['symbol']
    assert table_symbol['section'] == owner['section'] and table_symbol['value'] == 168
    assert struct.unpack_from('<I', data, 96)[0] == 0
    table_va = struct.unpack_from('<I', linked, 96)[0]
    assert table_va == linked_va + 168 and data[93:96] == bytes.fromhex('ff2485')
    table = image_bytes(exe, table_va, 28)
    original_table = image_bytes(original_image, 0x434018, 28)
    targets = []
    for i, relocation in enumerate(relocs[3:]):
        symbol = relocation['symbol']
        assert symbol['section'] == owner['section']
        target = symbol['value'] + struct.unpack_from('<I', data, 168 + 4*i)[0]
        old = struct.unpack_from('<I', original_table, 4*i)[0] - 0x433f70
        assert target == old + 1 and 100 <= target < 166
        assert struct.unpack_from('<I', table, 4*i)[0] == linked_va + target
        targets.append(dict(index=i, original_relative=old, source_relative=target))
    linked[96:100] = data[96:100]
    assert linked == data[:166], 'Linked code differs from compared production object'
    normalized[96:100] = struct.pack('<I', 0x434018)
    assert normalized[:14] == original[:14]
    assert normalized[48:166] == original[47:165]
    # Unequal windows remain visible: six original instructions versus seven
    # source instructions. Both compute the two aperture sums without a spill.
    assert normalized[14:48].hex() == 'd984810c010000d9848110010000d9848114010000d9c9dec28d0481d88018010000'
    result = dict(original_va='00433f70', original_body_bytes=165, source_body_bytes=166,
        byte_match=False, classification='func/close', linked_matches_compared_object=True,
        identical_aligned_bytes=132, original_unequal_window=original[14:47].hex(),
        source_unequal_window=normalized[14:48].hex(),
        difference='x87 load/add scheduling and one extra instruction in the two aperture sums; no extra rounding store',
        entire_remaining_body_equal=True, all_seven_dispatch_targets_verified=targets,
        constants_verified=True, linked_va=hex(linked_va),
        object_sha256=hashlib.sha256(obj).hexdigest(), executable_sha256=hashlib.sha256(exe).hexdigest())
    output = ROOT / 'META/m4/playable/ai-byte-match/portal-center/linked-close.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('all_seven_dispatch_targets_verified', 'original_unequal_window', 'source_unequal_window')}))


if __name__ == '__main__':
    main()
