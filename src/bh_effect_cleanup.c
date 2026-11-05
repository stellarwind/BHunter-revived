#include "bh_effect_cleanup.h"
#include <string.h>
int __cdecl bh_effect_cleanup_bind(BHEffectCleanupContext *ctx, BHEntity *entity, unsigned long handle)
{
    if (!ctx || !ctx->pool || !ctx->sound || !entity)
        return 0;
    if (!bh_entity_is_active(ctx->pool, entity))
        return 0;
    memcpy(entity->payload + 604, &ctx, 4);
    memcpy((char *)entity + 0x128, &handle, 4);
    entity->step = bh_effect_sound_cleanup;
    return 1;
}
void __cdecl bh_effect_sound_cleanup(BHEntity *entity)
{
    BHEffectCleanupContext *ctx;
    unsigned long sound;
    memcpy(&ctx, entity->payload + 604, 4);
    memcpy(&sound, (char *)entity + 0x128, 4);
    bh_sound_stop(ctx->sound, sound);
    bh_entity_release(ctx->pool, entity);
}
