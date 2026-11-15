#ifndef BH_NETWORK_WORLD_BINDING_H
#define BH_NETWORK_WORLD_BINDING_H
#include "Player/bh_player_query.h"
#include "../World/bh_world_reset.h"
#include <stdio.h>

typedef struct BHNetworkPickupBinding BHNetworkPickupBinding;

/* Binds network world state to one loaded game map. The world state itself
   belongs to the network session and can outlive a client map reload. */
typedef struct BHNetworkWorldBinding
{
    BHNetworkGame *game;
    BHScriptVMContext *vm;
    const BHWorldResetNode *placements;
    BHPlayerQueryContext *player_query;
    FILE *log;
    BHNetworkPickupBinding *pickups;
    unsigned int props_applied;
} BHNetworkWorldBinding;

int bh_network_world_binding_bind(BHNetworkWorldBinding *, BHNetworkGame *, BHScriptVMContext *,
                                  const BHWorldResetNode *, BHPlayerQueryContext *, FILE *log, int custom_level);
void bh_network_world_binding_step(BHNetworkWorldBinding *, unsigned long frame, int paused);
void bh_network_world_binding_dispose(BHNetworkWorldBinding *);
#endif
