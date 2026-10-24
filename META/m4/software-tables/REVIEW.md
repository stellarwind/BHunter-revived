# Software palette/blend initialization 0046e600

Complete ordinary engine-candidate initialization owner, called by 0045c540.
The body is 3,713 bytes / 1,122 instructions, with two ordinary return paths,
not a raster kernel or compiler helper. Unknown historical engine authorship
does not establish third-party library provenance. The only dependencies are
admitted 0044e9b0 texture tables and verified CRT malloc wrapper 0047dae0
(calls __nh_malloc); allocator/runtime is reused and excluded from recovery count.

The original fixed palette has6 red x7 green x6 blue levels. Components are
`floor(digit*255/(levels-1))`; index digits are `(i/42)%6,(i/6)%7,i%6`.
Indices252..255 wrap red to zero and green to zero with blue digits0..3.
They are not reserved colors or arbitrary caller palette entries.

All modes allocate the original128KiB byte table004e3858: first65,536 bytes
are `[intensity][palette]` RGB scaling by intensity/255, second65,536 are
`[RGB332][palette]` multiplication with normalized3/3/2bit components /7,/7,/3.
RGB quantization uses real admitted6/7/6 bucket tables and encodes `(R*7+G)*6+B`.

Pixel byte count1 allocates004e2df4's512KiB indexed blend table. Other original
values select highcolor; source explicitly admits byte count2 only. Native16
allocates004e2de4's256KiB word shade/modulation table and004e38c8's1MiB word
blend table, packed with actual display right/left shifts. Every allocation is
fully initialized with exact original table sizes and row order.

Eight blend planes have256 outer x256 inner entries. Indexed outer is the fixed
6x7x6 palette. Native outer is coarse RGB332: `i&224,(i&28)*8,(i&3)*64`, so its
maximum components are224/224/192, unlike the normalized modulation table.
Inner is always fixed-palette RGB. Planes are:

1. screen: inner + floor((255-inner)*outer/255)
2. inverse multiply: floor((255-inner)*outer/255)
3. saturating addition
4. through8: floor((inner*w+outer*(6-w))/6), weights1 through5

Original explicit0..255 clipping and integer truncation are retained. The
native mask-derived fields select top3 red/3 green/2 blue bits and retain
original signed extraction formulas13/10/8 minus stripped extra bits and field
offset. Mask and display shifts are explicit caller bindings; no globals guessed.

BHSoftwareTables owns all allocations and copies the real caller allocator.
Zero initialize it before use; a separate dispose adapter releases ownership.
Guards reject live reinitialization and safely clean partial allocation failures.
Invalid/noncontiguous/overlapping/out-of16bit masks, or insufficient3/3/2color
bits, reject before allocation rather than entering original nonterminating
division loops. Shared texture table initialization retains its original latch.

`python META/m4/software-tables/build.py` compiles the actual new source/test
TUs plus real src/bh_texture_tables.c with pinned SP3 compiler/headers, links
stock VC5 LINK and real CRT/kernel32, then runs
`build/software-tables/bhunter-software-tables.exe`. No existing m0 objects,
fake interfaces or original code are needed. All2,228,273 checks pass:
exhaustive buffers on indexed8,RGB565,RGB555; independent bucket-boundary
quantization/mixed-radix/component-field formulas; wrapped palette indices;
normalized-versus-coarse RGB332 controls; masks/extraction values; allocation
sizes/counts; each partial allocation failure and explicit malformed-input guards.

`audit.py` checks all original instructions against the preserved PE and verifies
source hashes/focused result. No game launches, media writes or original code
execution. No complete rasterizer, software renderer, source-game or historical
instruction-match claim. Parent integration remains separate evidence.
