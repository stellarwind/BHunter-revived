#include "bh_effect_cleanup.h"
#include <string.h>
int bh_effect_cleanup_bind(BHEffectCleanupContext *ctx, BHEntity *entity, unsigned long handle)
{
    if (!ctx || !ctx->pool || !ctx->sound || !entity)
        return 0;
    if (!bh_entity_is_active(ctx->pool, entity))
        return 0;
    memcpy(&entity->bindings.effect_cleanup, &ctx, 4);
    memcpy((char *)&entity->effect.sound, &handle, 4);
    entity->step = bh_effect_sound_cleanup;
    return 1;
}
void bh_effect_sound_cleanup(BHEntity *entity)
{
    BHEffectCleanupContext *ctx;
    unsigned long sound;
    memcpy(&ctx, &entity->bindings.effect_cleanup, 4);
    memcpy(&sound, (char *)&entity->effect.sound, 4);
    bh_sound_stop(ctx->sound, sound);
    bh_entity_release(ctx->pool, entity);
}
