#include "bh_network_world.h"
#include <stdlib.h>
#include <string.h>

enum { SINGLE_PICKUP = 1, TIMED_PICKUP, ACTIVATED_TIMED_PICKUP };

static unsigned int pickup_model(const BHLevelPlacement *entry)
{
    static const char *const models[] = {
        [SINGLE_PICKUP] = "POWERUP.BSP",
        [TIMED_PICKUP] = "POWERUP2.BSP",
        [ACTIVATED_TIMED_PICKUP] = "POWERUP3.BSP",
    };
    for (unsigned int model = SINGLE_PICKUP; model <= ACTIVATED_TIMED_PICKUP; ++model)
        if (!_strnicmp(entry->model, models[model], sizeof(entry->model)))
            return model;
    return 0;
}

/* These three retail placement models define map pickups. Zero stock denotes
   an unlimited offer, including shops, and stays in the normal script VM. */
static BHNetworkPickup definition(unsigned int model, unsigned int placement, const int16_t *values)
{
    BHNetworkPickup item = {.placement = (uint16_t)placement};
    item.type = (uint16_t)values[0];
    item.total = (uint16_t)values[1];
    item.offer = (uint16_t)values[2];
    if (model == ACTIVATED_TIMED_PICKUP)
    {
        item.offer = item.total;
        item.lifetime = (uint16_t)values[3];
        item.repeats = (uint16_t)values[4];
        item.period_min = (uint16_t)values[5];
        item.period_max = (uint16_t)values[6];
        item.activation = (uint16_t)values[7];
    }
    else
    {
        item.price = (uint16_t)values[3];
        item.lifetime = (uint16_t)values[4];
        if (model == SINGLE_PICKUP)
            item.activation = (uint16_t)values[5];
        else
        {
            item.repeats = (uint16_t)values[5];
            item.period_min = (uint16_t)values[6];
            item.period_max = (uint16_t)values[7];
        }
    }
    return item;
}

int bh_network_world_load(BHNetworkWorld *world, const BHLevelRecords *level)
{
    BHNetworkPickup *items = calloc(level->placement_count, sizeof(*items));
    unsigned int count = 0;
    if (!items)
        return 0;
    for (uint32_t i = 0; i < level->override_count; ++i)
    {
        const BHLevelPlacementOverride *entry = level->overrides + i;
        unsigned int model;
        if (entry->placement >= level->placement_count)
            continue;
        model = pickup_model(level->placements + entry->placement);
        if (model)
            items[entry->placement] = definition(model, entry->placement, entry->values);
    }
    /* The shipped maps specify the pickup variables in their placement
       overrides. Leave markers without a placement override to their script. */
    for (uint32_t i = 0; i < level->placement_count; ++i)
        if (items[i].total && items[i].total <= INT16_MAX && items[i].offer &&
            items[i].offer <= INT16_MAX && items[i].type <= 21 && items[i].price <= INT16_MAX &&
            items[i].lifetime <= INT16_MAX && items[i].period_max <= INT16_MAX &&
            items[i].period_min <= items[i].period_max && (items[i].type != 21 || items[i].price))
            items[count++] = items[i];
    free(world->pickups);
    world->pickups = items;
    world->pickup_count = count;
    world->placement_count = level->placement_count;
    return 1;
}
