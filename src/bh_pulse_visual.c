#include "bh_pulse_visual.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <string.h>
#include <math.h>
static float scalar(const BHEntity *e, unsigned long a)
{
    float f;
    memcpy(&f, (const unsigned char *)e + a, 4);
    return f;
}
static float stored(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static void result(BHPulseVisualContext *c, int n)
{
    ++c->attempted;
    if (n > 0)
        ++c->queued;
    else if (n < 0)
        c->error = -1;
    else if (!c->error)
        c->error = 1;
}
int __cdecl bh_pulse_visual_draw(BHPulseVisualContext *c, const BHEntity *e)
{
    BHVec3 angles, direction, position;
    float lifetime, phase, envelope, offset, size;
    unsigned long i;
    if (!c || !e || !c->queue || !bh_math_environment_supported())
        return -1;
    c->error = 0;
    lifetime = scalar(e, 0xe8);
    phase = scalar(e, 0xec);
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
            stored((double)((const float *)&e->position)[i] + (double)((float *)&direction)[i] * offset);
    result(c, bh_draw_submit_resource(c->queue, 0, c->model, &position, &angles, 1));
    offset = stored((double)envelope * .5);
    angles.x = stored((double)angles.x - (-3.141592653589793));
    angles.y = -angles.y;
    for (i = 0; i < 3; i++)
        ((float *)&position)[i] =
            stored((double)((const float *)&e->position)[i] + (double)((float *)&direction)[i] * offset);
    result(c, bh_draw_submit_resource(c->queue, 0, c->model, &position, &angles, 1));
    size = stored(.8 - (double)envelope * (-2));
    result(c, bh_draw_billboard(c->queue, 3, c->image, &e->position, size, size, -lifetime, 0, 1));
    if (lifetime < 2)
        result(c, bh_flare_queue(c->queue, &e->position, 20, stored((double)envelope * 25), c->image, 0, 0, 0,
                                 stored((double)lifetime + lifetime)));
    return c->error < 0 ? -1 : !c->error;
}
