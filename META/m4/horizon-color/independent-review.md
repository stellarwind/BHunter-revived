# Independent static review of 0045d7a0

2026-10-25. Reviewed complete 977-byte /303-instruction ordinary owner and its
real existing texture-table dependency. Implementation is parent-owned; this
file records independent findings without changing build/ledger files.

- `004edecc` is the indexed65536-byte table, `004eded0` the packed131072-byte
  table. Both share the **same** full32-bit cached color at `004eded4`. A hit
  tests only the selected buffer pointer and that shared key. It does not test
  the other buffer, display shifts, texture-table epoch, or a per-mode key.
  Consequently, indexed(A),packed(B),indexed(B) returns the old indexed(A)
  contents. Preserve and explicitly test this original cache behavior.
- Display storage mode exactly1 selects indexed output. Every other mode
  selects packed16 output. This is not a check for logical software enable.
- Texture-table initialization `0044e9b0` runs before the cache test. Its
  original latch behavior is already recovered and should remain real source.
- RGB generation ignores the top color byte but the cache key includes it.
  Changing only that byte therefore rebuilds the selected table.
- Input cube index i covers all256 entries. Its components are
  `((i/42)%6)*255/5`, `((i/6)%7)*255/6`, `(i%6)*255/5`. Entries252..255 repeat
  the first four cube colors; do not clamp or truncate the cube at252 entries.
- Row0 contains the constant input RGB color. Row255 contains the cube color.
  Intermediate channels are positive integer division by255 of
  `cube*row + constant*(255-row)`, with truncation toward zero.
- Indexed output uses actual six-level R/B quantization and seven-level G
  quantization from existing table owner: `((R*7+G)*6+B)` narrowed to a byte.
- Packed output reads the current original display channel shifts, uses x86
  CL count masking (low5bits), adds the three channel terms and stores low16
  bits. Original instructions are SAR then SHL. Their channel input is always
  in0..255, so unsigned right shift is equivalent for these valid values.
  Large shift counts must still be masked31; overflowing left shifts require
  unsigned32 arithmetic rather than undefined signed overflow.
- Display-format/shift changes alone do not invalidate a hit. On a color-key
  miss packed output uses the then-current live shifts, while indexed
  quantization uses the existing latched source texture tables.
- Original allocator failure was unchecked. A source null-allocation guard is
  an explicit binding extension, not evidence that the original returned an
  error. The shared key must not be published before a successful fill.

The engine provenance remains unverified. No import, verified CRT function,
EH/unwind helper or shared-tail body is being counted as a recovered gameplay
owner. No original code, game startup or gameplay was executed during review.
