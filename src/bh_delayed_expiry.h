#ifndef BH_DELAYED_EXPIRY_H
#define BH_DELAYED_EXPIRY_H
#include "bh_entities.h"
/* Count-zero adapter supplies actual entity pool; entity+e8 lifetime and
   +198 delay retain original values. Context must outlive installed callback. */
int __cdecl bh_delayed_expiry_bind(BHEntityPool *,BHEntity *,float lifetime,float delay);
void __cdecl bh_delayed_expiry_tick(BHEntity *); /*402cc0*/
#endif
