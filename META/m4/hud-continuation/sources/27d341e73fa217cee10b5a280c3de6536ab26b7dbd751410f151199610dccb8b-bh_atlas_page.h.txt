#ifndef BH_ATLAS_PAGE_H
#define BH_ATLAS_PAGE_H
#include "bh_atlas_image.h"
/* Original0x28 page state returned by00470380. Atlas is borrowed cache identity;
   image owns the newly created native page, surface aliases image->source.
   packed_color is undefined originally until first clear; source initializes0. */
typedef struct BHAtlasPage {
    BHAtlasImage *atlas;
    BHTextureImage *image;
    LPDIRECTDRAWSURFACE surface;
    unsigned long last_color,packed_color;
    long cursor_x,cursor_y,left;
    unsigned long size;
    long line_height;
} BHAtlasPage;
/* Complete00470380: requests(size,size,8,1), loads the literal atlas through the
   real cache owner, creates a blank native texture page named@@@PAGE@@@ and
   palette/dither tables when appropriate, then publishes image in q.images.
   Does not copy glyphs/create UVs or acquire a device texture handle; those are
   subsequent behavior. Source failure guards free partial page/image resources,
   while the real atlas cache and format-request side effects remain borrowed. */
BHAtlasPage *__cdecl bh_atlas_page_create(BHAtlasContext *,const char *name,unsigned long size);
/* Explicit source lifetime adapter; call before disposing borrowed atlas/q.
   Unlinks/releases the page image and detaches any real residency slot. */
int __cdecl bh_atlas_page_dispose(BHAtlasContext *,BHAtlasPage *);
#endif
