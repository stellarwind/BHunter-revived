# Atlas text-page backing constructor00470380

Complete ordinary engine-candidate owner, called by0042c4c0/004474c0.
Historical engine provenance is unknown. Its196instructions/569bytes are not
an import, CRT/compiler helper or shared tail; real allocator/free are verified
runtime dependencies and are excluded from newly recovered game-code counts.

The constructor first allocates the original0x70 BHTextureImage, sets flags1 and
the exact marker `@@@PAGE@@@` verified from original491870. It requests the real
format(size,size,8,1), then loads the literal atlas through recovered0046f490
using that format's actual DDPIXELFORMAT. Cache atlas identity remains borrowed.

Returned BHAtlasPage preserves original0x28 layout: atlas+00,image+04,
surface alias+08,last_color+0c,packed_color+10,cursor_x+14,cursor_y+18,left+1c,
size+20,line_height+24. Original initializes last_color=-1, cursor/left/height0;
packed_color+10 remains undefined until a later clear. Source initializes this
field and otherwise undefined padding to0, an explicit deterministic extension.

It creates a size-square actual native texture/system-memory surface, caps0x1800
in both native/software branches.8bit output receives a real native palette from
the borrowed atlas's transformed RGB triples, zero entry flags, caps0x44, then
the real admitted palette-table owner0044eb60. That owner only allocates dither/
screen buffers when q.software is active. Highcolor bypasses palette handling.
The constructor allocates0x80 run arena and0xc00 TL-vertex arena, capacity96,
sets queue counts/residency/unused54 zero and publishes image in real q.images.
Returned surface is a borrowed alias of the image's owning source interface.

There is no glyph blit, rectangle copy, UV generation or native GetHandle in
00470380. The page starts blank/uninitialized; later clear/text/residency owners
provide its content and device handle. No stub or guessed behavior substitutes
for those dependencies. Tests explicitly fill the page for separate downstream
residency positive controls, not as a constructor behavior claim.

Source failure guards release partial image/page/native/palette/table/arena
resources. Original unchecked palette/arena allocation behavior and leaked page
on native surface failure are not reproduced. Existing format pending-reference
increments and borrowed atlas-cache population remain after later failure, as
original. Separate bh_atlas_page_dispose is a source lifetime adapter: unlink the
image, detach actual residency slot association, release all owned resources and
page record, retain the independently owned atlas cache. Dispose before atlas/q.

`python META/m4/atlas-page/build.py` compiles actual new page/source tests and
the actual recovered atlas owner, then links current real m0 dependencies,
pinned SP3 compiler/headers, stock VC5 LINK and official native SDK/system libs
into `build/atlas-page/bhunter-atlas-page.exe`.135checks pass: native8/16 and
q.software0/1; actual surface caps/dimensions; exact record/image initial state;
real palettes/dither/screen tables; independent pages sharing atlas identity;
real preallocation, residency and native Texture2 GetHandle/content controls;
ten allocator failure positions; format/missing-file rejection; residency detach
and complete actual allocation cleanup. No fake COM or callback stubs.

`audit.py` checks the complete original body and marker against immutable EXE,
plus source hashes/focused results. No original/game execution or media writes.
Parent full integration and actual font/page caller behavior remain separate.
