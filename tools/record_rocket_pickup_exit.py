"""Preserve the reported exit and this diagnostic-only revision's evidence."""
import hashlib
import json
import shutil
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'META/m4/playable/rocket-pickup-exit'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    exe = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    count, = struct.unpack_from('<H', exe, pe + 6)
    optional, = struct.unpack_from('<H', exe, pe + 20)
    base, = struct.unpack_from('<I', exe, pe + 24 + 28)
    sections = pe + 24 + optional
    verified = 0
    for name in ('original-step.json', 'original-refresh.json'):
        rows = json.loads((OUT / name).read_text(encoding='utf-8-sig'))
        assert len(rows) > 10
        for row in rows:
            rva = int(row['address'], 16) - base
            expected = bytes.fromhex(row['bytes'])
            for i in range(count):
                size, va, raw_size, raw = struct.unpack_from('<IIII', exe, sections + i * 40 + 8)
                if va <= rva < va + raw_size:
                    assert exe[raw + rva - va:raw + rva - va + len(expected)] == expected
                    verified += 1
                    break
            else:
                raise AssertionError(row)
    for source, dest in (
        ('build/runtime-fixes/validation.json', 'focused-checks.json'),
        ('build/game/build.json', 'game-build.json'),
        ('build/game/BHunter-source.map', 'diagnostic-game.map'),
        ('build/game/startup-check-hardware.json', 'hardware-startup.json')):
        shutil.copyfile(ROOT / source, OUT / dest)
    for name in ('focused-checks.json', 'game-build.json', 'hardware-startup.json'):
        assert json.loads((OUT / name).read_text())['passed']
    changed = ['src/bh_entities.c', 'src/bh_entities.h', 'src/bh_frame_service.c',
               'src/bh_frame_service.h', 'src/bh_game_app.c', 'tests/entities.c',
               'tests/runtime_fixes.c', 'tools/check_runtime_fixes.py']
    record = dict(status='awaiting-human-reproduction-not-fixed',
                  human_log='human-startup.log', human_build_sha256=sha(OUT / 'human-build.exe'),
                  finding='Frame service ENTITY failure; exact callback and mutation not recorded by the human build.',
                  hypothesis='Cached successor lifetime guard; not yet attributed to the pickup or training objective.',
                  original_instructions_verified=verified,
                  original_sha256=hashlib.sha256(exe).hexdigest(),
                  behavior_changed=False, automated_gameplay=False, software_renderer_tests=False,
                  sources={p: sha(ROOT / p) for p in changed})
    (OUT / 'validation.json').write_text(json.dumps(record, indent=2) + '\n')
    ledger_path = ROOT / 'META/function-status.json'
    ledger = json.loads(ledger_path.read_text())
    def visit(value):
        if isinstance(value, dict):
            if value.get('source') in changed and 'validation' in value:
                value['validation']['rocket_pickup_exit_diagnostic'] = {
                    'evidence': 'META/m4/playable/rocket-pickup-exit/validation.json',
                    'source_sha256': sha(ROOT / value['source']),
                    'integration': 'linked_game', 'target': 'build/game/BHunter-source.exe',
                    'scope': 'Diagnostic-only revision; focused checks and static hardware startup pass. Reported training exit remains unresolved.'}
            for child in list(value.values()):
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)
    visit(ledger)
    ledger_path.write_text(json.dumps(ledger, indent=2) + '\n')
    print(json.dumps(record, indent=2))

if __name__ == '__main__':
    main()
