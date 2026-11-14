#include "bh_scene_background.h"
#ifdef BH_SDL_BACKEND
#include "../Graphics/bh_render_interpolation.h"
#endif
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
static_assert(sizeof(BHSceneBackgroundSlot) == 48, "BHSceneBackgroundSlot size");
static_assert(offsetof(BHSceneBackgroundSlot, model) == 0x14, "BHSceneBackgroundSlot.model offset");
static const double tick = 0.03333333333333333;
static const double random_angle = 0.0001953125;
static void reset(BHSceneBackground *scene_background, BHSceneBackgroundSlot *scene_background_slot, int fresh)
{
    double angle, other, dx, dz;
    float fx, fz, distance;
    angle = (double)rand() * random_angle;
    scene_background_slot->start.x = (float)(sin(angle) * 2500.0);
    scene_background_slot->start.y = 0;
    scene_background_slot->start.z = (float)(cos(angle) * 2500.0);
    other = angle - (-2.641592653589793 - (double)rand() * 0.000030517578125);
    scene_background_slot->end.x = (float)(sin(other) * 2500.0);
    scene_background_slot->end.y = 0;
    scene_background_slot->end.z = (float)(cos(other) * 2500.0);
    dx = (double)scene_background_slot->end.x - scene_background_slot->start.x;
    dz = (double)scene_background_slot->end.z - scene_background_slot->start.z;
    fx = (float)dx;
    fz = (float)dz;
    scene_background_slot->yaw = (float)atan2(dx, dz);
    distance = (float)sqrt((double)fz * fz + (double)fx * fx);
    scene_background_slot->height = (float)(-((double)rand() * distance * 0.000003509521484375 - (-175.0)));
    scene_background_slot->duration = (float)((double)distance / ((.75 - (double)rand() * (-0.0000152587890625)) * 40.0));
    if (fresh)
    {
        scene_background_slot->elapsed = (float)((double)rand() * scene_background_slot->duration * 0.000030517578125);
        scene_background_slot->delay = 0;
        if (rand() & 1)
            scene_background_slot->delay = (float)(tick - (double)rand() * (-0.001953125));
    }
    else
        scene_background_slot->elapsed = 0;
    scene_background_slot->model = scene_background->first_model;
    if (rand() > 5000)
    {
        scene_background_slot->model = scene_background->second_model;
        scene_background_slot->duration = scene_background_slot->duration * .25f;
    }
}
int bh_scene_background_init(BHSceneBackground *scene_background, BHDrawQueue *queue, unsigned long first_model,
                             unsigned long second_model, unsigned long first_flare, unsigned long second_flare)
{
    unsigned long i;
    if (!scene_background || !queue)
        return 0;
    memset(scene_background, 0, sizeof(*scene_background));
    scene_background->queue = queue;
    scene_background->first_model = first_model;
    scene_background->second_model = second_model;
    scene_background->first_flare = first_flare;
    scene_background->second_flare = second_flare;
    for (i = 0; i < 8; ++i)
        scene_background->slots[i].elapsed = -1;
    return 1;
}
static void result(int *out, int value)
{
    if (value < 0)
        *out = -1;
    else if (!value && *out > 0)
        *out = 0;
}
static void flares(BHSceneBackground *scene_background, BHVec3 *point, double base, float alpha, int *out)
{
    float size, opacity;
    int r;
    size = (float)(base - (double)rand() * (-0.00048828125));
    r = rand();
    opacity = (float)((.5 - (double)r * (-0.00000762939453125)) * alpha);
    result(out, bh_draw_billboard(scene_background->queue, 3, scene_background->first_flare, point, size, size, 0, 0, opacity));
    size = (float)(base - (double)rand() * (-0.00048828125));
    r = rand();
    opacity = (float)((.5 - (double)r * (-0.00000762939453125)) * alpha);
    result(out, bh_draw_billboard(scene_background->queue, 3, scene_background->second_flare, point, size, size, 0, 0, opacity));
}
int bh_scene_background_step(BHSceneBackground *scene_background, unsigned long index, const BHVec3 *eye)
{
    BHSceneBackgroundSlot *scene_background_slot;
    BHVec3 position, angles;
    double elapsed, remaining, ratio_exact, bump, fade_exact, sine, cosine;
    float ratio, complement, fade, vertical;
    int out = 1;
#ifdef BH_SDL_BACKEND
    unsigned long mark;
#endif
    if (!scene_background || !scene_background->queue || !eye || index >= 8)
        return -1;
    scene_background_slot = scene_background->slots + index;
    if (scene_background_slot->elapsed < 0)
        reset(scene_background, scene_background_slot, 1);
    if (scene_background_slot->delay > 0)
    {
        remaining = (double)scene_background_slot->delay - tick;
        scene_background_slot->delay = (float)remaining;
        if (remaining > 0)
            return 1;
        reset(scene_background, scene_background_slot, 0);
    }
    elapsed = (double)scene_background_slot->elapsed - (-0.03333333333333333);
    scene_background_slot->elapsed = (float)elapsed;
    if (!(elapsed < scene_background_slot->duration))
    {
        scene_background_slot->delay = (float)(tick - (double)rand() * (-0.001953125));
        return 1;
    }
    ratio_exact = elapsed / scene_background_slot->duration;
    ratio = (float)ratio_exact;
    complement = (float)(1.0 - (double)ratio);
    bump = ratio_exact + ratio_exact;
    if (bump > 1)
        bump = 2 - bump;
    fade_exact = ((double)ratio - .4) * 5;
    fade = (float)fade_exact;
    if (fade_exact > 1)
        fade = (float)(2.0 - (double)fade);
    position.x = (float)((double)scene_background_slot->end.x * ratio + (double)scene_background_slot->start.x * complement - (double)eye->x * (-.9));
    vertical = (float)((double)scene_background_slot->height * bump);
    position.y = (float)((double)eye->y + vertical);
    position.z = (float)((double)scene_background_slot->end.z * ratio + (double)scene_background_slot->start.z * complement - (double)eye->z * (-.9));
    angles.x = (float)((double)scene_background_slot->yaw - (-3.141592653589793));
    angles.y = angles.z = 0;
#ifdef BH_SDL_BACKEND
    mark = bh_interpolation_mark();
#endif
    result(&out, bh_draw_submit_resource(scene_background->queue, 0, scene_background_slot->model, &position, &angles, 1));
    if (fade > 0)
    {
        if (scene_background_slot->model == scene_background->first_model)
        {
            sine = sin((double)scene_background_slot->yaw);
            cosine = cos((double)scene_background_slot->yaw);
            position.z = (float)((double)position.z - cosine * 112);
            position.x = (float)((double)position.x - sine * 112);
            flares(scene_background, &position, 50, fade, &out);
            position.z = (float)((double)position.z - sin((double)scene_background_slot->yaw) * 20);
            position.x = (float)((double)position.x - cos((double)scene_background_slot->yaw) * (-20));
            flares(scene_background, &position, 25, fade, &out);
            position.z = (float)((double)position.z - sin((double)scene_background_slot->yaw) * (-40));
            position.x = (float)((double)position.x - cos((double)scene_background_slot->yaw) * 40);
            flares(scene_background, &position, 25, fade, &out);
        }
        else
        {
            position.z = (float)((double)position.z - cos((double)scene_background_slot->yaw) * 35);
            position.x = (float)((double)position.x - sin((double)scene_background_slot->yaw) * 35);
            flares(scene_background, &position, 40, fade, &out);
        }
    }
#ifdef BH_SDL_BACKEND
    bh_interpolation_stamp(mark, (unsigned long)scene_background_slot, 0);
#endif
    return out;
}
