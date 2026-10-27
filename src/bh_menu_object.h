#ifndef BH_MENU_OBJECT_H
#define BH_MENU_OBJECT_H
#include "bh_entities.h"
#include "bh_player_input_events.h"
#include "bh_menu_row.h"
typedef struct BHMenuObject
{
    BHEntityPool *pool;
    BHPlayerInputOwner *input_owner;
    BHEntity *entity;
    BHPlayerInputList *input;
    BHMenuRows rows;
    int rows_bound;
} BHMenuObject;
/* Whole0042c300. Fresh zeroed source owner. tag retains raw argument bits;
   live_name replaces borrowed004dfb5c (NULL selects its initial empty value).
   Failure preserves owner. Pool/input owners must outlive the menu. The
   original output pointer is represented by entity, published last.
   No native input polling, menu update/query callback or global is invented. */
int __cdecl bh_menu_object_create(BHMenuObject *, BHEntityPool *, BHPlayerInputOwner *, unsigned long tag,
                                  const char *live_name);
/* Count-zero source bindings to actual restored row/page/FIFO ownership.
   Caller provides row capacity; original constructor does not establish it. */
int __cdecl bh_menu_object_rows_init(BHMenuObject *, BHAtlasContext *, BHTextQueue *, BHMenuRow **, unsigned long);
BHMenuRow *__cdecl bh_menu_object_add_row(BHMenuObject *, unsigned long, const unsigned char *, unsigned char *,
                                          unsigned long, unsigned long, BHMenuRowRefresh);
/* Reject pending page commands or active entity dispatch. Callers must not
   release menu entity/list independently. Removes just this actual list. */
int __cdecl bh_menu_object_dispose(BHMenuObject *);
#endif
