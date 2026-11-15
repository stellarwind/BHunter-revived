#ifndef BH_LEVEL_RECORDS_H
#define BH_LEVEL_RECORDS_H
#include <stdint.h>

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

typedef struct BHLevelRecords
{
    BHLevelPlacement *placements;
    BHLevelPlacementOverride *overrides;
    uint32_t placement_count, override_count;
} BHLevelRecords;

/* Load the mounted custom level or the stock arena into an empty record set.
   Overrides retain their file order, including repeated placement IDs. */
int bh_level_records_load(BHLevelRecords *, int custom_level);
int bh_level_records_allocate(BHLevelRecords *, uint32_t placement_count, uint32_t override_count);
void bh_level_records_dispose(BHLevelRecords *);
#endif
