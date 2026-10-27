#ifndef BH_NETWORK_DEBUG_H
#define BH_NETWORK_DEBUG_H
#include "bh_network_game.h"
#include "bh_gdi_text.h"
typedef struct BHNetworkDebug
{
    BHDrawQueue *queue;
    const BHNetworkGame *game;
    const float *ping_ticks;
    long mode, defcon, good, bad, ugly;
    short frame;
    unsigned long attempted, queued, dropped;
    int error;
} BHNetworkDebug;
/* Whole00428dc0. Borrow actual roster/counters and queue; host shows active
   slots1..player_count-1, with compact y90/110/... rows and ping*33.333ms. CRT
   sprintf formatting and producer signed16XY/59byte cap remain authoritative.
   No GDI execution, network polling or session is initiated. Bounded source
   guards require <=8players and final text <=58bytes for real producer safety;
   source names over its32byte slot or nonfinite ping reject before publishing. */
int __cdecl bh_network_debug_draw(BHNetworkDebug *);
#endif
