#include "bh_remote_render.h"
#include "bh_round.h"
#include "bh_fp.h"
#include "bh_rotation.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
static float stored(double value)
{
    float out;
    bh_round_float(&out, value);
    return out;
}
int bh_remote_predict_pose(const BHEntity *entity, unsigned short tick, BHVec3 *position, BHVec3 *angles)
{
    BHVec3 anchor[2], velocity[2], acceleration[2];
    unsigned short stamp, elapsed;
    long dt;
    unsigned long i, j;
    BHVec3 *out[2];
    double t, half_squared;
    if (!entity || !position || !angles || !bh_math_environment_supported())
        return 0;
    stamp = entity->remote.newest.tick;
    elapsed = (unsigned short)(tick - stamp);
    dt = elapsed & 0x8000 ? (long)elapsed - 65536 : (long)elapsed;
    anchor[0] = entity->remote.newest.position;
    anchor[1] = entity->remote.newest.angles;
    if (dt <= 0)
    {
        *position = anchor[0];
        *angles = anchor[1];
        return 1;
    }
    velocity[0] = entity->velocity;
    velocity[1] = entity->angular_velocity;
    acceleration[0] = entity->acceleration;
    acceleration[1] = entity->angular_acceleration;
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++)
            if (!_finite(bh_vec3_component(&anchor[j], i)) || !_finite(bh_vec3_component(&velocity[j], i)) ||
                !_finite(bh_vec3_component(&acceleration[j], i)))
                return 0;
    t = (double)dt;
    half_squared = (t * t) * .5;
    out[0] = position;
    out[1] = angles;
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++)
            bh_round_float((&out[j]->x) + i,
                           (double)bh_vec3_component(&velocity[j], i) * t + bh_vec3_component(&anchor[j], i));
    for (j = 0; j < 2; j++)
        for (i = 0; i < 3; i++)
            bh_round_float((&out[j]->x) + i,
                           (double)bh_vec3_component(&acceleration[j], i) * half_squared + (&out[j]->x)[i]);
    return 1;
}
static void queued(BHRemoteRender *render, int result)
{
    ++render->attempted;
    if (result == 0)
        ++render->dropped;
    else if (result < 0)
        render->error = 1;
}

static void draw_propulsion(BHRemoteRender *render, const BHEntity *entity, const BHVec3 *position,
                            const BHVec3 *angles)
{
    const unsigned char *channels = entity->remote.propulsion;
    BHMat34 pose = {{{1, 0, 0, position->x}, {0, 1, 0, position->y}, {0, 0, 1, position->z}}};
    bh_mat34_rotate_y(angles->x, &pose);
    bh_mat34_rotate_x(angles->y, &pose);
    bh_mat34_rotate_z(angles->z, &pose);
    for (unsigned long i = 0; i < BH_PLAYER_NOZZLE_COUNT; ++i, channels += 3)
    {
        const BHVec3 *base = bh_player_nozzles + i;
        float x = (float)channels[0] / BH_PROPULSION_WIRE_SCALE;
        float y = 2.0f * channels[1] / BH_PROPULSION_WIRE_SCALE;
        float z = (float)channels[2] / BH_PROPULSION_WIRE_SCALE;
        BHVec3 from, to, tip;
        float width;
        int result;
        if (!(channels[0] || channels[1] || channels[2]))
            continue;
        tip.x = base->x + (base->x < 0 ? -x : x);
        tip.y = base->y + y;
        tip.z = base->z + (base->z < 0 ? -z : z);
        bh_mat34_transform_point(&pose, base, &from);
        bh_mat34_transform_point(&pose, &tip, &to);
        width = .5f + sqrtf(x * x + y * y + z * z);
        result = bh_draw_segment(render->queue, 3, render->thrust_texture, &from, &to, .5f, width, 1, .75f, 0);
#ifdef BH_SDL_BACKEND
        if (result > 0)
            bh_interpolation_part(render->queue->count - 1, (unsigned long)entity, entity->generation, i);
#endif
        queued(render, result);
    }
}

int bh_remote_render(BHRemoteRender *render, BHEntity *entity)
{
    BHVec3 position, angles, velocity, normal, point, up = {0, -1, 0};
    unsigned long handle, resource, model, i, j, frames = 60;
    short slot;
    unsigned char flags;
    float angle;
    int value;
    if (!render || !entity || !render->queue || !render->sound)
        return 0;
    render->attempted = render->dropped = 0;
    render->error = 0;
    memcpy(&slot, (char *)&entity->remote.slot, sizeof(slot));
    if (slot < 0 || slot >= 8)
        return 0;
    flags = entity->remote.pose_flags;
    memcpy(&handle, (char *)&entity->remote.engine_sound, sizeof(handle));
    if (flags & BH_REMOTE_POSE_HIDDEN)
    {
        if (handle != BH_SOUND_INVALID)
        {
            bh_sound_stop(render->sound, handle);
            handle = BH_SOUND_INVALID;
            memcpy((char *)&entity->remote.engine_sound, &handle, sizeof(handle));
        }
        return 1;
    }
    if (!bh_remote_predict_pose(entity, render->tick, &position, &angles))
        return 0;
    model = flags & BH_REMOTE_POSE_WRECK ? render->wreck_model : render->models[slot];
    if (model || !(flags & BH_REMOTE_POSE_WRECK))
        queued(render, bh_draw_submit_resource(render->queue, 0, model, &position, &angles, 1));
    if (render->thrust_texture && !(flags & BH_REMOTE_POSE_WRECK))
        draw_propulsion(render, entity, &position, &angles);
    for (i = 0; i < 3; i++)
    {
        float component;
        component = bh_vec3_component(&entity->velocity, i);
        bh_vec3_set_component(&velocity, i, stored((double)component * 30.0));
    }
    memcpy((char *)&entity->remote.sound_frames, &frames, 4);
    memcpy(&resource, (char *)&entity->remote.engine_resource, 4);
    if (handle == BH_SOUND_INVALID)
    {
        handle = bh_sound_request(render->sound, resource, 1, 1, &position, &velocity);
        memcpy((char *)&entity->remote.engine_sound, &handle, 4);
    }
    else
        bh_sound_update(render->sound, handle, 1, 1, &position, &velocity);
    if (flags & BH_REMOTE_POSE_SHIELD)
        for (j = 0; j < 24; j++)
        {
            for (i = 0; i < 3; i++)
            {
                value = rand();
                bh_vec3_set_component(&normal, i, (float)(value - 16384));
            }
            bh_vec3_normalize(&normal);
            for (i = 0; i < 3; i++)
            {
                bh_vec3_set_component(&normal, i, stored((double)bh_vec3_component(&normal, i) * 3.0));
                bh_vec3_set_component(&point, i,
                                      stored((double)bh_vec3_component(&normal, i) + bh_vec3_component(&position, i)));
            }
            value = rand();
            angle = stored((double)value * .0001953125);
            queued(render, bh_draw_oriented(render->queue, 3, render->spark_texture, &point, &up, &normal, 3, 3, angle,
                                            8, .3f));
        }
    return !render->error;
}
