#include "bh_level_records.h"
#include "bh_vfile.h"
#include <stddef.h>
#include <stdlib.h>

/* The level format and supported x86 builds are little-endian. */
static_assert(sizeof(BHLevelPlacement) == 32 && offsetof(BHLevelPlacement, coordinates) == 16,
              "level placement layout");
static_assert(sizeof(BHLevelPlacementOverride) == 20 && offsetof(BHLevelPlacementOverride, values) == 4,
              "level placement override layout");

int bh_level_records_allocate(BHLevelRecords *records, uint32_t placement_count, uint32_t override_count)
{
    BHLevelRecords allocated = {.placement_count = placement_count, .override_count = override_count};
    allocated.placements = calloc(placement_count, sizeof(*allocated.placements));
    if (!allocated.placements)
        return 0;
    if (override_count)
    {
        allocated.overrides = calloc(override_count, sizeof(*allocated.overrides));
        if (!allocated.overrides)
        {
            free(allocated.placements);
            return 0;
        }
    }
    *records = allocated;
    return 1;
}

int bh_level_records_load(BHLevelRecords *records, int custom_level)
{
    BHVFile *file = custom_level ? bh_vfile_open_level("level.lev") : bh_vfile_open("test.lev", "rb");
    BHLevelRecords loaded = {0};
    long length;
    uint32_t placement_count, remaining;
    int ok = 0;
    if (!file)
        return 0;
    if (bh_vfile_seek(file, 0, SEEK_END) || (length = bh_vfile_tell(file)) < (long)sizeof(placement_count) ||
        bh_vfile_seek(file, 0, SEEK_SET) || bh_vfile_read(&placement_count, sizeof(placement_count), 1, file) != 1)
        goto done;
    remaining = (uint32_t)length - sizeof(placement_count);
    /* Multiplayer placement IDs fit in 16 bits. */
    if (!placement_count || placement_count > UINT16_MAX || placement_count > remaining / sizeof(*loaded.placements))
        goto done;
    remaining -= placement_count * sizeof(*loaded.placements);
    if (remaining % sizeof(*loaded.overrides))
        goto done;
    if (!bh_level_records_allocate(&loaded, placement_count, remaining / sizeof(*loaded.overrides)) ||
        bh_vfile_read(loaded.placements, sizeof(*loaded.placements), loaded.placement_count, file) != loaded.placement_count)
        goto done;
    if (loaded.override_count &&
        bh_vfile_read(loaded.overrides, sizeof(*loaded.overrides), loaded.override_count, file) != loaded.override_count)
        goto done;
    *records = loaded;
    ok = 1;
done:
    if (!ok)
        bh_level_records_dispose(&loaded);
    bh_vfile_close(file);
    return ok;
}

void bh_level_records_dispose(BHLevelRecords *records)
{
    free(records->placements);
    free(records->overrides);
    *records = (BHLevelRecords){0};
}
