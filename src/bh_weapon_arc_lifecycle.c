#include "bh_weapon_arc_lifecycle.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef union ArcPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} ArcPacket;
static void begin(BHWeaponArcLifecycleContext *c)
{
    c->visited = c->queries = c->world_queries = c->requests = c->updates = c->stops = c->random_calls = 0;
    c->error = BH_WEAPON_ARC_LIFECYCLE_OK;
}
static int error(BHWeaponArcLifecycleContext *c, BHWeaponArcLifecycleError e)
{
    if (c && c->error == BH_WEAPON_ARC_LIFECYCLE_OK)
        c->error = e;
    return 0;
}
static int valid(BHWeaponArcLifecycleContext *c, BHEntity *weapon)
{
    if (!c)
        return 0;
    begin(c);
    return c->sound && weapon ? 1 : error(c, BH_WEAPON_ARC_LIFECYCLE_ARGUMENT);
}
static int finite_vector(const BHVec3 *p)
{
    return _finite(p->x) && _finite(p->y) && _finite(p->z);
}
static const BHVec3 *aim(const BHEntity *p)
{
    return (const BHVec3 *)((const unsigned char *)p + 0x280);
}
static unsigned long request(BHWeaponArcLifecycleContext *c, unsigned long id, const BHEntity *p, float level,
                             float pitch)
{
    unsigned long handle;
    ++c->requests;
    handle = bh_sound_request(c->sound, id, level, pitch, &p->position, &p->origin);
    if (handle == BH_SOUND_INVALID)
        error(c, BH_WEAPON_ARC_LIFECYCLE_SOUND);
    return handle;
}
static void stop(BHWeaponArcLifecycleContext *c, unsigned long handle)
{
    ++c->stops;
    if (bh_sound_stop(c->sound, handle) < 0)
        error(c, BH_WEAPON_ARC_LIFECYCLE_SOUND);
}
static int query(BHWeaponArcLifecycleContext *c, BHEntity *actor, ArcPacket *packet)
{
    if (!actor || !actor->query)
        return error(c, BH_WEAPON_ARC_LIFECYCLE_CALLBACK);
    ++c->queries;
    actor->query(actor, packet);
    return finite_vector(&packet->position.position) ? 1 : error(c, BH_WEAPON_ARC_LIFECYCLE_NUMERICAL);
}
int __cdecl bh_weapon_arc_activate(BHWeaponArcLifecycleContext *c, BHEntity *weapon, const BHEntity *player)
{
    BHWeaponCommonState *s;
    float pitch;
    int value;
    if (!valid(c, weapon) || !player)
        return error(c, BH_WEAPON_ARC_LIFECYCLE_ARGUMENT);
    s = bh_weapon_common_state(weapon);
    s->word_e8 = 0;
    s->hud.target = NULL;
    s->hud.charge = 0;
    s->word_100 = -1;
    value = rand();
    c->random_calls = 1;
    bh_round_float(&pitch, .9 - (double)value * (-.000006103515625));
    if (request(c, c->activate_sound, player, .8f, pitch) == BH_SOUND_INVALID && c->sound->error == BH_SOUND_PRIORITY)
        c->error = BH_WEAPON_ARC_LIFECYCLE_OK;
    s->word_108 = 0;
    return c->error == BH_WEAPON_ARC_LIFECYCLE_OK;
}
int __cdecl bh_weapon_arc_deactivate(BHWeaponArcLifecycleContext *c, BHEntity *weapon)
{
    BHWeaponCommonState *s;
    unsigned long handle;
    if (!valid(c, weapon))
        return 0;
    s = bh_weapon_common_state(weapon);
    handle = (unsigned long)s->word_100;
    s->word_e8 = 0;
    s->hud.target = NULL;
    s->hud.charge = 0;
    if (handle != BH_SOUND_INVALID)
        stop(c, handle);
    s->word_108 = 0;
    s->word_100 = -1;
    return c->error == BH_WEAPON_ARC_LIFECYCLE_OK;
}
int __cdecl bh_weapon_arc_track(BHWeaponArcLifecycleContext *c, BHEntity *weapon, const BHEntity *player)
{
    BHWeaponCommonState *s;
    ArcPacket packet;
    BHEntity *actor, *successor;
    BHWorldHit self;
    BHWorldSegmentHit hit;
    BHVec3 d;
    double x, y, z, dot, length, cone;
    float nearest, projection, stored_cone;
    unsigned long handle;
    int result;
    const BHVec3 *direction;
    if (!valid(c, weapon) || !player)
        return error(c, BH_WEAPON_ARC_LIFECYCLE_ARGUMENT);
    s = bh_weapon_common_state(weapon);
    packet.position.event = 3;
    nearest = 150;
    if (!s->word_108)
    {
        s->hud.target = NULL;
        handle = (unsigned long)s->word_100;
        if (handle != BH_SOUND_INVALID)
            stop(c, handle);
        s->word_100 = -1;
        if (s->hud.charge > 32)
            request(c, c->release_sound, player, 1, 1);
        s->hud.charge = 0;
        return c->error == BH_WEAPON_ARC_LIFECYCLE_OK;
    }
    s->word_108 = 0;
    if (s->hud.target)
    {
        s->hud.charge = (long)((unsigned long)s->hud.charge + 1UL);
        if (s->hud.charge == 16)
            s->word_100 = (long)request(c, c->charge16_sound, player, 1, 1);
        else if (s->hud.charge == 51)
            s->word_100 = (long)request(c, c->charge51_sound, player, 1, 1);
        else if ((unsigned long)s->word_100 != BH_SOUND_INVALID)
        {
            ++c->updates;
            if (bh_sound_update(c->sound, (unsigned long)s->word_100, 1, 1, &player->position, &player->origin) < 0)
                error(c, BH_WEAPON_ARC_LIFECYCLE_SOUND);
        }
        if (!query(c, s->hud.target, &packet))
            return 0;
        if (!finite_vector(&player->position) || !finite_vector(aim(player)))
            return error(c, BH_WEAPON_ARC_LIFECYCLE_NUMERICAL);
        x = (double)packet.position.position.x - player->position.x;
        y = (double)packet.position.position.y - player->position.y;
        z = (double)packet.position.position.z - player->position.z;
        bh_round_float(&d.x, x);
        bh_round_float(&d.y, y);
        length = sqrt((x * d.x + y * d.y) + z * z);
        direction = aim(player);
        dot = ((z * direction->z + (double)direction->y * d.y) + (double)direction->x * d.x);
        cone = 1.0 - dot / length;
        bh_round_float(&stored_cone, cone);
        if (!(dot < 0) && !(dot > 150.0) && !(stored_cone > .1))
            return c->error == BH_WEAPON_ARC_LIFECYCLE_OK;
        s->hud.target = NULL;
        handle = (unsigned long)s->word_100;
        if (handle != BH_SOUND_INVALID)
            stop(c, handle);
        s->word_100 = -1;
        if (s->hud.charge > 32)
            request(c, c->release_sound, player, 1, 1);
        return c->error == BH_WEAPON_ARC_LIFECYCLE_OK;
    }
    actor = c->head_owner ? *c->head_owner : c->head;
    while (actor)
    {
        if (c->visited >= c->limit)
        {
            error(c, BH_WEAPON_ARC_LIFECYCLE_LIMIT);
            break;
        }
        ++c->visited;
        if (!query(c, actor, &packet))
            break;
        if (!finite_vector(&player->position) || !finite_vector(aim(player)))
        {
            error(c, BH_WEAPON_ARC_LIFECYCLE_NUMERICAL);
            break;
        }
        bh_round_float(&d.x, (double)packet.position.position.x - player->position.x);
        bh_round_float(&d.y, (double)packet.position.position.y - player->position.y);
        bh_round_float(&d.z, (double)packet.position.position.z - player->position.z);
        direction = aim(player);
        dot = ((double)direction->x * d.x + (double)direction->z * d.z) + (double)direction->y * d.y;
        bh_round_float(&projection, dot);
        if (!(dot < 0) && projection < nearest)
        {
            length = sqrt(((double)d.x * d.x + (double)d.y * d.y) + (double)d.z * d.z);
            if (!(1.0 - (double)projection / length >= .1))
            {
                if (!c->world)
                {
                    error(c, BH_WEAPON_ARC_LIFECYCLE_WORLD);
                    break;
                }
                self.kind = BH_WORLD_HIT_ACTOR;
                self.object.actor = (BHEntity *)player;
                ++c->world_queries;
                result = bh_world_segment_run(c->world, &self, (BHWorldGeometry **)&s->word_104, &player->position,
                                              &packet.position.position, -1, &hit);
                if (result < 0)
                    error(c, BH_WEAPON_ARC_LIFECYCLE_WORLD);
                if (!result)
                {
                    s->hud.target = actor;
                    nearest = projection;
                }
            }
        }
        memcpy(&successor, actor->payload + 12, 4);
        actor = successor;
    }
    s->hud.charge = 0;
    return c->error == BH_WEAPON_ARC_LIFECYCLE_OK;
}
