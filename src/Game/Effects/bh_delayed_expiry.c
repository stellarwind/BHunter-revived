#include "bh_delayed_expiry.h"
#include "../../Math/bh_round.h"
#include <string.h>
#include <float.h>
int bh_delayed_expiry_bind(BHEntityPool *pool, BHEntity *entity, float lifetime, float delay)
{
    if (!pool || !entity)
        return 0;
    memcpy((char *)&entity->puff.lifetime, &lifetime, 4);
    memcpy((char *)&entity->puff.delay, &delay, 4);
    memcpy(&entity->bindings.expiry_pool, &pool, 4);
    entity->step = bh_delayed_expiry_tick;
    return 1;
}
void bh_delayed_expiry_tick(BHEntity *entity)
{
    BHEntityPool *pool;
    float old, stored;
    double next;
    memcpy(&pool, &entity->bindings.expiry_pool, 4);
    memcpy(&old, (char *)&entity->puff.delay, 4);
    if (!_isnan(old) && old > 0)
    {
        bh_round_float(&stored, (double)old - 1.0 / 30.0);
        memcpy((char *)&entity->puff.delay, &stored, 4);
        return;
    }
    memcpy(&old, (char *)&entity->puff.lifetime, 4);
    next = (double)old - 1.0 / 30.0;
    bh_round_float(&stored, next);
    memcpy((char *)&entity->puff.lifetime, &stored, 4);
    if (_isnan(next) || next < 0)
        bh_entity_release(pool, entity);
}
