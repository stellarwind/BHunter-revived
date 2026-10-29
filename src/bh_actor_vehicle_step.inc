#include "bh_actor_vehicle_step.h"
#include <stddef.h>

/* Verified original entity fields; no replacement entity or global layout. */
typedef struct BHActorVehicleTiming
{
    unsigned char other[0x180];
    float phase, duration;
} BHActorVehicleTiming;
typedef char timing_phase_offset[offsetof(BHActorVehicleTiming, phase) == 0x180 ? 1 : -1];
typedef char timing_duration_offset[offsetof(BHActorVehicleTiming, duration) == 0x184 ? 1 : -1];

void __cdecl bh_actor_vehicle_step(BHEntity *entity)
{
    BHActorVehicleTiming *timing = (BHActorVehicleTiming *)entity;
    bh_actor_attack_bound(entity);
    timing->phase = timing->phase - (-1.0 / 30.0);
    if (timing->phase >= timing->duration)
    {
        float remaining = timing->phase - timing->duration;
        bh_actor_pursuit_bound(entity);
        timing->phase = remaining;
    }
}
