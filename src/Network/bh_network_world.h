#ifndef BH_NETWORK_WORLD_H
#define BH_NETWORK_WORLD_H
#include <stdint.h>

typedef struct BHNetworkPickup
{
    uint16_t placement, type, total, offer, price, generation;
    uint16_t lifetime, period_min, period_max, repeats, activation;
    uint32_t expires_at, next_at, next_ms;
    int activated, synced, available;
} BHNetworkPickup;

typedef struct BHNetworkWorld
{
    BHNetworkPickup *pickups;
    unsigned int pickup_count, placement_count;
    uint16_t *destroyed_props;
    unsigned int destroyed_count, destroyed_capacity;
} BHNetworkWorld;

typedef void (*BHNetworkPickupUpdate)(void *, const BHNetworkPickup *, uint32_t now);
typedef int (*BHNetworkPickupAward)(void *, const BHNetworkPickup *);

int bh_network_world_load(BHNetworkWorld *, int custom_level);
void bh_network_world_dispose(BHNetworkWorld *);
int bh_network_world_record_prop(BHNetworkWorld *, unsigned int placement);
BHNetworkPickup *bh_network_world_pickup(BHNetworkWorld *, unsigned int placement);
void bh_network_world_tick(BHNetworkWorld *, uint32_t now, BHNetworkPickupUpdate, void *);
int bh_network_world_activate_pickup(BHNetworkPickup *, uint32_t now);
int bh_network_world_claim_pickup(BHNetworkPickup *, unsigned int generation, uint32_t now,
                                  BHNetworkPickupAward, void *);
uint32_t bh_network_world_pickup_next_ms(const BHNetworkPickup *, uint32_t now);
void bh_network_world_apply_pickup(BHNetworkPickup *, uint16_t generation, int available, uint32_t next_ms);
#endif
