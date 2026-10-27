#ifndef BH_ATLAS_GLYPH_H
#define BH_ATLAS_GLYPH_H
#include "bh_atlas_page.h"
/* Complete004705c0 page clear (not glyph copying). Color conversion follows
   borrowed atlas pixel format/palette, full32bit color cache and earliest RGB
   Manhattan tie. Always reset all cursor fields and mark real resident pixels
   dirty after the native attempt, even failed Lock. NULL page is original no-op
   success. Source int/error/HRESULT expose ignored native failures and guard
   invalid bindings/dimensions; no original return-value claim. */
int __cdecl bh_atlas_page_clear(BHAtlasContext *, BHAtlasPage *, unsigned long color);
/* Complete004708d0. Raw signed coordinates, no clamping. NULL page is no-op. */
void __cdecl bh_atlas_page_cursor(BHAtlasPage *, long x, long y);
#endif
