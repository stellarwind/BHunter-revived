#ifndef BH_WEAPON_LATCH_H
#define BH_WEAPON_LATCH_H
#include "bh_weapon_common.h"
/* Complete00433170, weapon-index3 method+12c. Both original+f0/+f4 words
   are binary32 timers: strictly positive values subtract the actual double
   1/30 constant then store binary32. No clamp: a small positive value crosses
   below zero and subsequent calls preserve it. Zero/negative/unordered values
   retain their exact memory bits, including signed zero and NaN payloads.
   This is not integer decrement/wrap behavior. All other entity bytes survive.
   Caller borrows a live actual weapon allocation and masked PC53/nearest FP
   environment. No constructor, callback table or original code is supplied. */
int __cdecl bh_weapon_latch_tick(BHEntity *weapon);
#endif
