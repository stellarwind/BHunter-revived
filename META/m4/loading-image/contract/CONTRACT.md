# Loading-image dependency and lifetime contracts

Complete original owners are00449e50 (769 bytes/267 instructions),0044a160
(178/42), and owning PCX-close wrapper0044f1b0 (15/5). The existing read-only
exports under `../../sound-loader/next-review/` verify every body against raw
immutable PE bytes. Image/stream logic is ordinary engine code with unknown
historical provenance; the three-image initializer is game-specific. None is
CRT/import/compiler support/shared-tail scaffolding. Known malloc/VFILE/clock
dependencies are already supplied by real definitions/imports.

## Admitted PCX contracts reused unchanged

`bh_pcx_reader_open(reader,name)` owns its VFILE and reads the final768 RGB
palette bytes, dimension words and compressed stream. It seeds identity map
and does not require a palette marker/version5/zero origin/one plane. Its
existing source size/seek/read guards reject unsafe files. Width and height
are original16-bit xmax+1/ymax+1, including wrap to zero.

`bh_pcx_palette(&converted,reader.palette,0,0)` reproduces the complete mode0
palette/index transformation. Index0 is reserved and original zero pixels map
to a selected nearest nonzero entry; other entries are brightened by the
original dark-color rule. Copy converted.indices into reader.map before any
pixel reads. Build loading lookup tables from converted.rgb, not the original
raw RGB palette. `bh_pcx_reader_next(reader,&index)` consults the **current map**
on every pixel, including a pending RLE repeat.

The active ledger has complete0044f0a0=`bh_pcx_reader_open`,0044efa0=
`bh_pcx_reader_next` and0044f1c0=`bh_pcx_palette`. Older WIP data-subset records
are historical duplicates, not missing assignments. No previously admitted
source/header variant changes for this family.

## Close reconciliation: adapter reuse, no new counted owner

Raw0044f1b0 loads owning VFILE global004e1174, calls real0047ce50 and returns.
It does not clear the global handle, palette, map or dimensions. Existing
`bh_pcx_reader_close` closes its owning VFILE, reports success/failure, and
clears the **whole explicit reader**, with additional NULL/empty/repeated-close
guards. It is already compiled/linked as source-lifecycle adapter
`bh_pcx_reader_open-source-lifecycle` with original_function_count0.

Root's conservative decision keeps that historical record and uses the same
unchanged definition as owning closure, with original_function_count0 for the
new mapping as well. Do not add a redundant wrapper/TU or call the extra
reader clearing an instruction-equivalent recovery of the original globals.

The source seam matters: original00449e50 calls close at00449f36 and branches
to lookup-table generation at00449f3b, **after** closing the stream. Retain
converted RGB/index state in the loading owner before closing the scoped
reader. The palette needed by lookup generation must survive the explicit
reader memset. `close-reconciliation.json` records this difference, complete
raw body and unchanged source hashes matching admitted reader variants.

Focused `build_check.py` compiles actual unchanged bh_pcx_reader.c and
bh_vfile.c plus close_check.c, links real current integration definitions into
`bhunter-pcx-close-reuse-check.exe`, and passes16 checks. A native exclusive
CreateFile positive control fails with sharing violation while the actual
FILE is owned and succeeds after closing; the caller's palette copy survives.
Missing-file, valid-file, NULL/empty/repeated-close and reopening paths execute.
No original code, game, UI, input, audio or mocked native interfaces execute.
Exact hashes/commands/results are in `standalone-build.json`.

## Sampling, table generation and arena boundaries

The branch at00449e50 uses **display width**004e2998, not the PCX width:
640 selects full output (640*reader.height pixels). Any other display width
selects half: for each of floor(reader.height/2) row pairs, store the first
of every horizontal pair across640 source pixels, then consume/discard640
pixels for the second row. It therefore stores320*floor(height/2), consuming
1280*floor(height/2); a final odd row is not read before close. Real assets are
640-wide. Source bounds must separately validate accessible stream/arena spans.

Original page_bytes global004dfc80 is fixed307200(full) or76800(half), not the
image's total stored bytes. BAD has960 source rows and occupies two such pages.
All full/half output indices are transformed through the mode0 map above.

For pixel_bytes==1, only indexed normal and quarter tables are recomputed:
normal=(R*5/255)*42+(G*6/255)*6+(B*5/255), integer floors;
quarter applies component>>2 **before** each multiplication/division.
For all other pixel sizes, only native normal/quarter tables are recomputed:
normal=sum((component >> expand[channel]) << extract[channel]); quarter first
applies component>>2, then the same expand/extract. This is not packedword>>2.
The palette components are RGB. Native globals are red expand38c0/extract32a4,
green expand3890/extract3260, blue expand3894/extract3264 (prefix004e).
Original x86 shift counts use their low5 bits; ordinary display formats should
have reviewed shifts0..31. Use unsigned32-bit packing, preserving addition.
Do not silently update inactive backend tables: their previous contents remain.

Initializer0044a160 allocates0x12c000=1,228,800 bytes and loads MIDAS at0x96000,
IEL at0xe1000, BAD at0. It copies indexednormal256 and nativenormal1024 after
each MIDAS/IEL load **regardless of active backend**, so stale inactive tables
remain meaningful snapshots. It does not copy quarter tables to those slots.
Then clock+5000 wraps32 bits, display state becomes2, and progress clears.
Original allocation/image return values are unchecked; source guards must
report failure and define newly owned arena rollback explicitly.

The three independently decoded archive controls in tests/pcx_manifest.h are
BAD640x960, MIDAS640x480 and IEL640x480. Their full output exactly fills the
arena at those offsets; half output has intentional gaps. Tests can seed all
tables, verify active/inactive behavior and retained snapshots, compare every
mapped output byte with independent PCX decode, and force allocation/stream/
capacity errors. No renderer, screenshot or broad reference replay is needed.
