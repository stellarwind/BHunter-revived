#include "bh_delayed_expiry.h"
#include "bh_round.h"
#include <string.h>
#include <float.h>
int bh_delayed_expiry_bind(BHEntityPool *p, BHEntity *e, float lifetime, float delay)
{
    if (!p || !e)
        return 0;
    memcpy((char *)&e->puff.lifetime, &lifetime, 4);
    memcpy((char *)&e->puff.delay, &delay, 4);
    memcpy(&e->bindings.expiry_pool, &p, 4);
    e->step = bh_delayed_expiry_tick;
    return 1;
}
void bh_delayed_expiry_tick(BHEntity *e)
{
    BHEntityPool *p;
    float old, stored;
    double next;
    memcpy(&p, &e->bindings.expiry_pool, 4);
    memcpy(&old, (char *)&e->puff.delay, 4);
    if (!_isnan(old) && old > 0)
    {
        bh_round_float(&stored, (double)old - 1.0 / 30.0);
        memcpy((char *)&e->puff.delay, &stored, 4);
        return;
    }
    memcpy(&old, (char *)&e->puff.lifetime, 4);
    next = (double)old - 1.0 / 30.0;
    bh_round_float(&stored, next);
    memcpy((char *)&e->puff.lifetime, &stored, 4);
    if (_isnan(next) || next < 0)
        bh_entity_release(p, e);
}
