#include "bh_network_world.h"
#include <enet/time.h>
#include <stdlib.h>

enum { ARENA_START = 3333 };

void bh_network_world_dispose(BHNetworkWorld *world)
{
    free(world->pickups);
    free(world->destroyed_props);
    *world = (BHNetworkWorld){0};
}

int bh_network_world_record_prop(BHNetworkWorld *world, unsigned int placement)
{
    if (world->placement_count && placement >= world->placement_count)
        return 0;
    for (unsigned int i = 0; i < world->destroyed_count; ++i)
        if (world->destroyed_props[i] == placement)
            return 1;
    if (world->destroyed_count == world->destroyed_capacity)
    {
        unsigned int capacity = world->destroyed_capacity ? world->destroyed_capacity * 2 : 16;
        uint16_t *props = realloc(world->destroyed_props, capacity * sizeof(*props));
        if (!props)
            return 0;
        world->destroyed_props = props;
        world->destroyed_capacity = capacity;
    }
    world->destroyed_props[world->destroyed_count++] = (uint16_t)placement;
    return 1;
}

BHNetworkPickup *bh_network_world_pickup(BHNetworkWorld *world, unsigned int placement)
{
    for (unsigned int i = 0; i < world->pickup_count; ++i)
        if (world->pickups[i].placement == placement)
            return world->pickups + i;
    return NULL;
}

static int will_spawn(const BHNetworkPickup *item)
{
    return item->activated && item->period_max && (!item->repeats || item->generation < item->repeats);
}

uint32_t bh_network_world_pickup_next_ms(const BHNetworkPickup *item, uint32_t now)
{
    if (!will_spawn(item))
        return UINT32_MAX;
    return ENET_TIME_LESS(now, item->next_at) ? item->next_at - now : 0;
}

void bh_network_world_apply_pickup(BHNetworkPickup *item, uint16_t generation, int available, uint32_t next_ms)
{
    item->generation = generation;
    item->available = available;
    item->next_ms = next_ms;
    item->activated = generation != 0 || next_ms != UINT32_MAX;
    item->synced = 1;
}

static uint32_t next_pickup(const BHNetworkPickup *item, uint32_t now)
{
    unsigned int range = 1000U * (item->period_max - item->period_min);
    return now + 1000U * item->period_min + (range ? (unsigned int)rand() % (range + 1) : 0);
}

static void spawn_pickup(BHNetworkPickup *item, uint32_t now)
{
    ++item->generation;
    item->available = 1;
    item->expires_at = now + 1000U * item->lifetime;
    item->next_at = next_pickup(item, now);
}

int bh_network_world_activate_pickup(BHNetworkPickup *item, uint32_t now)
{
    if (!item || item->activated)
        return 0;
    item->activated = 1;
    if (item->period_max)
        item->next_at = next_pickup(item, now);
    else
        spawn_pickup(item, now);
    return 1;
}

void bh_network_world_tick(BHNetworkWorld *world, uint32_t now, BHNetworkPickupUpdate update, void *user)
{
    for (unsigned int i = 0; i < world->pickup_count; ++i)
    {
        BHNetworkPickup *item = world->pickups + i;
        if ((!item->activation || item->activation == ARENA_START) && bh_network_world_activate_pickup(item, now))
            update(user, item, now);
        if (will_spawn(item) && !ENET_TIME_LESS(now, item->next_at))
        {
            spawn_pickup(item, now);
            update(user, item, now);
        }
        else if (item->available && item->lifetime && !ENET_TIME_LESS(now, item->expires_at))
        {
            item->available = 0;
            update(user, item, now);
        }
    }
}

int bh_network_world_claim_pickup(BHNetworkPickup *item, unsigned int generation, uint32_t now,
                                  BHNetworkPickupAward award, void *user)
{
    if (!item || !item->available || item->generation != generation ||
        (item->lifetime && !ENET_TIME_LESS(now, item->expires_at)) || !award(user, item))
        return 0;
    item->available = 0;
    return 1;
}
