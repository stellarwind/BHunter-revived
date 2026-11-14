#include "bh_sound_queue.h"
#include "../Math/bh_rotation.h"
#include "../Math/bh_fp.h"
#include <stddef.h>
#include <float.h>
#include <string.h>

static_assert(offsetof(BHSoundQueue, listener_velocity) - offsetof(BHSoundQueue, listener) == 12,
              "listener position velocity");
static_assert(offsetof(BHSoundQueue, listener_right) - offsetof(BHSoundQueue, listener) == 24, "listener right offset");
static_assert(offsetof(BHSoundQueue, listener_active) - offsetof(BHSoundQueue, listener) == 36, "listener flag offset");
static int vector_ok(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
int bh_sound_listener(BHSoundQueue *sound_queue, unsigned long index, long enabled, const BHVec3 *position,
                      const BHVec3 *angles, const BHVec3 *velocity)
{
    BHMat34 rotation;
    const BHVec3 right = {1, 0, 0};
    if (!sound_queue || index != 0)
    {
        if (sound_queue)
            sound_queue->error = BH_SOUND_ARGUMENT;
        return 0;
    }
    if (enabled &&
        (!vector_ok(position) || !vector_ok(angles) || !vector_ok(velocity) || !bh_math_environment_supported()))
    {
        sound_queue->error = BH_SOUND_NUMERICAL;
        return 0;
    }
    sound_queue->listener_active = enabled;
    if (enabled)
    {
        memset(&rotation, 0, sizeof(rotation));
        rotation.m[0][0] = rotation.m[1][1] = rotation.m[2][2] = 1;
        bh_mat34_rotate_y(angles->x, &rotation);
        bh_mat34_rotate_x(angles->y, &rotation);
        bh_mat34_rotate_z(angles->z, &rotation);
        bh_mat34_transform_point(&rotation, &right, &sound_queue->listener_right);
        sound_queue->listener = *position;
        sound_queue->listener_velocity = *velocity;
    }
    sound_queue->error = BH_SOUND_OK;
    return 1;
}
int bh_sound_set_voice_limit(BHSoundQueue *sound_queue, long requested)
{
    if (!sound_queue)
        return 0;
    if (requested >= 0 && requested < 4)
        sound_queue->voice_limit_policy = (unsigned short)(requested ? requested : 2);
    else
        sound_queue->voice_limit_policy = 3;
    sound_queue->error = BH_SOUND_OK;
    return 1;
}
