#ifndef BH_SCRIPT_FLASH_H
#define BH_SCRIPT_FLASH_H
#include "bh_entities.h"
/* Count-zero source binding of verified flash countdown+e8 and persistent
   actual pool at private payload+456. This recovers neither the missing
   004035e0 light refresh nor its full VM constructor; existing refresh/query
   bindings are retained. Caller must retain pool/entity through the tick. */
int __cdecl bh_script_flash_bind(BHEntityPool *,BHEntity *,float countdown);
/* Whole00403640: always subtract exact PE binary64 1/30, store binary32,
   but decide expiry from the retained pre-store value. Zero/negative/NaN
   subtraction results release the actual pool entity. No draw/input/audio. */
void __cdecl bh_script_flash_tick(BHEntity *);
float __cdecl bh_script_flash_countdown(const BHEntity *);
#endif
