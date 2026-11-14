#include "bh_actor_vehicle_step.h"
#include <stddef.h>

void bh_actor_vehicle_step(BHEntity *entity)
{
    BHActorFields *timing = &entity->actor;
    bh_actor_attack_bound(entity);
    timing->phase = timing->phase - (-1.0 / 30.0);
    if (timing->phase >= timing->duration)
    {
        float remaining = timing->phase - timing->duration;
        bh_actor_pursuit_bound(entity);
        timing->phase = remaining;
    }
}
