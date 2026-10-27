#include "bh_physics_response.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stddef.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef char contact_packet_size[sizeof(BHPhysicsContactPacket) == 36 ? 1 : -1];
typedef char contact_packet_impulse[offsetof(BHPhysicsContactPacket, impulse) == 0x1c ? 1 : -1];
typedef char contact_packet_severity[offsetof(BHPhysicsContactPacket, severity) == 0x20 ? 1 : -1];
static int vector_ok(const BHVec3 *v)
{
    return v && _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int matrix_ok(const BHMat34 *m)
{
    unsigned long i;
    if (!m)
        return 0;
    for (i = 0; i < 12; ++i)
        if (!_finite(((const float *)m)[i]))
            return 0;
    return 1;
}
static double reverse_dot(const BHVec3 *a, const BHVec3 *b)
{
    return ((double)a->z * b->z + (double)a->y * b->y) + (double)a->x * b->x;
}
int __cdecl bh_physics_overlap(BHSweepQuery *q, const BHMat34 *pose, const BHMat34 *world, const BHMat34 *local,
                               BHCollisionHit *out)
{
    BHMat34 relative;
    if (!q || !q->initialized || q->busy || !pose || !world || !local || !out)
        return -1;
    if (!q->a || !q->b)
        return 0;
    bh_mat34_multiply(local, pose, &relative);
    return bh_collision_query_run(&q->collision, 0, 0, &relative, world, out);
}
int __cdecl bh_physics_prepare(BHPhysicsState *s, float dt, float radius, BHVec3 *movement, BHVec3 *angles,
                               float *broad_margin)
{
    BHPhysicsState next;
    BHVec3 delta, rotation;
    unsigned long i;
    float margin;
    if (!s || !movement || !angles || !broad_margin || !_finite(dt) || dt <= 0 || !_finite(radius) ||
        !matrix_ok(&s->pose) || !vector_ok(&s->velocity) || !vector_ok(&s->angular_velocity) ||
        !vector_ok(&s->acceleration) || !vector_ok(&s->angular_acceleration) || !bh_math_environment_supported())
        return 0;
    next = *s;
    for (i = 0; i < 3; ++i)
    {
        bh_round_float(((float *)&next.velocity) + i,
                       (double)((float *)&s->acceleration)[i] * dt + ((float *)&s->velocity)[i]);
        bh_round_float(((float *)&next.angular_velocity) + i,
                       (double)((float *)&s->angular_acceleration)[i] * dt + ((float *)&s->angular_velocity)[i]);
        bh_round_float(((float *)&delta) + i, (double)((float *)&next.velocity)[i] * dt);
        bh_round_float(((float *)&rotation) + i, (double)((float *)&next.angular_velocity)[i] * dt);
    }
    memset(&next.acceleration, 0, sizeof(next.acceleration));
    memset(&next.angular_acceleration, 0, sizeof(next.angular_acceleration));
    bh_round_float(&margin, ((double)radius - (-.5)) + sqrt(reverse_dot(&delta, &delta)));
    if (!vector_ok(&delta) || !vector_ok(&rotation) || !_finite(margin))
        return 0;
    *s = next;
    *movement = delta;
    *angles = rotation;
    *broad_margin = margin;
    return 1;
}
int __cdecl bh_physics_slide(const BHVec3 *movement, const BHSweepHit *contact, float coefficient, BHVec3 *out)
{
    BHVec3 slide;
    float factor;
    double normal_motion;
    unsigned long i;
    if (!vector_ok(movement) || !contact || !contact->surface_present || !out || !vector_ok(&contact->surface.normal) ||
        !_finite(contact->fraction) || !_finite(coefficient) || !bh_math_environment_supported())
        return 0;
    normal_motion = reverse_dot(movement, &contact->surface.normal);
    bh_round_float(&factor, (double)coefficient * (1.0 - contact->fraction));
    for (i = 0; i < 3; ++i)
        bh_round_float(((float *)&slide) + i,
                       (((const float *)movement)[i] - normal_motion * ((const float *)&contact->surface.normal)[i]) *
                           factor);
    if (!vector_ok(&slide))
        return 0;
    *out = slide;
    return 1;
}
int __cdecl bh_physics_contact_response(const BHSweepHit *hit, float slide_fraction, float dt, float a_mass,
                                        float b_mass, BHVec3 *acceleration, BHPhysicsResponse *out)
{
    BHPhysicsResponse next;
    BHVec3 force;
    const BHVec3 *normal;
    float factor, impulse, b_impulse, severity = 0, noise;
    double speed, normal_force, sum, a_impulse;
    unsigned long i, flags;
    int random_value;
    if (!hit || !hit->surface_present || !out || !vector_ok(acceleration) || !vector_ok(&hit->motion) ||
        !vector_ok(&hit->surface.normal) || !vector_ok(&hit->surface.contact.point) || !_finite(slide_fraction) ||
        !_finite(dt) || dt <= 0 || !_finite(a_mass) || !_finite(b_mass) || !bh_math_environment_supported())
        return 0;
    memset(&next, 0, sizeof(next));
    force = *acceleration;
    normal = &hit->surface.normal;
    flags = hit->surface.flags;
    next.material_flags = flags;
    speed = sqrt(reverse_dot(&hit->motion, &hit->motion)) / dt;
    bh_round_float(&next.speed, speed);
    bh_round_float(&factor, speed * 7.62939453125e-8);
    bh_round_float(&impulse, reverse_dot(normal, &hit->motion));
    for (i = 0; i < 3; ++i)
    {
        random_value = rand();
        if (i < 2)
        {
            bh_round_float(&noise, ((double)random_value - 16384.0) * factor);
            bh_round_float(((float *)&next.jittered_motion) + i, (double)((const float *)&hit->motion)[i] + noise);
        }
        else
            bh_round_float(&next.jittered_motion.z, (double)hit->motion.z + ((double)random_value - 16384.0) * factor);
    }
    normal_force = 30.0 - (double)next.speed * -.00125;
    for (i = 0; i < 3; ++i)
        bh_round_float(((float *)&force) + i, (double)((float *)&force)[i] + ((const float *)normal)[i] * normal_force);
    next.slide_fraction = slide_fraction;
    if (flags & 6)
    {
        normal_force = (flags & 2) ? -100.0 : -150.0;
        for (i = 0; i < 3; ++i)
        {
            bh_round_float(((float *)&force) + i,
                           (double)((float *)&force)[i] - ((const float *)normal)[i] * normal_force);
            bh_round_float(((float *)&next.jittered_motion) + i, (double)((float *)&next.jittered_motion)[i] * 4.0);
        }
    }
    else
    {
        if (flags & 8)
        {
            bh_round_float(&impulse, (double)impulse - (impulse < 0 ? .3 : -.3));
            if (next.slide_fraction < .01)
                next.slide_fraction = .01f;
        }
        bh_round_float(&next.audio_level, ((double)factor * next.slide_fraction) * 1048576.0);
        if (next.audio_level > 1)
            next.audio_level = 1;
        bh_round_float(&severity, fabs(((double)impulse * next.slide_fraction) / ((double)dt * 50.0)));
        if (severity > 1)
            severity = 1;
    }
    a_impulse = b_impulse = impulse;
    if (a_mass >= 0)
    {
        if (a_mass == 0)
        {
            if (b_mass == 0)
            {
                bh_round_float(&b_impulse, (double)impulse * .5);
                a_impulse = b_impulse;
            }
            else
                b_impulse = 0;
        }
        else
        {
            if (b_mass < 0)
                b_impulse = 0;
            else
            {
                sum = (double)b_mass + a_mass;
                a_impulse = ((double)impulse * b_mass) / sum;
                bh_round_float(&b_impulse, ((double)impulse * a_mass) / sum);
            }
        }
    }
    else
    {
        a_impulse = 0;
        if (b_mass < 0)
        {
            bh_round_float(&b_impulse, (double)impulse * .5);
            a_impulse = b_impulse;
        }
    }
    next.first.event = next.second.event = 2;
    next.first.point = next.second.point = hit->surface.contact.point;
    next.first.normal = *normal;
    next.second.normal.x = -normal->x;
    next.second.normal.y = -normal->y;
    next.second.normal.z = -normal->z;
    next.first.impulse = a_impulse;
    next.second.impulse = b_impulse;
    next.first.severity = next.second.severity = severity;
    next.emit_first = a_impulse != 0;
    next.emit_second = b_impulse != 0;
    *acceleration = force;
    *out = next;
    return 1;
}
int __cdecl bh_physics_finish(BHPhysicsState *s, const BHMat34 *previous, float dt, int no_collision)
{
    BHPhysicsState next;
    BHMat34 rotation;
    const BHVec3 forward = {0, 0, 1}, right = {1, 0, 0};
    BHVec3 direction;
    double inverse;
    unsigned long i;
    if (!s || !matrix_ok(&s->pose) || !matrix_ok(previous) || !vector_ok(&s->angular_velocity) ||
        !vector_ok(&s->acceleration) || !_finite(dt) || dt <= 0 || !bh_math_environment_supported())
        return 0;
    next = *s;
    if (no_collision)
        next.last_clear = s->pose;
    inverse = 1.0 / dt;
    for (i = 0; i < 3; ++i)
    {
        bh_round_float(((float *)&next.angular_velocity) + i, (double)((float *)&s->angular_velocity)[i] * .98);
        bh_round_float(((float *)&next.velocity) + i, (((double)s->pose.m[i][3] - previous->m[i][3]) * inverse) * .995);
    }
    bh_round_float(&next.acceleration.y, (double)s->acceleration.y - (-9.82));
    bh_round_float(&next.euler.x, atan2((double)s->pose.m[0][2], s->pose.m[2][2]));
    rotation = s->pose;
    bh_mat34_rotate_world_y(-next.euler.x, &rotation);
    bh_mat34_transform_direction(&rotation, &forward, &direction);
    bh_round_float(&next.euler.y, atan2(-(double)direction.y, direction.z));
    bh_mat34_rotate_world_x(-next.euler.y, &rotation);
    bh_mat34_transform_direction(&rotation, &right, &direction);
    bh_round_float(&next.euler.z, atan2((double)direction.y, direction.x));
    if (!vector_ok(&next.velocity) || !vector_ok(&next.euler))
        return 0;
    *s = next;
    return 1;
}
