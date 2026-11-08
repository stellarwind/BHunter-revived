#include "bh_effect_draw.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
static int valid(BHEffectDrawContext *ctx, const BHEntity *entity)
{
    return ctx && ctx->queue && entity && bh_math_environment_supported();
}
static void combine(int *status, int result)
{
    if (result < 0)
        *status = -1;
    else if (!result && *status > 0)
        *status = 0;
}
static BHVec3 angles(const BHEntity *entity, float roll)
{
    BHVec3 velocity, result;
    memcpy(&velocity, (const unsigned char *)entity + 0xf8, 12);
    bh_round_float(&result.x, atan2((double)velocity.x, velocity.z));
    bh_round_float(&result.y,
                   atan2(-(double)velocity.y, sqrt((double)velocity.z * velocity.z + (double)velocity.x * velocity.x)));
    result.z = roll;
    return result;
}
int bh_effect_directional_draw(BHEffectDrawContext *ctx, const BHEntity *entity)
{
    BHVec3 rotation;
    float phase, angle;
    int status = 1;
    if (!valid(ctx, entity))
        return -1;
    rotation = angles(entity, 0);
    combine(&status, bh_draw_submit_resource(ctx->queue, 0, ctx->directional_model, &entity->position, &rotation, 1));
    memcpy(&phase, (const unsigned char *)entity + 0xe8, 4);
    bh_round_float(&angle, (double)phase * 1.5);
    combine(&status, bh_flare_queue(ctx->queue, &entity->position, 4, 50, ctx->timed_texture, 0, 0, 0, angle));
    return status;
}
int bh_effect_variable_draw(BHEffectDrawContext *ctx, const BHEntity *entity)
{
    unsigned long model;
    float roll, strength, size;
    BHVec3 rotation;
    int status = 1;
    if (!valid(ctx, entity))
        return -1;
    memcpy(&model, (const unsigned char *)entity + 0x148, 4);
    if (model)
    {
        memcpy(&roll, (const unsigned char *)entity + 0xf0, 4);
        rotation = angles(entity, roll);
        combine(&status, bh_draw_submit_resource(ctx->queue, 0, model, &entity->position, &rotation, 1));
    }
    bh_round_float(&strength, (double)rand() * (1.0 / 128.0));
    bh_round_float(&size, (double)rand() * (1.0 / 16384.0));
    combine(&status, bh_flare_queue(ctx->queue, &entity->position, size, strength, ctx->random_texture, 0, 0, 0, 0));
    return status;
}
int bh_effect_fixed_draw(BHEffectDrawContext *ctx, const BHEntity *entity)
{
    BHVec3 rotation;
    int status = 1;
    if (!valid(ctx, entity))
        return -1;
    memcpy(&rotation, (const unsigned char *)entity + 0x3c, 12);
    combine(&status, bh_draw_submit_resource(ctx->queue, 0, ctx->fixed_model, &entity->position, &rotation, 1));
    combine(&status, bh_flare_queue(ctx->queue, &entity->position, 4, 250, ctx->fixed_texture, 0, 0, 0, 0));
    return status;
}
