#include "bh_level_records.h"
#include "bh_vfile.h"
#include <stddef.h>
#include <stdlib.h>

/* The level format and supported x86 builds are little-endian. */
static_assert(sizeof(BHLevelPlacement) == 32 && offsetof(BHLevelPlacement, coordinates) == 16,
              "level placement layout");
static_assert(sizeof(BHLevelPlacementOverride) == 20 && offsetof(BHLevelPlacementOverride, values) == 4,
              "level placement override layout");

int bh_level_records_load(BHLevelRecords *records, int custom_level)
{
    BHVFile *file = custom_level ? bh_vfile_open_level("level.lev") : bh_vfile_open("test.lev", "rb");
    BHLevelRecords loaded = {0};
    long length;
    uint32_t remaining;
    int ok = 0;
    if (!file)
        return 0;
    if (bh_vfile_seek(file, 0, SEEK_END) || (length = bh_vfile_tell(file)) < (long)sizeof(loaded.placement_count) ||
        bh_vfile_seek(file, 0, SEEK_SET) || bh_vfile_read(&loaded.placement_count, sizeof(loaded.placement_count), 1, file) != 1)
        goto done;
    remaining = (uint32_t)length - sizeof(loaded.placement_count);
    /* Multiplayer placement IDs fit in 16 bits. */
    if (!loaded.placement_count || loaded.placement_count > UINT16_MAX ||
        loaded.placement_count > remaining / sizeof(*loaded.placements))
        goto done;
    remaining -= loaded.placement_count * sizeof(*loaded.placements);
    if (remaining % sizeof(*loaded.overrides))
        goto done;
    loaded.override_count = remaining / sizeof(*loaded.overrides);

    loaded.placements = malloc(loaded.placement_count * sizeof(*loaded.placements));
    if (!loaded.placements ||
        bh_vfile_read(loaded.placements, sizeof(*loaded.placements), loaded.placement_count, file) != loaded.placement_count)
        goto done;
    if (loaded.override_count)
    {
        loaded.overrides = malloc(remaining);
        if (!loaded.overrides ||
            bh_vfile_read(loaded.overrides, sizeof(*loaded.overrides), loaded.override_count, file) != loaded.override_count)
            goto done;
    }
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
