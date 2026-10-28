#include "bh_script_particles.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef char particle_layout[sizeof(BHScriptParticleState) == 84 && offsetof(BHScriptParticleState, velocity) == 60 &&
                                     offsetof(BHEntity, payload) == 0xe4
                                 ? 1
                                 : -1];
static BHScriptParticleState *state(BHEntity *e)
{
    return (BHScriptParticleState *)(e->payload + 4);
}
static BHScriptParticleContext *context(BHEntity *e)
{
    BHScriptParticleContext *c;
    memcpy(&c, e->payload + 440, 4);
    return c;
}
static int live(BHEntityPool *pool, BHEntity *e)
{
    return bh_entity_is_active(pool, e);
}
int __cdecl bh_script_particles_init(BHScriptParticleContext *c, BHEntityPool *p, BHDrawQueue *q)
{
    if (!c || !p || !q || !q->commands || !bh_entities_valid(p))
        return 0;
    c->pool = p;
    c->queue = q;
    c->error = BH_PARTICLE_OK;
    return 1;
}
BHScriptParticleState *__cdecl bh_script_particle_state(BHScriptParticleContext *c, BHEntity *e)
{
    if (!c || !e || !live(c->pool, e) || e->step != bh_script_particle_tick ||
        e->refresh != bh_script_particle_refresh || context(e) != c)
        return NULL;
    return state(e);
}
BHEntity *__cdecl bh_script_particle_create(BHScriptParticleContext *c, unsigned long resource, unsigned long bucket,
                                            const BHVec3 *p, const BHVec3 *v, const BHVec3 *a, float size, float angle,
                                            float size_rate, float angular_rate, float size_decay, float angular_decay,
                                            float lifetime, float alpha)
{
    BHEntity *e;
    BHScriptParticleState *s;
    BHVec3 zero = {0, 0, 0};
    if (!c || !c->pool || !c->queue || !p || !bh_math_environment_supported())
    {
        if (c)
            c->error = BH_PARTICLE_ARGUMENT;
        return NULL;
    }
    e = bh_entity_acquire(c->pool);
    if (!e)
    {
        c->error = BH_PARTICLE_POOL;
        return NULL;
    }
    s = state(e);
    e->position = *p;
    s->lifetime = lifetime;
    s->velocity = v ? *v : zero;
    s->acceleration = a ? *a : zero;
    s->size = size;
    s->angle = angle;
    bh_round_float(&s->size_step, (double)size_rate * (1.0 / 30.0));
    bh_round_float(&s->angular_step, (double)angular_rate * (1.0 / 30.0));
    s->size_decay = size_decay;
    s->angular_decay = angular_decay;
    s->alpha = alpha;
    bh_round_float(&s->alpha_step, ((double)alpha * (-1.0 / 30.0)) / lifetime);
    s->frame = (unsigned long)rand() & 7;
    s->resource = resource;
    s->bucket = bucket;
    memcpy(e->payload + 440, &c, 4);
    e->step = bh_script_particle_tick;
    e->refresh = bh_script_particle_refresh;
    c->error = BH_PARTICLE_OK;
    return e;
}
void __cdecl bh_script_particle_tick(BHEntity *e)
{
    BHScriptParticleContext *c = context(e);
    BHScriptParticleState *s = state(e);
    double next, x;
    if (!bh_math_environment_supported())
    {
        c->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    next = (double)s->lifetime - (1.0 / 30.0);
    bh_round_float(&s->lifetime, next);
    if (!(next > 0))
    {
        bh_entity_release(c->pool, e);
        return;
    }
    /* X's velocity sum survives its binary32 store on the original x87 stack.
    Y/Z position additions reload their rounded stored velocity instead. */
    x = (double)s->acceleration.x + s->velocity.x;
    bh_round_float(&s->velocity.x, x);
    bh_round_float(&s->velocity.y, (double)s->acceleration.y + s->velocity.y);
    bh_round_float(&s->velocity.z, (double)s->acceleration.z + s->velocity.z);
    bh_round_float(&e->position.x, (double)e->position.x + x);
    bh_round_float(&e->position.y, (double)s->velocity.y + e->position.y);
    bh_round_float(&e->position.z, (double)s->velocity.z + e->position.z);
    bh_round_float(&s->angle, (double)s->angular_step + s->angle);
    bh_round_float(&s->size, (double)s->size_step + s->size);
    bh_round_float(&s->angular_step, (double)s->angular_decay * s->angular_step);
    bh_round_float(&s->size_step, (double)s->size_decay * s->size_step);
    bh_round_float(&s->alpha, (double)s->alpha_step + s->alpha);
}
void __cdecl bh_script_particle_refresh(BHEntity *e, void *packet)
{
    BHScriptParticleContext *c = context(e);
    BHScriptParticleState *s = state(e);
    int r;
    (void)packet;
    r = bh_draw_billboard(c->queue, s->bucket, s->resource, &e->position, s->size, s->size, s->angle, s->frame,
                          s->alpha);
    if (r < 0)
        c->error = BH_PARTICLE_QUEUE;
}
static void get(BHEntity *e, unsigned long o, void *p, unsigned long n)
{
    memcpy(p, (unsigned char *)e + o, n);
}
static void put(BHEntity *e, unsigned long o, const void *p, unsigned long n)
{
    memcpy((unsigned char *)e + o, p, n);
}
static BHScriptEmitterContext *emitter_context(BHEntity *e)
{
    BHScriptEmitterContext *c;
    memcpy(&c, e->payload + 448, 4);
    return c;
}
static const BHScriptTimerActor *actor_record(BHScriptEmitterContext *c, BHEntity *e)
{
    unsigned long i;
    for (i = 0; i < c->lifetime->timers.actor_count; ++i)
        if (c->lifetime->timers.actors[i].entity == e)
            return c->lifetime->timers.actors + i;
    return NULL;
}
int __cdecl bh_script_emitter_read(BHEntity *e, BHScriptEmitterState *s)
{
    if (!e || !s)
        return 0;
    get(e, 0xf0, &s->actor, 4);
    get(e, 0xf4, &s->script_loaded, 4);
    get(e, 0x128, &s->countdown, 4);
    get(e, 0x12c, &s->spread, 4);
    get(e, 0x130, &s->velocity, 12);
    get(e, 0x13c, &s->acceleration, 12);
    get(e, 0x10c, &s->size, 4);
    return 1;
}
int __cdecl bh_script_emitter_bind(BHScriptEmitterContext *c, BHEntity *e, const BHScriptEmitterState *s, int steam)
{
    BHScriptLifetimeState base;
    unsigned char retained[12];
    if (!c || !c->lifetime || !c->particles || !c->particles->pool || !c->lifetime->timers.pool ||
        c->particles->pool != c->lifetime->timers.pool || !e || !s || !actor_record(c, s->actor))
        return 0;
    get(e, 0xfc, retained, 4);
    get(e, 0x104, retained + 4, 4);
    get(e, 0x180, retained + 8, 4);
    memset(&base, 0, sizeof(base));
    base.actor = s->actor;
    base.script_loaded = s->script_loaded;
    base.countdown = s->countdown;
    base.delta = s->velocity;
    if (!bh_script_lifetime_bind(c->lifetime, e, &base, bh_script_sensor_query))
        return 0;
    put(e, 0xfc, retained, 4);
    put(e, 0x104, retained + 4, 4);
    put(e, 0x180, retained + 8, 4);
    put(e, 0x12c, &s->spread, 4);
    put(e, 0x13c, &s->acceleration, 12);
    put(e, 0x10c, &s->size, 4);
    memcpy(e->payload + 448, &c, 4);
    e->step = steam ? bh_script_steam_tick : bh_script_fountain_tick;
    e->refresh = steam ? bh_script_steam_refresh : NULL;
    c->error = BH_PARTICLE_OK;
    return 1;
}
static int active(BHScriptEmitterContext *c, BHEntity *e)
{
    const BHScriptTimerActor *a = actor_record(c, e);
    if (!a)
    {
        c->error = BH_PARTICLE_ACTOR;
        return 0;
    }
    if (a->owner)
        a = a->owner;
    return a->active != 0;
}
static int countdown(BHEntity *e, float value)
{
    double n;
    if (!(value > 0))
        return 0;
    n = (double)value - (1.0 / 30.0);
    bh_round_float(&value, n);
    put(e, 0x128, &value, 4);
    return !(n > 0);
}
void __cdecl bh_script_fountain_tick(BHEntity *e)
{
    BHScriptEmitterContext *c = emitter_context(e);
    BHScriptEmitterState s;
    int expires, r;
    float spin, rate, angle;
    bh_script_emitter_read(e, &s);
    if (!active(c, s.actor))
        return;
    if (!bh_math_environment_supported())
    {
        c->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    expires = countdown(e, s.countdown);
    if (expires || rand() < 11000)
    {
        r = rand();
        bh_round_float(&spin, (double)(r - 16384) * (1.0 / 131072.0));
        r = rand();
        bh_round_float(&rate, 10.0 - (double)r * (-1.0 / 2048.0));
        r = rand();
        bh_round_float(&angle, (double)r * (1.0 / 4096.0));
        bh_script_particle_create(c->particles, c->fountain_texture, 3, &s.actor->position, &s.velocity,
                                  &s.acceleration, .1f, angle, rate, spin, .98f, .95f, 1, 1);
    }
    if (expires)
        bh_entity_release(c->lifetime->timers.pool, e);
}
void __cdecl bh_script_steam_tick(BHEntity *e)
{
    BHScriptEmitterContext *c = emitter_context(e);
    BHScriptEmitterState s;
    BHVec3 v;
    int expires, r;
    float spin, angle;
    bh_script_emitter_read(e, &s);
    if (!active(c, s.actor))
        return;
    if (!bh_math_environment_supported())
    {
        c->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    expires = countdown(e, s.countdown);
    if (expires || rand() < 10000)
    {
        v = s.velocity;
        r = rand();
        bh_round_float(&v.x, (double)(r - 16384) * s.spread + v.x);
        r = rand();
        bh_round_float(&v.y, (double)(r - 16384) * s.spread + v.y);
        r = rand();
        bh_round_float(&v.z, (double)(r - 16384) * s.spread + v.z);
        r = rand();
        bh_round_float(&spin, (double)(r - 16384) * (1.0 / 131072.0));
        r = rand();
        bh_round_float(&angle, (double)r * (1.0 / 4096.0));
        bh_script_particle_create(c->particles, c->steam_texture, 3, &s.actor->position, &v, &s.acceleration, .2f,
                                  angle, 4, spin, 1, .95f, 3, 1);
    }
    if (expires)
        bh_entity_release(c->lifetime->timers.pool, e);
}
void __cdecl bh_script_steam_refresh(BHEntity *e, void *packet)
{
    BHScriptEmitterContext *c = emitter_context(e);
    BHScriptEmitterState s;
    BHVec3 p, up = {0, 0, 1}, normal = {0, -1, 0};
    unsigned long frame;
    float size, angle;
    int r;
    (void)packet;
    bh_script_emitter_read(e, &s);
    if (!active(c, s.actor))
        return;
    if (!bh_math_environment_supported())
    {
        c->error = BH_PARTICLE_NUMERICAL;
        return;
    }
    bh_round_float(&size, (double)s.size);
    frame = (unsigned long)rand() & 7;
    r = rand();
    bh_round_float(&angle, (double)r * (1.0 / 4096.0));
    p = s.actor->position;
    bh_round_float(&p.y, (double)p.y - 1.6);
    if (bh_draw_oriented(c->particles->queue, 3, c->steam_texture, &p, &up, &normal, size, size, angle, frame, 1) < 0)
        c->error = BH_PARTICLE_QUEUE;
}
