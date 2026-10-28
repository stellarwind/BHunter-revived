#include "bh_target_step.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <float.h>
#include <stdlib.h>
#include <string.h>
static BHTargetStepContext *context(BHEntity *e)
{
    BHTargetStepContext *c;
    memcpy(&c, e->payload + 616, 4);
    return c;
}
static int live(BHEntityPool *p, BHEntity *e, unsigned long generation)
{
    return bh_entity_is_active(p, e) && e->generation == generation;
}
int __cdecl bh_target_step_read(const BHEntity *e, BHTargetStepState *s)
{
    if (!e || !s)
        return 0;
    memcpy(&s->countdown, (const char *)e + 0xec, 4);
    memcpy(&s->minimum, (const char *)e + 0xfc, 12);
    memcpy(&s->maximum, (const char *)e + 0x108, 12);
    memcpy(&s->emission, (const char *)e + 0x114, 16);
    return 1;
}
int __cdecl bh_target_step_write(BHEntity *e, const BHTargetStepState *s)
{
    if (!e || !s)
        return 0;
    memcpy((char *)e + 0xec, &s->countdown, 4);
    memcpy((char *)e + 0xfc, &s->minimum, 12);
    memcpy((char *)e + 0x108, &s->maximum, 12);
    memcpy((char *)e + 0x114, &s->emission, 16);
    if (e->step == bh_target_step)
    {
        BHTargetStepContext *c = context(e);
        unsigned long i;
        for (i = 0; i < c->physics->body_count; ++i)
            if (c->physics->bodies[i].entity == e)
                c->physics->bodies[i].flags = (unsigned long)s->emission;
    }
    return 1;
}
int __cdecl bh_target_step_bind(BHTargetStepContext *c, const BHTargetStepState *s)
{
    unsigned long i;
    BHDebrisBody *b;
    if (!c || !s || !(b = c->body) || !b->entity || !b->context || !c->physics || !c->explosions || !c->particles ||
        !c->pickups || !c->mode || !bh_math_environment_supported())
        return 0;
    if (b->entity->query != bh_debris_receive || !live(b->context->pool, b->entity, b->generation) ||
        c->physics->pool != b->context->pool || c->particles->pool != b->context->pool ||
        c->explosions->pool != b->context->pool || c->pickups->pool != b->context->pool ||
        c->pickups->outside_actors != b->context->registry || !c->pickups->first_world || !c->pickups->world_count ||
        c->pickups->resource_count < 20 || c->pickups->default_draw != bh_pickup_default_step ||
        c->pickups->special_draw != bh_pickup_special_step)
        return 0;
    for (i = 0; i < c->physics->body_count; ++i)
        if (c->physics->bodies[i].entity == b->entity && c->physics->bodies[i].physics == b->physics)
            break;
    if (i == c->physics->body_count)
        return 0;
    c->physics->bodies[i].flags = (unsigned long)s->emission;
    bh_target_step_write(b->entity, s);
    memcpy(b->entity->payload + 616, &c, 4);
    b->entity->step = bh_target_step;
    c->error = 0;
    return 1;
}
static double norm(const BHVec3 *v)
{
    return ((double)v->x * v->x + (double)v->y * v->y) + (double)v->z * v->z;
}
static void sample(BHVec3 *out, BHEntity *e, const BHTargetStepState *s)
{
    BHPhysicsState p;
    float x, y;
    double t, z;
    t = (double)rand() / 32768;
    bh_round_float(&x, (double)s->maximum.x * (1 - t) + (double)s->minimum.x * t);
    t = (double)rand() / 32768;
    bh_round_float(&y, (double)s->maximum.y * (1 - t) + (double)s->minimum.y * t);
    t = (double)rand() / 32768;
    z = (double)s->maximum.z * (1 - t) + (double)s->minimum.z * t;
    bh_physics_step_read_entity(&p, e);
    bh_round_float(&out->x, (((double)p.pose.m[0][0] * x + (double)p.pose.m[0][2] * z) + (double)p.pose.m[0][1] * y) +
                                e->position.x);
    bh_round_float(&out->y, (((double)p.pose.m[1][1] * y + (double)p.pose.m[1][2] * z) + (double)p.pose.m[1][0] * x) +
                                e->position.y);
    bh_round_float(&out->z, (((double)p.pose.m[2][2] * z + (double)p.pose.m[2][1] * y) + (double)p.pose.m[2][0] * x) +
                                e->position.z);
}
void __cdecl bh_target_step(BHEntity *e)
{
    BHTargetStepContext *c = context(e);
    BHDebrisBody *b = c->body;
    BHEntityPool *pool = b->context->pool;
    BHTargetStepState s;
    BHVec3 velocity, acceleration = {0, -.002f, 0}, point;
    BHEntity *owner;
    unsigned long generation = e->generation, u;
    long event[2];
    int r, i;
    float angle, spin;
    double value, n;
    static const long amount[10] = {100, 50, 25, 2, 50, 1, 1, 1, 1, 1};
    if (*c->mode == 1)
        return;
    if (!bh_math_environment_supported())
    {
        c->error = 1;
        return;
    }
    c->error = 0;
    bh_target_step_read(e, &s);
    if (s.counter > 0)
    {
        u = (unsigned long)s.counter + 1;
        memcpy(&s.counter, &u, 4);
        memcpy((char *)e + 0x120, &s.counter, 4);
    }
    if (s.emission)
    {
        r = rand();
        if (r < s.emission)
        {
            r = rand();
            bh_round_float(&velocity.x, (double)(r - 16384) / 262144);
            r = rand();
            bh_round_float(&velocity.y, (double)(r - 16384) / 262144);
            r = rand();
            bh_round_float(&velocity.z, (double)(r - 16384) / 262144);
            r = rand();
            bh_round_float(&spin, (double)(r - 16384) / 16384);
            r = rand();
            bh_round_float(&angle, (double)r / 512);
            bh_script_particle_create(c->particles, c->smoke_image, 3, &e->position, &velocity, &acceleration, .5f,
                                      angle, 12, spin, .9f, .95f, 2, .3f);
        }
    }
    if (!_isnan(s.countdown) && s.countdown > 0)
    {
        value = (double)s.countdown - 1.0 / 30.0;
        bh_round_float(&s.countdown, value);
        memcpy((char *)e + 0xec, &s.countdown, 4);
        if (_isnan(value) || value <= 0)
        {
            owner = b->owner;
            if (owner)
            {
                if (!live(pool, owner, owner->generation))
                {
                    c->error = 1;
                    return;
                }
                event[0] = 12;
                event[1] = s.token;
                if (owner->query)
                    owner->query(owner, event);
                if (!live(pool, e, generation))
                {
                    c->error = 1;
                    return;
                }
            }
            bh_target_step_read(e, &s);
            if (s.loot < 0)
            {
                for (i = 0; i < 6; ++i)
                {
                    bh_target_step_read(e, &s);
                    sample(&point, e, &s);
                    bh_script_explosion_create(c->explosions, &point, 1, 12, .95f, (unsigned long)b->owner);
                    if (!live(pool, e, generation))
                    {
                        c->error = 1;
                        return;
                    }
                }
            }
            else
            {
                if (s.loot < 10)
                    bh_pickup_spawn(c->pickups, &e->position, s.loot, 0, amount[s.loot], amount[s.loot], 0, 40);
                else if ((s.loot >= 10 && s.loot <= 12) || s.loot == 14)
                    bh_pickup_spawn(c->pickups, &e->position, s.loot - 10, 1, 1, 1, 0, 40);
                bh_script_explosion_create(c->explosions, &e->position, 1.2f, 15, .95f, (unsigned long)b->owner);
                if (!live(pool, e, generation))
                {
                    c->error = 1;
                    return;
                }
            }
            bh_debris_remove(e);
            return;
        }
    }
    if (!_isnan(b->sleep_time) && b->sleep_time > 2)
        return;
    value = (double)b->sleep_time - 1.0 / 30.0;
    bh_round_float(&b->sleep_time, value);
    if (_isnan(value) || value < 0)
        b->sleep_time = 0;
    if (!bh_physics_step(c->physics, e, .03333333507180214f))
    {
        c->error = 1;
        return;
    }
    if (!live(pool, e, generation))
    {
        c->error = 1;
        return;
    }
    n = norm(&b->physics->velocity);
    if (!_isnan(n) && n >= .25)
        return;
    n = norm(&b->physics->angular_velocity);
    if (!_isnan(n) && n >= .01)
        return;
    bh_round_float(&b->sleep_time, (double)b->sleep_time - (-2.0 / 30.0));
}
