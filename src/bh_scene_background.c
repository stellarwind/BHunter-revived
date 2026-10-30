#include "bh_scene_background.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef char slot_size[sizeof(BHSceneBackgroundSlot) == 48 ? 1 : -1];
typedef char slot_model_offset[offsetof(BHSceneBackgroundSlot, model) == 0x14 ? 1 : -1];
static const double tick = 0.03333333333333333;
static const double random_angle = 0.0001953125;
static void reset(BHSceneBackground *c, BHSceneBackgroundSlot *s, int fresh)
{
    double angle, other, dx, dz;
    float fx, fz, distance;
    angle = (double)rand() * random_angle;
    s->start.x = (float)(sin(angle) * 2500.0);
    s->start.y = 0;
    s->start.z = (float)(cos(angle) * 2500.0);
    other = angle - (-2.641592653589793 - (double)rand() * 0.000030517578125);
    s->end.x = (float)(sin(other) * 2500.0);
    s->end.y = 0;
    s->end.z = (float)(cos(other) * 2500.0);
    dx = (double)s->end.x - s->start.x;
    dz = (double)s->end.z - s->start.z;
    fx = (float)dx;
    fz = (float)dz;
    s->yaw = (float)atan2(dx, dz);
    distance = (float)sqrt((double)fz * fz + (double)fx * fx);
    s->height = (float)(-((double)rand() * distance * 0.000003509521484375 - (-175.0)));
    s->duration = (float)((double)distance / ((.75 - (double)rand() * (-0.0000152587890625)) * 40.0));
    if (fresh)
    {
        s->elapsed = (float)((double)rand() * s->duration * 0.000030517578125);
        s->delay = 0;
        if (rand() & 1)
            s->delay = (float)(tick - (double)rand() * (-0.001953125));
    }
    else
        s->elapsed = 0;
    s->model = c->first_model;
    if (rand() > 5000)
    {
        s->model = c->second_model;
        s->duration = s->duration * .25f;
    }
}
int __cdecl bh_scene_background_init(BHSceneBackground *c, BHDrawQueue *queue, unsigned long first_model,
                                     unsigned long second_model, unsigned long first_flare, unsigned long second_flare)
{
    unsigned long i;
    if (!c || !queue)
        return 0;
    memset(c, 0, sizeof(*c));
    c->queue = queue;
    c->first_model = first_model;
    c->second_model = second_model;
    c->first_flare = first_flare;
    c->second_flare = second_flare;
    for (i = 0; i < 8; ++i)
        c->slots[i].elapsed = -1;
    return 1;
}
static void result(int *out, int value)
{
    if (value < 0)
        *out = -1;
    else if (!value && *out > 0)
        *out = 0;
}
static void flares(BHSceneBackground *c, BHVec3 *point, double base, float alpha, int *out)
{
    float size, opacity;
    int r;
    size = (float)(base - (double)rand() * (-0.00048828125));
    r = rand();
    opacity = (float)((.5 - (double)r * (-0.00000762939453125)) * alpha);
    result(out, bh_draw_billboard(c->queue, 3, c->first_flare, point, size, size, 0, 0, opacity));
    size = (float)(base - (double)rand() * (-0.00048828125));
    r = rand();
    opacity = (float)((.5 - (double)r * (-0.00000762939453125)) * alpha);
    result(out, bh_draw_billboard(c->queue, 3, c->second_flare, point, size, size, 0, 0, opacity));
}
int __cdecl bh_scene_background_step(BHSceneBackground *c, unsigned long index, const BHVec3 *eye)
{
    BHSceneBackgroundSlot *s;
    BHVec3 position, angles;
    double elapsed, remaining, ratio_exact, bump, fade_exact, sine, cosine;
    float ratio, complement, fade, vertical;
    int out = 1;
#ifdef BH_SDL_BACKEND
    unsigned long mark;
#endif
    if (!c || !c->queue || !eye || index >= 8)
        return -1;
    s = c->slots + index;
    if (s->elapsed < 0)
        reset(c, s, 1);
    if (s->delay > 0)
    {
        remaining = (double)s->delay - tick;
        s->delay = (float)remaining;
        if (remaining > 0)
            return 1;
        reset(c, s, 0);
    }
    elapsed = (double)s->elapsed - (-0.03333333333333333);
    s->elapsed = (float)elapsed;
    if (!(elapsed < s->duration))
    {
        s->delay = (float)(tick - (double)rand() * (-0.001953125));
        return 1;
    }
    ratio_exact = elapsed / s->duration;
    ratio = (float)ratio_exact;
    complement = (float)(1.0 - (double)ratio);
    bump = ratio_exact + ratio_exact;
    if (bump > 1)
        bump = 2 - bump;
    fade_exact = ((double)ratio - .4) * 5;
    fade = (float)fade_exact;
    if (fade_exact > 1)
        fade = (float)(2.0 - (double)fade);
    position.x = (float)((double)s->end.x * ratio + (double)s->start.x * complement - (double)eye->x * (-.9));
    vertical = (float)((double)s->height * bump);
    position.y = (float)((double)eye->y + vertical);
    position.z = (float)((double)s->end.z * ratio + (double)s->start.z * complement - (double)eye->z * (-.9));
    angles.x = (float)((double)s->yaw - (-3.141592653589793));
    angles.y = angles.z = 0;
#ifdef BH_SDL_BACKEND
    mark = bh_interpolation_mark();
#endif
    result(&out, bh_draw_submit_resource(c->queue, 0, s->model, &position, &angles, 1));
    if (fade > 0)
    {
        if (s->model == c->first_model)
        {
            sine = sin((double)s->yaw);
            cosine = cos((double)s->yaw);
            position.z = (float)((double)position.z - cosine * 112);
            position.x = (float)((double)position.x - sine * 112);
            flares(c, &position, 50, fade, &out);
            position.z = (float)((double)position.z - sin((double)s->yaw) * 20);
            position.x = (float)((double)position.x - cos((double)s->yaw) * (-20));
            flares(c, &position, 25, fade, &out);
            position.z = (float)((double)position.z - sin((double)s->yaw) * (-40));
            position.x = (float)((double)position.x - cos((double)s->yaw) * 40);
            flares(c, &position, 25, fade, &out);
        }
        else
        {
            position.z = (float)((double)position.z - cos((double)s->yaw) * 35);
            position.x = (float)((double)position.x - sin((double)s->yaw) * 35);
            flares(c, &position, 40, fade, &out);
        }
    }
#ifdef BH_SDL_BACKEND
    bh_interpolation_stamp(mark, (unsigned long)s, 0);
#endif
    return out;
}
