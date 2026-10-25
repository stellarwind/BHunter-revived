#include "bh_sound_queue.h"
#include "bh_rotation.h"
#include "bh_fp.h"
#include <stddef.h>
#include <float.h>
#include <string.h>

typedef char listener_position_velocity[offsetof(BHSoundQueue,listener_velocity)-offsetof(BHSoundQueue,listener)==12 ? 1:-1];
typedef char listener_right_offset[offsetof(BHSoundQueue,listener_right)-offsetof(BHSoundQueue,listener)==24 ? 1:-1];
typedef char listener_flag_offset[offsetof(BHSoundQueue,listener_active)-offsetof(BHSoundQueue,listener)==36 ? 1:-1];
static int vector_ok(const BHVec3 *v) {return v && _finite(v->x) && _finite(v->y) && _finite(v->z);}
int __cdecl bh_sound_listener(BHSoundQueue *q,unsigned long index,long enabled,
    const BHVec3 *position,const BHVec3 *angles,const BHVec3 *velocity) {
    BHMat34 rotation;const BHVec3 right={1,0,0};
    if(!q || index!=0) {if(q)q->error=BH_SOUND_ARGUMENT;return 0;}
    if(enabled && (!vector_ok(position) || !vector_ok(angles) || !vector_ok(velocity) ||
       !bh_math_environment_supported())) {q->error=BH_SOUND_NUMERICAL;return 0;}
    q->listener_active=enabled;
    if(enabled) {
        memset(&rotation,0,sizeof(rotation));rotation.m[0][0]=rotation.m[1][1]=rotation.m[2][2]=1;
        bh_mat34_rotate_y(angles->x,&rotation);
        bh_mat34_rotate_x(angles->y,&rotation);
        bh_mat34_rotate_z(angles->z,&rotation);
        bh_mat34_transform_point(&rotation,&right,&q->listener_right);
        q->listener=*position;q->listener_velocity=*velocity;
    }
    q->error=BH_SOUND_OK;return 1;
}
int __cdecl bh_sound_set_voice_limit(BHSoundQueue *q,long requested) {
    if(!q) return 0;
    if(requested>=0 && requested<4)q->voice_limit_policy=(unsigned short)(requested ? requested:2);
    else q->voice_limit_policy=3;
    q->error=BH_SOUND_OK;return 1;
}
