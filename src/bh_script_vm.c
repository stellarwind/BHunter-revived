#include "bh_script_vm.h"
#include "bh_round.h"
#include "bh_fp.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
static float rf(double v)
{
    float f;
    bh_round_float(&f, v);
    return f;
}
static unsigned long word(BHEntity *e, unsigned long o)
{
    unsigned long v;
    memcpy(&v, (char *)e + o, 4);
    return v;
}
static void setword(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static void real(BHEntity *e, unsigned long o, float f)
{
    memcpy((char *)e + o, &f, 4);
}
static BHScriptVMContext *context(BHEntity *e)
{
    BHScriptVMContext *c;
    memcpy(&c, e->payload + 576, 4);
    return c;
}
static BHEntityPool *pool(BHScriptVMContext *c)
{
    return c->lifetime->timers.pool;
}
static BHScriptVMActor *actor(BHScriptVMContext *c, BHEntity *e)
{
    unsigned long i;
    for (i = 0; i < c->actor_count; ++i)
        if (c->actors[i].state && c->actors[i].state->entity == e)
            return c->actors + i;
    return 0;
}
static BHEntity *owner(BHScriptVMActor *a)
{
    return a->state->owner ? a->state->owner->entity : a->state->entity;
}
static int live(BHEntityPool *p, BHEntity *e)
{
    BHEntity *at;
    unsigned long i = 0;
    for (at = p->active; at && i++ < p->capacity; at = at->next)
        if (at == e)
            return 1;
    return 0;
}
int __cdecl bh_script_vm_read(BHEntity *e, BHScriptVMState *s)
{
    if (!e || !s)
        return 0;
    s->next = (BHEntity *)word(e, 0xec);
    s->loaded = (long)word(e, 0xf4);
    s->opcode = (long)word(e, 0xf8);
    memcpy(s->args, (char *)e + 0xfc, 20);
    memcpy(s->selectors, (char *)e + 0x110, 20);
    s->variable_sum = (long)word(e, 0x124);
    return 1;
}
int __cdecl bh_script_vm_bind(BHScriptVMContext *c, BHEntity *e, const BHScriptVMState *s)
{
    if (!c || !c->lifetime || !c->lifetime->timers.pool || !s || !e || !live(pool(c), e))
        return 0;
    memcpy(e->payload + 576, &c, 4);
    setword(e, 0xec, (unsigned long)s->next);
    setword(e, 0xf4, (unsigned long)s->loaded);
    setword(e, 0xf8, (unsigned long)s->opcode);
    memcpy((char *)e + 0xfc, s->args, 20);
    memcpy((char *)e + 0x110, s->selectors, 20);
    setword(e, 0x124, (unsigned long)s->variable_sum);
    return bh_entity_set_query(pool(c), e, bh_script_vm_query);
}
BHEntity *__cdecl bh_script_vm_create(BHScriptVMContext *c, BHEntity **out, long opcode, const long args[5],
                                      const long selectors[5])
{
    BHScriptVMState s;
    BHEntity *e;
    unsigned long sum = 5, i;
    if (!c || !c->lifetime || !c->lifetime->timers.pool || !args || !selectors)
        return 0;
    e = bh_entity_acquire(pool(c));
    if (!e)
        return 0;
    s.next = 0;
    s.loaded = c->script_loading;
    s.opcode = opcode;
    for (i = 0; i < 5; ++i)
    {
        s.args[i] = args[i];
        s.selectors[i] = selectors[i];
        sum += (unsigned long)selectors[i];
    }
    memcpy(&s.variable_sum, &sum, 4);
    e->position = pool(c)->origin;
    if (!bh_script_vm_bind(c, e, &s))
    {
        bh_entity_release(pool(c), e);
        return 0;
    }
    if (out)
        *out = e;
    return e;
}
static void point(BHEntity *e, BHVec3 *minimum, BHVec3 *maximum)
{
    memcpy(minimum, (char *)e + 0x1b0, 12);
    memcpy(maximum, (char *)e + 0x1bc, 12);
}
static void random_point(BHEntity *e, BHVec3 *p, int inner)
{
    BHVec3 lo, hi;
    unsigned long i;
    double t;
    point(e, &lo, &hi);
    for (i = 0; i < 3; ++i)
    {
        t = inner ? .25 - (double)rand() * (-1.0 / 65536.0) : (double)rand() * (1.0 / 32768.0);
        ((float *)p)[i] = rf(((float *)&lo)[i] * t + ((float *)&hi)[i] * (1 - t));
    }
}
static int delta(BHScriptVMActor *a, const long *args, double scale, BHVec3 *v)
{
    double x = args[0], y = args[1], z = args[2];
    switch (a->orientation)
    {
    case 0:
        break;
    case 1:
        x = args[2];
        z = -(double)args[0];
        break;
    case 2:
        x = -(double)args[0];
        z = -(double)args[2];
        break;
    case 3:
        x = -(double)args[2];
        z = args[0];
        break;
    default:
        return 0;
    }
    v->x = rf(x * scale);
    v->y = rf(y * scale);
    v->z = rf(z * scale);
    return 1;
}
static void portal_extent(BHPortal *p, const BHVec3 *point, float first, float second)
{
    float u, v, w, hu, hv;
    switch (p->direction)
    {
    case 0:
    case 4:
        u = point->y;
        v = point->z;
        w = point->x;
        hu = second;
        hv = first;
        break;
    case 1:
    case 5:
        u = point->x;
        v = point->z;
        w = point->y;
        hu = first;
        hv = second;
        break;
    case 2:
    case 6:
        u = point->x;
        v = point->y;
        w = point->z;
        hu = first;
        hv = second;
        break;
    default:
        return;
    }
    if (p->overlap[0] <= u && u <= p->overlap[1] && p->overlap[2] <= v && v <= p->overlap[3] &&
        (double)p->plane - .5 <= w && w <= (double)p->plane - (-.5))
    {
        p->scratch[0] = rf((double)u - hu);
        p->scratch[1] = rf((double)u + hu);
        p->scratch[2] = rf((double)v - hv);
        p->scratch[3] = rf((double)v + hv);
    }
}
static void radar_query(BHEntity *e, void *packet)
{
    BHScriptVMContext *c;
    memcpy(&c, e->payload + 580, 4);
    c->radar->head = c->launch->actors->response->vehicles->head;
    c->radar->count = c->launch->actors->response->vehicles->count;
    bh_radar_entity_query(e, packet);
    c->launch->actors->response->vehicles->head = c->radar->head;
    c->launch->actors->response->vehicles->count = c->radar->count;
}
static int radar(BHScriptVMContext *c, BHEntity *a, BHEntity *watched, const BHVec3 *p, long remove)
{
    BHEntity *e;
    BHActorVehicleLifetimeContext *v;
    if (!c->radar || !c->launch || !c->launch->actors)
    {
        c->error = 1;
        return 0;
    }
    e = bh_entity_acquire(pool(c));
    if (!e)
        return 0;
    e->position = *p;
    e->flags |= 0x100;
    setword(e, 0x108, 0);
    setword(e, 0xfc, 6);
    setword(e, 0x11c, 0);
    v = c->launch->actors->response->vehicles;
    c->radar->head = v->head;
    c->radar->count = v->count;
    if (!bh_radar_query_bind(c->radar, e, watched, remove))
    {
        bh_entity_release(pool(c), e);
        c->error = 2;
        return 0;
    }
    v->head = c->radar->head;
    v->count = c->radar->count;
    memcpy(e->payload + 580, &c, 4);
    bh_entity_set_query(pool(c), e, radar_query);
    return 1;
}
static int execute(BHScriptVMContext *c, BHEntity *node, BHEntity *target, BHScriptVMActor *a, BHScriptVMState *s)
{
    BHEntity *e, *effective;
    BHScriptVMActor *binding;
    BHVec3 p, d, minimum, maximum, velocity, spin, anchor, angles;
    BHScriptLifetimeState life;
    BHScriptAttachedSoundState sound;
    BHScriptEmitterState emitter;
    BHScriptSensorState sensor;
    BHBlockDamageState damage;
    BHScriptVMWaypoint *waypoint;
    BHPortalNode *neighbor;
    long *args = s->args, n, kind;
    unsigned long i, j, flags;
    float t;
    struct
    {
        long event;
        BHEntity *actor;
        long value;
        unsigned long rest[7];
    } packet;
    memset(&life, 0, sizeof(life));
    life.actor = target;
    life.script_loaded = c->script_loading;
    memset(&packet, 0, sizeof(packet));
    packet.actor = target;
    switch (s->opcode)
    {
    case 0:
    case 16:
        effective = s->opcode == 16 ? owner(a) : target;
        binding = actor(c, effective);
        if (!binding)
        {
            c->error = 1;
            return 0;
        }
        flags = word(effective, 0x114);
        flags = args[0] ? flags | (unsigned long)args[1] : flags & ~(unsigned long)args[1];
        setword(effective, 0x114, flags);
        if (binding->world)
            binding->world->query_flags = flags;
        break;
    case 1:
        if (!c->lifetime->sounds)
        {
            c->error = 1;
            return 0;
        }
        if (!args[2])
        {
            if (!args[1])
                bh_sound_request(c->lifetime->sounds, (unsigned long)args[0], 1, 1, &target->position, 0);
            else
            {
                e = bh_entity_acquire(pool(c));
                if (!e)
                    return 0;
                memset(&sound, 0, sizeof(sound));
                sound.actor = target;
                sound.script_loaded = c->script_loading;
                sound.handle = BH_SOUND_INVALID;
                sound.resource = (unsigned long)args[0];
                if (!bh_script_attached_sound_bind(c->lifetime, e, &sound))
                {
                    c->error = 2;
                    return 0;
                }
                e->step = bh_script_attached_sound_tick;
            }
        }
        else
        {
            e = bh_entity_acquire(pool(c));
            if (!e)
                return 0;
            life.sound = bh_sound_request(c->lifetime->sounds, (unsigned long)args[0], 1, 1, &target->position, 0);
            life.countdown = (float)args[2];
            if (!bh_script_lifetime_bind(c->lifetime, e, &life, bh_script_sound_query))
            {
                c->error = 2;
                return 0;
            }
            e->step = bh_script_sound_expiry;
        }
        break;
    case 2:
    case 3:
        packet.event = s->opcode == 2 ? 9 : 10;
        if (!bh_entities_query(pool(c), 0, 0, 0, 0, &packet))
        {
            c->error = 2;
            return 0;
        }
        break;
    case 4:
        if (!c->contacts || !c->flash)
        {
            c->error = 1;
            return 0;
        }
        n = (long)((unsigned long)rand() * ((unsigned long)args[1] - (unsigned long)args[0] + 1UL)) / 32768 + args[0];
        if (n < 0 || n > 65535)
        {
            c->error = 3;
            return 0;
        }
        random_point(target, &p, 0);
        while (n--)
        {
            t = rf(.8 - (double)rand() * (-.00001220703125));
            bh_contact_spark_create(c->contacts, &p, t, 1);
        }
        e = bh_entity_acquire(pool(c));
        if (e)
        {
            e->position = p;
            if (!bh_flash_flare_bind(c->flash, e, .1f, 25, .5f))
            {
                c->error = 2;
                return 0;
            }
        }
        break;
    case 5:
    case 12:
        if (!c->motion)
        {
            c->error = 1;
            return 0;
        }
        e = bh_entity_acquire(pool(c));
        if (!e)
            return 0;
        life.countdown = (float)args[0];
        if (s->opcode == 5)
        {
            if (!delta(a, args + 1, 1.0 / 30.0, &life.delta))
            {
                c->error = 3;
                return 0;
            }
        }
        else
        {
            life.delta.x = rf(args[1] * (1.0 / 300.0));
            life.delta.y = rf(args[2] * (1.0 / 300.0));
            life.delta.z = rf(args[3] * (1.0 / 300.0));
        }
        if (!bh_script_motion_bind(c->motion, e, &life, s->opcode == 12))
        {
            c->error = 2;
            return 0;
        }
        if (s->opcode == 5)
        {
            target->origin.x = rf((double)target->origin.x + life.delta.x);
            target->origin.y = rf((double)target->origin.y + life.delta.y);
            target->origin.z = rf((double)target->origin.z + life.delta.z);
        }
        break;
    case 6:
    case 9:
        if (!c->emitters)
        {
            c->error = 1;
            return 0;
        }
        e = bh_entity_acquire(pool(c));
        if (!e)
            return 0;
        memset(&emitter, 0, sizeof(emitter));
        emitter.actor = target;
        emitter.script_loaded = c->script_loading;
        emitter.countdown = (float)args[0];
        emitter.size = args[4];
        if (!delta(a, args + 1, 1.0 / 30.0, &emitter.velocity))
        {
            c->error = 3;
            return 0;
        }
        if (s->opcode == 6)
        {
            emitter.acceleration.x = rf(emitter.velocity.x * (-1.0 / 30.0));
            emitter.acceleration.y = rf(-.004999999888241291 - (double)emitter.velocity.y * (1.0 / 30.0));
            emitter.acceleration.z = rf(emitter.velocity.z * (-1.0 / 30.0));
        }
        else
        {
            emitter.acceleration.y = .010911110788583755f;
            emitter.spread = rf(sqrt((double)emitter.velocity.x * emitter.velocity.x +
                                     (double)emitter.velocity.y * emitter.velocity.y +
                                     (double)emitter.velocity.z * emitter.velocity.z) *
                                (1.0 / 150000.0));
        }
        if (!bh_script_emitter_bind(c->emitters, e, &emitter, s->opcode == 9))
        {
            c->error = 2;
            return 0;
        }
        break;
    case 7:
        if (!c->explosions)
        {
            c->error = 1;
            return 0;
        }
        random_point(target, &p, 0);
        bh_script_explosion_create(c->explosions, &p, 1, (float)args[0], .75f, 0);
        break;
    case 8:
        if (!c->targets)
        {
            c->error = 1;
            return 0;
        }
        bh_target_construct_basic(c->targets, &target->position, args[0], args[1]);
        break;
    case 10:
    case 14:
    case 22:
        if (!c->sensors)
        {
            c->error = 1;
            return 0;
        }
        e = bh_entity_acquire(pool(c));
        if (!e)
            return 0;
        memset(&sensor, 0, sizeof(sensor));
        sensor.actor = target;
        sensor.script_loaded = c->script_loading;
        sensor.remaining = args[0];
        sensor.threshold = (long)((unsigned long)args[1] * (unsigned long)args[1]);
        sensor.broadcast_radius = args[2];
        sensor.expected = args[3];
        sensor.countdown = rf((double)rand() * (s->opcode == 10 ? (1.0 / 131072.0) : (1.0 / 32768.0)));
        if (s->opcode != 10)
        {
            if (!a->world)
            {
                c->error = 1;
                return 0;
            }
            a->damage.world = a->world;
            a->damage.player = c->sensors->player;
            damage.health = bh_script_sensor_damage(target);
            memcpy(&damage.owner, (char *)target + 0x14c, 4);
            if (!bh_block_damage_bind(pool(c), target, &a->damage, &damage, s->opcode == 22))
            {
                c->error = 2;
                return 0;
            }
        }
        if (!bh_script_sensor_bind(c->sensors, e, &sensor, s->opcode != 10))
        {
            c->error = 2;
            return 0;
        }
        if (s->opcode != 10)
            setword(e, 0x10c, (unsigned long)args[4]);
        if (s->opcode == 22)
        {
            point(target, &minimum, &maximum);
            for (i = 0; i < 3; ++i)
                ((float *)&p)[i] = rf(((double)((float *)&minimum)[i] + ((float *)&maximum)[i]) * .5);
            if (!radar(c, target, target, &p, 0))
                return 0;
        }
        break;
    case 11:
        if (!c->script_actors)
        {
            c->error = 1;
            return 0;
        }
        bh_script_actor_construct(c->script_actors, &target->position, (unsigned long)args[0]);
        break;
    case 13:
        if (!c->pickups)
        {
            c->error = 1;
            return 0;
        }
        random_point(target, &p, 1);
        if (args[0] < 20)
            bh_pickup_spawn(c->pickups, &p, args[0] < 10 ? args[0] : args[0] - 10, args[0] < 10 ? 0 : 1, args[1],
                            args[2], args[3], (float)args[4]);
        else
            bh_pickup_basic_spawn(c->pickups, &p, args[0], args[1], args[2], args[3], (float)args[4]);
        break;
    case 15:
        if (!c->targets || !args[3])
        {
            c->error = 1;
            return 0;
        }
        point(target, &minimum, &maximum);
        for (i = 0; i < 3; ++i)
        {
            ((float *)&p)[i] = rf(((double)((float *)&minimum)[i] + ((float *)&maximum)[i]) * .5);
            ((float *)&anchor)[i] = rf((double)((float *)&target->position)[i] - ((float *)&p)[i]);
        }
        for (i = 0; i < 3; ++i)
            ((float *)&spin)[i] = rf(((double)rand() - 16384) * (1.0 / 16384.0));
        velocity.x = rf(((double)rand() - 16384) * (1.0 / 16384.0));
        velocity.y = rf(((double)rand() - 16384) * (1.0 / 16384.0) - .6);
        velocity.z = rf(((double)rand() - 16384) * (1.0 / 16384.0));
        t = rf(100000.0 / args[3]);
        for (i = 0; i < 3; ++i)
            ((float *)&velocity)[i] = rf((double)((float *)&velocity)[i] * t);
        memcpy(&angles, (char *)target + 0x3c, 12);
        bh_target_construct_box(c->targets, &p, &angles, &velocity, &spin, a->resource, (float)args[3], (float)args[0],
                                args[1] ? (BHEntity *)word(target, 0x14c) : 0, args[2], &anchor, &minimum, &maximum, 0);
        break;
    case 17:
        if (!c->launch)
        {
            c->error = 1;
            return 0;
        }
        binding = actor(c, owner(a));
        if (!binding || !binding->world || args[0] < 0 || args[0] > 65535)
        {
            c->error = 1;
            return 0;
        }
        for (n = args[0]; n; n--)
            bh_actor_launch(c->launch, binding->world, (unsigned long)args[1], (long)((unsigned long)args[2] * 1000UL),
                            (float)args[3], args[4]);
        break;
    case 18:
        packet.event = 14;
        packet.value = args[1];
        if (!bh_entities_query(pool(c), 0, 0, &target->position, (float)args[0], &packet))
        {
            c->error = 2;
            return 0;
        }
        break;
    case 19:
        effective = owner(a);
        setword(effective, 0x11c, (unsigned long)args[0]);
        break;
    case 20:
        if (args[0] < 0 || (unsigned long)args[0] >= c->waypoint_count || !c->waypoints)
        {
            c->error = 1;
            return 0;
        }
        waypoint = c->waypoints + args[0];
        waypoint->position = target->position;
        memcpy(&waypoint->angles, (char *)target + 0x3c, 12);
        waypoint->speed = (float)args[1];
        waypoint->delay = (float)args[2];
        break;
    case 21:
        if (!c->rails)
        {
            c->error = 1;
            return 0;
        }
        d.x = d.y = d.z = 0;
        switch (a->orientation)
        {
        case 0:
            d.z = 1;
            break;
        case 1:
            d.x = 1;
            break;
        case 2:
            d.z = -1;
            break;
        case 3:
            d.x = -1;
            break;
        default:
            c->error = 3;
            return 0;
        }
        bh_rail_wagon_spawn(c->rails, &target->position, &d);
        break;
    case 23:
        binding = actor(c, owner(a));
        if (!binding || !binding->world || binding->world->navigation.count > 6)
        {
            c->error = 1;
            return 0;
        }
        for (i = 0; i < binding->world->navigation.count; ++i)
        {
            BHPortal *port = binding->world->navigation.portals + i;
            if (port->neighbor)
            {
                portal_extent(port, &target->position, rf(args[0] * .5), rf(args[1] * .5));
                neighbor = port->neighbor;
                if (neighbor->count > 6)
                {
                    c->error = 1;
                    return 0;
                }
                for (j = 0; j < neighbor->count; ++j)
                    if (neighbor->portals[j].neighbor)
                        portal_extent(neighbor->portals + j, &target->position, rf(args[0] * .5), rf(args[1] * .5));
            }
        }
        break;
    case 24:
        point(target, &minimum, &maximum);
        for (i = 0; i < 3; ++i)
            ((float *)&p)[i] = rf(((double)((float *)&minimum)[i] + ((float *)&maximum)[i]) * .5);
        if (!radar(c, target, target, &p, args[0]))
            return 0;
        break;
    default:
        break;
    }
    return 1;
}
void __cdecl bh_script_vm_query(BHEntity *e, void *packet)
{
    BHScriptVMContext *c = context(e);
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    BHScriptVMActor *a;
    BHScriptVMState s;
    unsigned long i, generation;
    BHEntity *next;
    if (!c || !packet || !live(pool(c), e))
        return;
    bh_script_vm_read(e, &s);
    if (p->event == 4 || (!s.loaded && p->event == 20))
    {
        bh_entity_release(pool(c), e);
        return;
    }
    if (p->event != 8)
        return;
    if (c->depth >= 32 || !bh_math_environment_supported() || !(a = actor(c, p->actor)))
    {
        c->error = 1;
        return;
    }
    ++c->depth;
    c->error = 0;
    generation = e->generation;
    if (s.variable_sum)
    {
        for (i = 0; i < 5; ++i)
            if (s.selectors[i] >= 0)
            {
                if (s.selectors[i] >= 22)
                {
                    c->error = 1;
                    --c->depth;
                    return;
                }
                s.args[i] = a->state->variables[s.selectors[i]];
                setword(e, 0xfc + i * 4, (unsigned long)s.args[i]);
            }
    }
    ++c->executed;
    if (execute(c, e, p->actor, a, &s) && live(pool(c), e) && e->generation == generation)
    {
        next = (BHEntity *)word(e, 0xec);
        if (next)
        {
            if (!live(pool(c), next) || !next->query)
                c->error = 2;
            else
                next->query(next, packet);
        }
    }
    --c->depth;
}
