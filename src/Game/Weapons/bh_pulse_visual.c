#include "../Entities/bh_field_access.h"
#include "bh_pulse_visual.h"
#include "../../Math/bh_round.h"
#include "../../Math/bh_fp.h"
#include <string.h>
#include <math.h>
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static void result(BHPulseVisualContext *context, int n)
{
    ++context->attempted;
    if (n > 0)
        ++context->queued;
    else if (n < 0)
        context->error = -1;
    else if (!context->error)
        context->error = 1;
}
int bh_pulse_visual_draw(BHPulseVisualContext *context, const BHEntity *projectile)
{
    BHVec3 angles, direction, position;
    float lifetime, phase, envelope, offset, size;
    unsigned long i;
    if (!context || !projectile || !context->queue || !bh_math_environment_supported())
        return -1;
    context->error = 0;
    lifetime = bh_field_f32(&projectile->projectile.countdown);
    phase = bh_field_f32(&projectile->pulse.phase);
    angles.x = stored((double)phase + phase);
    angles.y = stored(sin((double)phase * 1.7) * 1.5);
    angles.z = 0;
    direction.x = stored(-sin((double)angles.x) * cos((double)angles.y));
    direction.y = stored(sin((double)angles.y));
    direction.z = stored(-cos((double)angles.x) * cos((double)angles.y));
    envelope = stored(.1 / (pow((double)lifetime, 3) - (-.001)));
    if (lifetime >= 2)
        envelope = 0;
    else
        angles.z = stored((double)lifetime * (-3));
    offset = stored(-(double)envelope * .5);
    for (i = 0; i < 3; i++)
        ((float *)&position)[i] =
            stored((double)((const float *)&projectile->position)[i] + (double)((float *)&direction)[i] * offset);
    result(context, bh_draw_submit_resource(context->queue, 0, context->model, &position, &angles, 1));
    offset = stored((double)envelope * .5);
    angles.x = stored((double)angles.x - (-3.141592653589793));
    angles.y = -angles.y;
    for (i = 0; i < 3; i++)
        ((float *)&position)[i] =
            stored((double)((const float *)&projectile->position)[i] + (double)((float *)&direction)[i] * offset);
    result(context, bh_draw_submit_resource(context->queue, 0, context->model, &position, &angles, 1));
    size = stored(.8 - (double)envelope * (-2));
    result(context, bh_draw_billboard(context->queue, 3, context->image, &projectile->position, size, size, -lifetime, 0, 1));
    if (lifetime < 2)
        result(context, bh_flare_queue(context->queue, &projectile->position, 20, stored((double)envelope * 25), context->image, 0, 0, 0,
                                 stored((double)lifetime + lifetime)));
    return context->error < 0 ? -1 : !context->error;
}
