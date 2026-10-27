#ifndef BH_ATLAS_TEXT_H
#define BH_ATLAS_TEXT_H
#include "bh_atlas_page.h"
/* Complete00470900: synchronous actual DirectDrawBlt glyph copying with
   DDBLT_WAIT. Original ignored native failures: cursor, total advance and
   resident pixels_dirty update proceed even when a blit fails. Source guards
   reject invalid page/surface bindings before mutation. NULL text returns0
   untouched; an empty nonnull string still marks an attached resident dirty.
   Rectangle x/y/width/height are original signed16-bit metrics. Width return
   sums all advances across lines, rather than reporting the widest line.
   Only glyphs wrap; spaces can pass the right edge. All cursor/advance adds
   retain original32-bit wrapping, including negative metrics. */
long __cdecl bh_atlas_page_text(BHAtlasPage *, const unsigned char *text);
/* Complete00470c90: sums signed widths for all valid byte glyphs including
   space. Newlines and unsupported bytes contribute0. Cursor is untouched. */
long __cdecl bh_atlas_page_text_width(const BHAtlasPage *, const unsigned char *text);
/* Complete00470d40: borrowed page image identity; NULL page yieldsNULL. */
BHTextureImage *__cdecl bh_atlas_page_image(const BHAtlasPage *);
#endif
