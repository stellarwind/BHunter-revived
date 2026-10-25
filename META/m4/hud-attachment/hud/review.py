"""Read-only whole-owner and constant audit. Never executes original code."""
from pathlib import Path
import json, struct, hashlib

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
blob = (ROOT / 'GAME_DATA/BHUNTER.EXE').read_bytes()
assert hashlib.sha256(blob).hexdigest() == 'c86205e299119a66e27f687679ee7e978f26fa9a6c0bfb21f664aa6e74e848b1'
pe = struct.unpack_from('<I', blob, 0x3c)[0]
n, opt = struct.unpack_from('<H', blob, pe+6)[0], struct.unpack_from('<H', blob, pe+20)[0]
sections = [struct.unpack_from('<IIII', blob, pe+24+opt+i*40+8) for i in range(n)]
def read(va, size):
    rva = va-0x400000
    for virtual_size, virtual_address, raw_size, raw_address in sections:
        if virtual_address <= rva and rva+size <= virtual_address+raw_size:
            return blob[raw_address+rva-virtual_address:raw_address+rva-virtual_address+size]
    raise ValueError(hex(va))

original = ROOT / 'META/m4/radar/original'
rows = json.loads((original / '0041b2d0.disasm.json').read_text())
cursor = 0x41b2d0
for row in rows:
    raw = bytes.fromhex(row['bytes'])
    assert int(row['address'],16) == cursor
    assert raw == read(cursor,len(raw))
    cursor += len(raw)
assert cursor == 0x41b64d and len(rows) == 233
constants = []
for va, fmt in [(0x489858,'f'),(0x48985c,'f'),(0x489860,'f'),(0x489864,'f'),(0x489868,'f'),(0x4894f8,'d'),(0x489870,'d'),(0x489878,'d'),(0x489200,'f')]:
    raw = read(va,struct.calcsize(fmt))
    constants.append(dict(address=f'{va:08x}',type=fmt,raw=raw.hex(),value=struct.unpack('<'+fmt,raw)[0]))
result = dict(address='0041b2d0',end_exclusive=f'{cursor:08x}',bytes=cursor-0x41b2d0,instructions=len(rows),raw_sha256=hashlib.sha256(read(0x41b2d0,cursor-0x41b2d0)).hexdigest(),whole_raw_body_verified=True,constants=constants,original_code_executed=False)
(OUT/'raw-review.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
