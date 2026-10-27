"""Audit current UI resource wiring against retained original-owner contracts.

Read-only: does not start the game or send input. This verifies bindings, not
pixel fidelity, UI timing, actual peer gameplay or the hardware backend.
"""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'META/m4/playable/ui-bindings/contracts.json'


def compact(text):
    return re.sub(r'\s+', '', text)


def verify(contract, app, bootstrap):
    errors = []
    for row in contract['resources']:
        field = row['field']
        expected = int(row['identity'], 16)
        bindings = [int(value, 16) for value in re.findall(
            r'B\(0x([0-9a-fA-F]+),' + re.escape(field) + r'\)', app)]
        bindings += [int(value, 16) for value in re.findall(
            r'a->' + re.escape(field) + r'=(?:V\(|bh_bootstrap_value\(&a->bootstrap,)0x([0-9a-fA-F]+)\)', app)]
        if field.startswith('panel.'):
            member = field.removeprefix('panel.')
            bindings += [int(value, 16) for value in re.findall(
                r'c->panel->' + re.escape(member) + r'=bh_bootstrap_value\(c,0x([0-9a-fA-F]+)\)', bootstrap)]
        if field.startswith('panel.icon_images['):
            if 'for(i=0;i<12;++i)c->panel->icon_images[i]=bh_bootstrap_value(c,0x4df1d8+i*4);' in bootstrap:
                index = int(field.split('[')[1].split(']')[0])
                bindings.append(0x4df1d8 + index * 4)
        if not bindings or any(value != expected for value in bindings):
            errors.append(f'{field}: expected {expected:08x}, found {bindings}')
    for field, expression in contract['expressions'].items():
        if compact(f'a->{field}={expression};') not in app:
            errors.append(f'{field}: missing derived binding {expression}')
    return errors


def main():
    contract = json.loads(CONTRACT.read_text())
    app = compact((ROOT / 'src/bh_game_app.c').read_text())
    bootstrap = compact((ROOT / 'src/bh_game_bootstrap.c').read_text())
    assets = {int(identity, 16): name for identity, name in re.findall(
        r'\{[123],0x([0-9a-fA-F]+)UL,"([^"]+)"',
        (ROOT / 'src/bh_bootstrap_data.h').read_text())}
    for row in contract['resources']:
        data = (ROOT / row['reference']).read_bytes()
        assert hashlib.sha256(data).hexdigest() == row['reference_sha256'], row['reference']
        identity = '004df1d8' if row['field'].startswith('panel.icon_images[') else row['identity']
        assert identity in data.decode().lower(), row['field']
        if not row['asset'].startswith('derived '):
            assert assets.get(int(row['identity'], 16)) == row['asset'], row['field']
    errors = verify(contract, app, bootstrap)
    assert not errors, '\n'.join(errors)
    # Positive audit plus in-memory wrong-resource/missing-page controls.
    wrong = app.replace('B(0x4f00c0,hud.background_image)', 'B(0x4f54d8,hud.background_image)')
    assert any(error.startswith('hud.background_image:') for error in verify(contract, wrong, bootstrap))
    wrong = app.replace('a->score_draw.page_image=V(0x4cdb9c);', '')
    assert any(error.startswith('score_draw.page_image:') for error in verify(contract, wrong, bootstrap))
    wrong = app + 'a->projectile_draw.ballistic_flare=V(0x4f007c);'
    assert any(error.startswith('projectile_draw.ballistic_flare:') for error in verify(contract, wrong, bootstrap))
    print(f"UI binding audit passed: {len(contract['resources'])} resource contracts, "
          f"{len(contract['expressions'])} derived/pointer/viewport bindings, 3 rejection controls. "
          'Visual fidelity and live multiplayer remain unverified.')


if __name__ == '__main__':
    main()
