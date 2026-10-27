#ifndef BH_TEXTURE_DRAW_H
#define BH_TEXTURE_DRAW_H
#include "bh_texture.h"
/* 004507d0. Returned pointer may be invalidated by the next append/growth.
   Loaded images own real arenas. Zero-capacity staging helpers must first use
   the explicit polygon composition below. Failure retains active data/counts. */
D3DTLVERTEX *__cdecl bh_texture_draw_vertex(BHTextureContext *, BHTextureImage *);
/* 004508a0. Nonempty native queues submit one fan per run, flags0x0c, then
   clear run/vertex/serial counts even after native API failure. Optional run
   counter replaces the original global. Software mode retains pending data;
   this routine does not invoke or implement the separate software rasterizer. */
int __cdecl bh_texture_draw_flush(BHTextureContext *, BHTextureImage *, unsigned long *runs_drawn);
/* Explicit source composition of already projected TL vertices, NOT recovery
   of original 0046e390 linked-ring conversion or software producer. Initializes
   staging-helper arenas when absent, appends a complete >=3 vertex fan, and
   rolls back active counts on failure. Input must not overlap the image arenas. Does not flush or reorder images. */
int __cdecl bh_texture_draw_polygon(BHTextureContext *, BHTextureImage *, const D3DTLVERTEX *, unsigned long count);
#endif
