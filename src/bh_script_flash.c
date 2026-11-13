#include "bh_script_flash.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4, "flash layout");
static BHEntityPool *pool(BHEntity *entity)
{
    BHEntityPool *pool;
    memcpy(&pool, &entity->bindings.flash_pool, 4);
    return pool;
}
float bh_script_flash_countdown(const BHEntity *entity)
{
    float f = 0;
    if (entity)
        memcpy(&f, &entity->flash.countdown, 4);
    return f;
}
int bh_script_flash_bind(BHEntityPool *pool, BHEntity *entity, float countdown)
{
    if (!bh_entity_is_active(pool, entity))
        return 0;
    memcpy(&entity->flash.countdown, &countdown, 4);
    memcpy(&entity->bindings.flash_pool, &pool, 4);
    entity->step = bh_script_flash_tick;
    return 1;
}
void bh_script_flash_tick(BHEntity *entity)
{
    float f;
    double value;
    if (!bh_math_environment_supported())
        return;
    value = (double)bh_script_flash_countdown(entity) - (1.0 / 30.0);
    bh_round_float(&f, value);
    memcpy(&entity->flash.countdown, &f, 4);
    if (!(value > 0))
        bh_entity_release(pool(entity), entity);
}
