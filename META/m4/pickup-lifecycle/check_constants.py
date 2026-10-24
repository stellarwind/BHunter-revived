"""Read pickup constants directly from the immutable PE file; no execution."""
from pathlib import Path
import struct

exe = Path(__file__).resolve().parents[3] / "GAME_DATA/BHUNTER.EXE"
data = exe.read_bytes()
pe = struct.unpack_from("<I", data, 0x3c)[0]
assert data[pe:pe + 4] == b"PE\0\0"
count = struct.unpack_from("<H", data, pe + 6)[0]
optional = pe + 24
assert struct.unpack_from("<H", data, optional)[0] == 0x10b
image_base = struct.unpack_from("<I", data, optional + 28)[0]
section_start = optional + struct.unpack_from("<H", data, pe + 20)[0]
sections = []
for i in range(count):
    at = section_start + i * 40
    name = data[at:at + 8].rstrip(b"\0").decode("ascii")
    virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from("<IIII", data, at + 8)
    sections.append((name, virtual_address, max(virtual_size, raw_size), raw_pointer))

expected = {
    0x004891f8: "111111111111a13f",
    0x00489200: "00000000",
    0x00489208: "111111111111a1bf",
    0x00489218: "000000000000d0bf",
    0x00489228: "9a9999999999293f",
    0x00489328: "000000000000d03f",
    0x004895a0: "000000000000e03e",
    0x004896f0: "000000000000f0bf",
    0x00489da8: "0000000000004240",
}
for address, hex_bytes in expected.items():
    rva = address - image_base
    length = len(hex_bytes) // 2
    matches = [s for s in sections if s[1] <= rva and rva + length <= s[1] + s[2]]
    assert len(matches) == 1, hex(address)
    name, start, _, raw = matches[0]
    actual = data[raw + rva - start:raw + rva - start + length]
    assert actual.hex() == hex_bytes, (hex(address), actual.hex())
    value = struct.unpack("<f" if length == 4 else "<d", actual)[0]
    print(f"{address:08x} {name} {actual.hex()} {value:.17g}")
print(f"verified {len(expected)} constants against {exe}")
