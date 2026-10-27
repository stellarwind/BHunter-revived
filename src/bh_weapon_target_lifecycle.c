#include "bh_weapon_target_lifecycle.h"
#include "bh_round.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <stddef.h>
static float stored(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static int fail(BHWeaponTargetContext *c, BHWeaponTargetError e)
{
    if (c)
        c->error = e;
    return 0;
}
static int finite_vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
BHVec3 *__cdecl bh_weapon_target_player_aim(BHEntity *p)
{
    return p ? (BHVec3 *)((unsigned char *)p + 0x280) : NULL;
}
static void clear(BHWeaponCommonState *s)
{
    s->word_e8 = s->word_f4 = s->word_f0 = 0;
    s->hud.target = NULL;
    s->hud.charge = 0;
}
static void stop(BHWeaponTargetContext *c, unsigned long h)
{
    if (h != BH_SOUND_INVALID)
    {
        ++c->stops;
        if (bh_sound_stop(c->sound, h) < 0)
            c->error = BH_WEAPON_TARGET_SOUND;
    }
}
int __cdecl bh_weapon_target_ready(BHWeaponTargetContext *c, BHEntity *w, const BHEntity *p)
{
    BHWeaponCommonState *s = bh_weapon_common_state(w);
    int value;
    float pitch;
    unsigned long h;
    if (!c || !c->sound || !s || !p)
        return fail(c, BH_WEAPON_TARGET_ARGUMENT);
    c->error = BH_WEAPON_TARGET_OK;
    clear(s);
    s->word_100 = -1;
    value = rand();
    ++c->random_calls;
    pitch = stored(.9 - (double)value * (-.000006103515625));
    ++c->sound_requests;
    h = bh_sound_request(c->sound, c->ready_sound, .8f, pitch, &p->position, &p->origin);
    if (h == BH_SOUND_INVALID)
        c->error = BH_WEAPON_TARGET_SOUND;
    return c->error == BH_WEAPON_TARGET_OK;
}
int __cdecl bh_weapon_target_cancel(BHWeaponTargetContext *c, BHEntity *w)
{
    BHWeaponCommonState *s = bh_weapon_common_state(w);
    if (!c || !c->sound || !s)
        return fail(c, BH_WEAPON_TARGET_ARGUMENT);
    c->error = BH_WEAPON_TARGET_OK;
    clear(s);
    stop(c, (unsigned long)s->word_100);
    s->word_100 = -1;
    return c->error == BH_WEAPON_TARGET_OK;
}
static int query(BHWeaponTargetContext *c, BHEntity *e, BHEntityPositionPacket *p)
{
    if (!e || !e->query)
        return fail(c, BH_WEAPON_TARGET_CALLBACK);
    ++c->queries;
    e->query(e, p);
    return finite_vector(&p->position) ? 1 : fail(c, BH_WEAPON_TARGET_NUMERICAL);
}
int __cdecl bh_weapon_target_update(BHWeaponTargetContext *c, BHEntity *w, const BHEntity *p)
{
    BHWeaponCommonState *s = bh_weapon_common_state(w);
    const BHVec3 *aim;
    BHEntity *e, *next;
    BHVec3 d;
    union {
        BHEntityPositionPacket packet;
        unsigned long words[10];
    } packet;
    BHWorldSegmentHit hit;
    BHWorldHit self;
    unsigned long visited = 0, h, charge;
    float best = 150, dot, angle, value;
    double x, y, z, projection, length;
    int result;
    if (!c || !c->sound || !s || !p)
        return fail(c, BH_WEAPON_TARGET_ARGUMENT);
    c->error = BH_WEAPON_TARGET_OK;
    memcpy(&value, &s->word_f0, 4);
    if (value > 0)
    {
        value = stored((double)value - 1.0 / 30.0);
        memcpy(&s->word_f0, &value, 4);
    }
    memcpy(&value, &s->word_f4, 4);
    if (value > 0)
    {
        value = stored((double)value - 1.0 / 30.0);
        memcpy(&s->word_f4, &value, 4);
    }
    packet.packet.event = 3;
    if (!s->ammo)
    {
        h = (unsigned long)s->word_100;
        s->hud.target = NULL;
        stop(c, h);
        s->word_100 = -1;
        return c->error == BH_WEAPON_TARGET_OK;
    }
    if (!s->hud.target)
    {
        e = c->actors_owner ? *c->actors_owner : c->actors;
        while (e)
        {
            if (++visited > c->max_actors)
                return fail(c, BH_WEAPON_TARGET_LIMIT);
            if (!query(c, e, &packet.packet))
                return 0;
            aim = bh_weapon_target_player_aim((BHEntity *)p);
            if (!finite_vector(&p->position) || !finite_vector(aim))
                return fail(c, BH_WEAPON_TARGET_NUMERICAL);
            d.x = stored((double)packet.packet.position.x - p->position.x);
            d.y = stored((double)packet.packet.position.y - p->position.y);
            d.z = stored((double)packet.packet.position.z - p->position.z);
            projection = ((double)aim->y * d.y + (double)aim->x * d.x) + (double)aim->z * d.z;
            dot = stored(projection);
            if (projection >= 0 && dot < best)
            {
                length = sqrt(((double)d.x * d.x + (double)d.y * d.y) + (double)d.z * d.z);
                if (!(1.0 - (double)dot / length >= .02))
                {
                    if (!c->world)
                        return fail(c, BH_WEAPON_TARGET_WORLD);
                    self.kind = BH_WORLD_HIT_ACTOR;
                    self.object.actor = (BHEntity *)p;
                    memset(&hit, 0, sizeof(hit));
                    ++c->traces;
                    result = bh_world_segment_run(c->world, &self, (BHWorldGeometry **)&s->word_104, &p->position,
                                                  &packet.packet.position, -1, &hit);
                    if (result < 0)
                        c->error = BH_WEAPON_TARGET_WORLD;
                    if (!result)
                    {
                        s->hud.target = e;
                        best = dot;
                    }
                }
            }
            memcpy(&next, e->payload + 12, 4);
            e = next;
        }
        s->hud.charge = 0;
        return c->error == BH_WEAPON_TARGET_OK;
    }
    charge = (unsigned long)s->hud.charge + 1UL;
    s->hud.charge = (long)charge;
    if ((long)charge > 16 && (long)charge < 48)
    {
        if (charge & 2)
        {
            ++c->sound_requests;
            if (bh_sound_request(c->sound, c->beep_sound, .5f, 1, NULL, NULL) == BH_SOUND_INVALID)
                c->error = BH_WEAPON_TARGET_SOUND;
        }
    }
    else if ((long)charge == 48)
    {
        ++c->sound_requests;
        h = bh_sound_request(c->sound, c->lock_sound, .5f, 1.7f, NULL, NULL);
        s->word_100 = (long)h;
        if (h == BH_SOUND_INVALID)
            c->error = BH_WEAPON_TARGET_SOUND;
    }
    if (!query(c, s->hud.target, &packet.packet))
        return 0;
    aim = bh_weapon_target_player_aim((BHEntity *)p);
    if (!finite_vector(&p->position) || !finite_vector(aim))
        return fail(c, BH_WEAPON_TARGET_NUMERICAL);
    x = (double)packet.packet.position.x - p->position.x;
    y = (double)packet.packet.position.y - p->position.y;
    z = (double)packet.packet.position.z - p->position.z;
    d.x = stored(x);
    d.y = stored(y);
    length = sqrt((x * d.x + y * d.y) + z * z);
    projection = ((double)aim->z * z + (double)aim->y * d.y) + (double)aim->x * d.x;
    angle = stored(1.0 - projection / length);
    if (projection >= 0 && !(projection > 150.0) && !((double)angle > .075) &&
        (s->hud.charge >= 48 || !((double)angle > .02)))
        return c->error == BH_WEAPON_TARGET_OK;
    h = (unsigned long)s->word_100;
    s->hud.target = NULL;
    stop(c, h);
    s->word_100 = -1;
    return c->error == BH_WEAPON_TARGET_OK;
}
