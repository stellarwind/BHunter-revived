#include "bh_script_debris.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
typedef char debris_layout
    [sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 && offsetof(BHEntity, origin) == 0x48 ? 1 : -1];
static BHScriptDebrisContext *context(BHEntity *e)
{
    BHScriptDebrisContext *c;
    memcpy(&c, e->payload + 472, 4);
    return c;
}
static void get(const BHEntity *e, unsigned long a, void *p)
{
    memcpy(p, (const unsigned char *)e + a, 4);
}
static void put(BHEntity *e, unsigned long a, const void *p)
{
    memcpy((unsigned char *)e + a, p, 4);
}
int __cdecl bh_script_debris_read(const BHEntity *e, BHScriptDebrisState *s)
{
    if (!e || !s)
        return 0;
    get(e, 0xe8, &s->lifetime);
    get(e, 0x194, &s->angle);
    get(e, 0x198, &s->spin);
    get(e, 0x1a0, &s->width);
    get(e, 0x1a4, &s->height);
    get(e, 0x1a8, &s->frame);
    get(e, 0x1b0, &s->origin_tag);
    return 1;
}
int __cdecl bh_script_debris_write(BHEntity *e, const BHScriptDebrisState *s)
{
    if (!e || !s)
        return 0;
    put(e, 0xe8, &s->lifetime);
    put(e, 0x194, &s->angle);
    put(e, 0x198, &s->spin);
    put(e, 0x1a0, &s->width);
    put(e, 0x1a4, &s->height);
    put(e, 0x1a8, &s->frame);
    put(e, 0x1b0, &s->origin_tag);
    return 1;
}
unsigned long __cdecl bh_script_debris_create(BHScriptDebrisContext *c, long count, const BHVec3 *p, float lifetime,
                                              float speed, float size)
{
    BHEntity *e;
    BHScriptDebrisState s;
    unsigned long created = 0;
    float length;
    double factor;
    int r;
    if (!c || !c->pool || !c->queue || !c->queue->commands || count < 0 || count > 65535 ||
        !bh_math_environment_supported())
    {
        if (c)
            c->error = BH_DEBRIS_ARGUMENT;
        return 0;
    }
    if (!count)
    {
        c->origin_tag = 0;
        c->error = BH_DEBRIS_OK;
        return 0;
    }
    if (!p)
    {
        c->error = BH_DEBRIS_ARGUMENT;
        return 0;
    }
    while (count--)
    {
        e = bh_entity_acquire(c->pool);
        if (!e)
        {
            c->error = BH_DEBRIS_POOL;
            return created;
        }
        e->position = *p;
        s.lifetime = lifetime;
        s.origin_tag = c->origin_tag;
        s.frame = (unsigned long)rand() & 4;
        r = rand();
        e->origin.x = (float)(r - 16384);
        r = rand();
        e->origin.y = (float)(r - 16384);
        r = rand();
        e->origin.z = (float)(r - 16384);
        bh_round_float(&length, sqrt(((double)e->origin.x * e->origin.x + (double)e->origin.y * e->origin.y) +
                                     (double)e->origin.z * e->origin.z));
        r = rand();
        factor = (((.9 - (double)r * (-.000006103515625)) * speed) * (1.0 / 30.0)) / length;
        bh_round_float(&e->origin.x, factor * e->origin.x);
        bh_round_float(&e->origin.y, factor * e->origin.y);
        bh_round_float(&e->origin.z, factor * e->origin.z);
        r = rand();
        bh_round_float(&s.width, ((double)r * size) * (1.0 / 16384.0));
        r = rand();
        bh_round_float(&s.height, ((double)r * size) * (1.0 / 16384.0));
        r = rand();
        bh_round_float(&s.angle, (double)r * .0001953125);
        r = rand();
        bh_round_float(&s.spin, ((double)r - 16384.0) * (1.0 / 32768.0));
        bh_script_debris_write(e, &s);
        memcpy(e->payload + 472, &c, 4);
        e->step = bh_script_debris_tick;
        e->refresh = bh_script_debris_refresh;
        ++created;
    }
    c->origin_tag = 0;
    c->error = BH_DEBRIS_OK;
    return created;
}
void __cdecl bh_script_debris_tick(BHEntity *e)
{
    BHScriptDebrisContext *c = context(e);
    BHScriptDebrisState s;
    BHVec3 old = e->origin;
    double life, y, factor;
    float ry;
    if (!bh_math_environment_supported())
    {
        c->error = BH_DEBRIS_ARGUMENT;
        return;
    }
    bh_script_debris_read(e, &s);
    life = (double)s.lifetime - (1.0 / 30.0);
    bh_round_float(&s.lifetime, life);
    put(e, 0xe8, &s.lifetime);
    if (_isnan(life) || life < 0)
    {
        bh_entity_release(c->pool, e);
        return;
    }
    y = (double)old.y - (-.010911111111111113);
    bh_round_float(&ry, y);
    factor = 1.0 - sqrt(((double)old.x * old.x + y * ry) + (double)old.z * old.z) * .0025;
    bh_round_float(&e->position.x, (double)old.x + e->position.x);
    bh_round_float(&e->position.y, (double)old.y + e->position.y);
    bh_round_float(&e->position.z, (double)old.z + e->position.z);
    e->origin.y = ry;
    if (_isnan(factor) || factor < .5)
        factor = .5;
    bh_round_float(&e->origin.x, factor * old.x);
    bh_round_float(&e->origin.y, factor * y);
    bh_round_float(&e->origin.z, factor * old.z);
    bh_round_float(&s.angle, (double)s.spin + s.angle);
    put(e, 0x194, &s.angle);
}
void __cdecl bh_script_debris_refresh(BHEntity *e, void *packet)
{
    BHScriptDebrisContext *c = context(e);
    BHScriptDebrisState s;
    int r;
    (void)packet;
    bh_script_debris_read(e, &s);
    r = bh_draw_billboard(c->queue, 0, c->image, &e->position, s.width, s.height, s.angle, s.frame, 1);
    if (r < 0)
        c->error = BH_DEBRIS_QUEUE;
}
