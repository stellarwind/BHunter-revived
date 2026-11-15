#include "bh_network_world.h"
#include "../Assets/bh_vfile.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

enum { PLACEMENT_BYTES = 32, OVERRIDE_BYTES = 20 };
enum { SINGLE_PICKUP = 1, TIMED_PICKUP, ACTIVATED_TIMED_PICKUP };

/* The level format and the supported x86 builds are little-endian. */
typedef struct BHLevelPlacement
{
    char model[16];
    int32_t coordinates[4];
} BHLevelPlacement;

typedef struct BHLevelPlacementOverride
{
    uint32_t placement;
    int16_t values[8];
} BHLevelPlacementOverride;

static_assert(sizeof(BHLevelPlacement) == PLACEMENT_BYTES && offsetof(BHLevelPlacement, coordinates) == 16,
              "level placement layout");
static_assert(sizeof(BHLevelPlacementOverride) == OVERRIDE_BYTES && offsetof(BHLevelPlacementOverride, values) == 4,
              "level placement override layout");

static int read(BHVFile *file, void *record, unsigned long size)
{
    return bh_vfile_read(record, size, 1, file) == 1;
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

int bh_network_world_load(BHNetworkWorld *world, int custom_level)
{
    static const char *const models_by_type[] = {
        [SINGLE_PICKUP] = "POWERUP.BSP",
        [TIMED_PICKUP] = "POWERUP2.BSP",
        [ACTIVATED_TIMED_PICKUP] = "POWERUP3.BSP",
    };
    BHVFile *file = custom_level ? bh_vfile_open_level("level.lev") : bh_vfile_open("test.lev", "rb");
    unsigned char *models = NULL;
    BHNetworkPickup *items = NULL;
    long length;
    uint32_t placements, extra;
    unsigned int count = 0;
    int ok = 0;
    if (!file)
        return 0;
    if (bh_vfile_seek(file, 0, SEEK_END) || (length = bh_vfile_tell(file)) < (long)sizeof(placements) ||
        bh_vfile_seek(file, 0, SEEK_SET) || !read(file, &placements, sizeof(placements)))
        goto done;
    extra = (uint32_t)length - sizeof(placements);
    if (!placements || placements > UINT16_MAX || placements > extra / PLACEMENT_BYTES)
        goto done;
    extra -= placements * PLACEMENT_BYTES;
    if (extra % OVERRIDE_BYTES)
        goto done;
    models = calloc(placements, sizeof(*models));
    items = calloc(placements, sizeof(*items));
    if (!models || !items)
        goto done;
    for (unsigned int i = 0; i < placements; ++i)
    {
        BHLevelPlacement entry;
        if (!read(file, &entry, sizeof(entry)))
            goto done;
        for (unsigned int model = SINGLE_PICKUP; model <= ACTIVATED_TIMED_PICKUP; ++model)
            if (!_strnicmp(entry.model, models_by_type[model], sizeof(entry.model)))
            {
                models[i] = (unsigned char)model;
                break;
            }
    }
    for (; extra; extra -= OVERRIDE_BYTES)
    {
        BHLevelPlacementOverride entry;
        if (!read(file, &entry, sizeof(entry)))
            goto done;
        if (entry.placement >= placements || !models[entry.placement])
            continue;
        items[entry.placement] = definition(models[entry.placement], entry.placement, entry.values);
    }
    /* The shipped maps specify the pickup variables in their placement
       overrides. Leave markers without a placement override to their script. */
    for (unsigned int i = 0; i < placements; ++i)
        if (models[i] && items[i].total && items[i].total <= INT16_MAX && items[i].offer &&
            items[i].offer <= INT16_MAX && items[i].type <= 21 && items[i].price <= INT16_MAX &&
            items[i].lifetime <= INT16_MAX && items[i].period_max <= INT16_MAX &&
            items[i].period_min <= items[i].period_max && (items[i].type != 21 || items[i].price))
            items[count++] = items[i];
    free(world->pickups);
    world->pickups = items;
    items = NULL;
    world->pickup_count = count;
    world->placement_count = placements;
    ok = 1;
done:
    free(items);
    free(models);
    bh_vfile_close(file);
    return ok;
}
