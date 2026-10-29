#include <math.h>
#include <stddef.h>

/* Original 00434810 consumes these four entity fields, in this order.
   The public source adapter supplies only those borrowed control values. */
typedef struct BHActorCurveLengthImage
{
    unsigned char unused[0x120];
    BHVec3 first, last, center, first_tangent, last_tangent;
} BHActorCurveLengthImage;
typedef char curve_length_last[offsetof(BHActorCurveLengthImage, last) == 0x12c ? 1 : -1];
typedef char curve_length_tangent[offsetof(BHActorCurveLengthImage, first_tangent) == 0x144 ? 1 : -1];
typedef char curve_length_last_tangent[offsetof(BHActorCurveLengthImage, last_tangent) == 0x150 ? 1 : -1];

float __cdecl bh_actor_curve_length_core(const BHActorCurveLengthImage *actor)
{
    BHVec3 previous, current;
    float sum = 0, t;
    bh_camera_curve(0, &current, &actor->first, &actor->first_tangent, &actor->last_tangent, &actor->last);
    t = .25f;
    do
    {
        BHVec3 delta;
        previous = current;
        bh_camera_curve(t, &current, &actor->first, &actor->first_tangent, &actor->last_tangent, &actor->last);
        delta.x = current.x - previous.x;
        delta.y = current.y - previous.y;
        delta.z = current.z - previous.z;
        sum += sqrt((delta.x * delta.x + delta.y * delta.y) + delta.z * delta.z);
        t -= -.25;
    } while (t < 1);
    return sum;
}
