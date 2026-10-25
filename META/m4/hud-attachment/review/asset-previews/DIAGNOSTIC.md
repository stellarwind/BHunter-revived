# Bounded HUD native texture diagnostic

Static investigation, 2026-10-25. No game/original execution or production edits. PCXs extracted unchanged from immutable BIN by analysis/assets-index.json offsets; inspection.json records payload hashes/headers/pixel statistics. PNG previews use an independent standard PCX decoder:8-bit one-plane RLE,128-byte header, padded bytes-per-line and trailing256-entry RGB palette. Decode produced exactly bytes_per_line*height for each image.

RADAR128x128 has60colors and green radar texture. CLIPBRD128x128 has105colors: green text, red/orange gauges and labels. WEAPONS128x128 has250colors. They are not white masks. BLUE32x32,TAGS128x128,DOTS32x32,TVFONT320x280 also decoded as colored content. Palette mode2 only collapses near-black colors/reassigns transparency; it cannot account for white/checkerboard geometry. Viewed CLIPBRD preview against the existing hud-native-original-flat.bmp: bitmap's flat white/checker regions do not represent the raw atlas.

Concrete likely fixture binding cause from source:

1. tests/hud_native.c creates only32x32 procedural solid images, then calls bh_render_material_initialize.
2. Material initializer calls bh_texture_preallocate. That caps each format.minimum to current references, so at that time128x128 format references/minimum/count remain0 and no128 texture slots are created.
3. Actual128x128 HUD images are loaded later. No texture preallocation/slot creation appears after those loads.
4. bh_texture_resident returns0 when image->format->slots is NULL without setting a backend error. bh_texture_draw_flush then binds texturehandle0 and draws vertex colors, explaining solid white HUD geometry while earlier32x32 procedural controls succeed.

Recommended narrow check: inspect128 image format count/resident handle, create actual fixture-owned slots after actual image loading using existing admitted slot/preallocation owners, and verify nonzero resident handles plus colored atlas pixels. This is a fixture/runtime-binding diagnosis, not an admitted correction or proven execution result. Do not rewrite recovered cache behavior solely from this symptom.

Resolution: the fixture now invokes actual bh_texture_preallocate after loading the HUD/font assets, checks nonzero slots/resident handles, and renders the colored atlas correctly. Final linked hud-native scenario passes102 real-asset checks. No production cache behavior was changed.
