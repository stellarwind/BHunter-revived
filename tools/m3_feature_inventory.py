"""Derive exposed features and loaded bindings from original data/evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--write-report', action='store_true')
    args = ap.parse_args()
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    manifest = json.loads((ROOT / 'META/reference-manifest.json').read_text())
    assert hashlib.sha256(exe).hexdigest() == next(x['sha256'] for x in manifest['files'] if x['path'] == 'GAME_DATA/BHUNTER.EXE')
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    table = pe + 24 + struct.unpack_from('<H', exe, pe + 20)[0]
    count = struct.unpack_from('<H', exe, pe + 6)[0]
    base = struct.unpack_from('<I', exe, pe + 52)[0]
    def raw(va, size):
        for i in range(count):
            _, start, length, offset = struct.unpack_from('<IIII', exe, table + i * 40 + 8)
            if start <= va - base and va - base + size <= start + length:
                return exe[offset + va - base - start:offset + va - base - start + size]
        raise ValueError(hex(va))
    def original_label(table_address, value):
        address = struct.unpack('<I', raw(table_address + value * 4, 4))[0]
        data = bytearray()
        for i in range(128):
            c = raw(address + i, 1)[0]
            if not c:
                return data.decode('latin1')
            data.append(c)
        raise ValueError('Unterminated original control label')
    menu_path = ROOT / 'META/m3/menu-inventory.json'
    menu = json.loads(menu_path.read_text())
    source = ROOT / menu['source_report']
    assert sha(source) == menu['source_report_sha256']
    menus = {e['global_slot']: e for e in menu['menus'] if e['phase'] == 'bootstrap-return'}
    assert len(menus) == 18 and sum(len(m['items']) for m in menus.values()) == 118
    reload_path = ROOT / 'META/m3/startup/c42a7051b694.json'
    reload = json.loads(reload_path.read_text())
    memory = next(e for e in reload['events'] if e['event'] == 'loaded-config-memory')
    profiles = [dict(name=bytes.fromhex(p['record_hex'])[12:28].split(b'\0', 1)[0].decode('ascii'), data=bytes.fromhex(p['record_hex'])) for p in memory['profiles']]
    pages = [(0x4d0484, '0042d380', list(range(0x1c, 0x28))), (0x4cdb98, '0042d550', list(range(0x28, 0x31))), (0x4d4408, '0042d620', list(range(0x31, 0x3a))), (0x496344, '0042d6f0', list(range(0x3a, 0x44)) + [0x45, 0x44, 0x47, 0x46])]
    controls = []
    for slot, callback, offsets in pages:
        body = (ROOT / ('META/m3/control-review/' + callback + '.c.txt')).read_text()
        cases = re.split(r'\bcase\s+(0x[0-9a-f]+|\d+):', body)
        extracted = {}
        for i in range(1, len(cases), 2):
            match = re.search(r'DAT_004df240\s*\+\s*(0x[0-9a-f]+)', cases[i + 1])
            assert match
            extracted[int(cases[i], 0)] = int(match[1], 0)
        assert [extracted[i + 1] for i in range(len(offsets))] == offsets
        for i, offset in enumerate(offsets):
            item = menus[slot]['items'][i + 1]
            assert item['edit_callback'] == int(callback, 16)
            kind = 'joystick_axis' if offset in (0x22, 0x23, 0x24, 0x44, 0x45) else 'mouse_axis' if offset in (0x25, 0x26, 0x27, 0x46, 0x47) else 'key_or_button'
            address = {'joystick_axis': 0x492110, 'mouse_axis': 0x492158, 'key_or_button': 0x491d10}[kind]
            bindings = [dict(profile=p['name'], value=p['data'][offset], original_display_label=original_label(address, p['data'][offset])) for p in profiles]
            controls.append(dict(menu_slot=f'{slot:08x}', item_index=i + 1, label=item['label'].replace('\n', ' '), profile_offset=offset, kind=kind, bindings=bindings, gameplay_validation='acceleration captured' if offset == 0x20 else 'not separately exercised'))
    assert len(controls) == 44 and {c['profile_offset'] for c in controls} == set(range(0x1c, 0x48))
    owned = {e['entity']: f'{slot:08x}' for slot, e in menus.items()}
    exposed = [dict(slot=f'{slot:08x}', items=[dict(label=x['label'].replace('\n', ' '), target_slot=owned.get(x['target_menu']), action_callback=f'{x["action_callback"]:08x}', edit_callback=f'{x["edit_callback"]:08x}') for x in e['items']]) for slot, e in menus.items()]
    installer = ROOT / 'META/m3/play-review/00421740.c.txt'
    installer_body = installer.read_text()
    for code, key in [('0x21', '0x1b'), ('0x20', '9'), ('0x22', '0x59'), ('0x23', '0x4e')]:
        assert re.search(r',' + code + r',&DAT_004dfb5c,0,0,' + key + r'\);', installer_body)
    evidence_paths = [Path(__file__), menu_path, source, reload_path, installer, ROOT / 'META/m3/play-review/00421740.asm.txt', ROOT / 'GAME_DATA/BHUNTER.EXE'] + sorted((ROOT / 'META/m3/control-review').glob('*'))
    report = dict(scope='Exposed original menu features and exact loaded profile bindings; not full feature execution', menus=exposed, controls=controls, profiles=[p['name'] for p in profiles], episode_slots=[x['label'] for x in menus[0x4ce450]['items'][:9]], singleplayer_ships=[x['label'] for x in menus[0x4cdc9c]['items'][:3]], primary_weapons=[x['label'].replace('\n', ' ') for x in menus[0x4cdb98]['items'][5:]], secondary_weapons=[x['label'].replace('\n', ' ') for x in menus[0x4d4408]['items'][5:]], cameras=[x['label'].replace('\n', ' ') for x in menus[0x496344]['items'][1:7]], fixed_keys=[dict(action='abort/pause', value=27), dict(action='menu/input action 0x20', value=9), dict(action='confirm abort', value=89), dict(action='cancel abort', value=78)], evidence_sha256={str(p.relative_to(ROOT)).replace('\\', '/'): sha(p) for p in evidence_paths if p.is_file()}, runtime_limits=['Three loaded default profiles only; editing/new/delete operations not exercised', 'Weapon/camera labels are inventory, not successful gameplay scenarios', 'Episode labels do not prove unlocked status or completed content', 'Network host/join/transport and rendering/audio settings remain separately unvalidated'], feature_execution_complete=False, m3_complete=False)
    path = ROOT / 'META/m3/feature-inventory.json'
    if args.write_report:
        path.write_text(json.dumps(report, indent=2) + '\n')
    else:
        assert json.loads(path.read_text()) == report
    print(json.dumps(dict(passed=True, menus=len(exposed), control_fields=len(controls), profiles=len(profiles), binding_values=len(controls)*len(profiles), primary_weapons=len(report['primary_weapons']), secondary_weapons=len(report['secondary_weapons']), cameras=len(report['cameras']), feature_execution_complete=False)))

if __name__ == '__main__':
    main()
