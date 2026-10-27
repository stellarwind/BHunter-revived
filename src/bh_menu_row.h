#ifndef BH_MENU_ROW_H
#define BH_MENU_ROW_H
#include "bh_atlas_wide.h"
typedef void(__cdecl *BHMenuRowRefresh)(void *menu, long index);
typedef struct BHMenuRow
{
    unsigned long identity;
    BHTextureImage *image;
    BHAtlasPage *page;
    const unsigned char *title;
    unsigned long first, second;
    BHMenuRowRefresh refresh;
} BHMenuRow;
typedef struct BHMenuRows
{
    BHAtlasContext *atlas;
    BHTextQueue *text;
    BHMenuRow **rows;
    unsigned long count, capacity, dropped;
    int error;
} BHMenuRows;
/* Count-zero owner bindings: row-pointer array/atlas/text live until disposal;
   title and optionalrefresh remain borrowed. Dispose only after actual FIFO
   drained, before atlas/texture owners. Body is writable and split inplace. */
int __cdecl bh_menu_rows_init(BHMenuRows *, BHAtlasContext *, BHTextQueue *, BHMenuRow **, unsigned long);
int __cdecl bh_menu_rows_dispose(BHMenuRows *);
/* Whole0042c4c0. Real28byte row +128fontpage construction. Optionalrefresh!=0
   skips initial queue painting but retains all metadata/count. Queuefull drops
   individual commands with diagnostics; it doesn't undo valid ownership. */
BHMenuRow *__cdecl bh_menu_row_create(BHMenuRows *, unsigned long, const unsigned char *title, unsigned char *body,
                                      unsigned long, unsigned long, BHMenuRowRefresh);
/* Whole0042c7d0. NULLnewtitle retains previous pointer; clears and paints exact
   convertedwide title and centeredASCIIbody. Bodynewlines are replacedbyNUL.
   Validtitlelines have<=255convertedunits. Source guardsrejectunsafeinput. */
int __cdecl bh_menu_row_refresh(BHMenuRows *, unsigned long, const unsigned char *newtitle, unsigned char *body);
#endif
