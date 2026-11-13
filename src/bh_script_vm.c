#include "bh_field_access.h"
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
static void setword(BHEntity *entity, unsigned long o, unsigned long v)
{
    memcpy((char *)entity + o, &v, 4);
}
static BHScriptVMContext *context(BHEntity *entity)
{
    BHScriptVMContext *ctx;
    memcpy(&ctx, &entity->bindings.script_vm, 4);
    return ctx;
}
static BHEntityPool *pool(BHScriptVMContext *ctx)
{
    return ctx->lifetime->timers.pool;
}
static BHScriptVMActor *actor(BHScriptVMContext *ctx, BHEntity *entity)
{
    unsigned long i;
    long found;
    const BHBindingIndex *index = ctx->lifetime->timers.actor_index;
    if (index)
    {
        found = bh_binding_find(index, entity, NULL);
        if (found >= 0 && (unsigned long)found < ctx->actor_count && ctx->actors[found].state &&
            ctx->actors[found].state->entity == entity)
            return ctx->actors + found;
    }
    for (i = 0; i < ctx->actor_count; ++i)
        if (ctx->actors[i].state && ctx->actors[i].state->entity == entity)
            return ctx->actors + i;
    return 0;
}
static BHEntity *owner(BHScriptVMActor *a)
{
    return a->state->owner ? a->state->owner->entity : a->state->entity;
}
static int live(BHEntityPool *pool, BHEntity *entity)
{
    return bh_entity_is_active(pool, entity);
}
int bh_script_vm_read(BHEntity *entity, BHScriptVMState *state)
{
    if (!entity || !state)
        return 0;
    state->next = (BHEntity *)bh_field_u32(&entity->script_vm.next);
    state->loaded = (long)bh_field_u32(&entity->script_vm.loaded);
    state->opcode = (long)bh_field_u32(&entity->script_vm.opcode);
    memcpy(state->args, (char *)&entity->script_vm.args, 20);
    memcpy(state->selectors, (char *)&entity->script_vm.selectors, 20);
    state->variable_sum = (long)bh_field_u32(&entity->script_vm.variable_sum);
    return 1;
}
int bh_script_vm_bind(BHScriptVMContext *ctx, BHEntity *entity, const BHScriptVMState *state)
{
    if (!ctx || !ctx->lifetime || !ctx->lifetime->timers.pool || !state || !entity || !live(pool(ctx), entity))
        return 0;
    memcpy(&entity->bindings.script_vm, &ctx, 4);
    bh_field_store_u32(&entity->script_vm.next, (unsigned long)state->next);
    bh_field_store_u32(&entity->script_vm.loaded, (unsigned long)state->loaded);
    bh_field_store_u32(&entity->script_vm.opcode, (unsigned long)state->opcode);
    memcpy((char *)&entity->script_vm.args, state->args, 20);
    memcpy((char *)&entity->script_vm.selectors, state->selectors, 20);
    bh_field_store_u32(&entity->script_vm.variable_sum, (unsigned long)state->variable_sum);
    return bh_entity_set_query(pool(ctx), entity, bh_script_vm_query);
}
BHEntity *bh_script_vm_create(BHScriptVMContext *ctx, BHEntity **out, long opcode, const long args[5],
                              const long selectors[5])
{
    BHScriptVMState state;
    BHEntity *entity;
    unsigned long sum = 5, i;
    if (!ctx || !ctx->lifetime || !ctx->lifetime->timers.pool || !args || !selectors)
        return 0;
    entity = bh_entity_acquire(pool(ctx));
    if (!entity)
        return 0;
    state.next = 0;
    state.loaded = ctx->script_loading;
    state.opcode = opcode;
    for (i = 0; i < 5; ++i)
    {
        state.args[i] = args[i];
        state.selectors[i] = selectors[i];
        sum += (unsigned long)selectors[i];
    }
    memcpy(&state.variable_sum, &sum, 4);
    entity->position = pool(ctx)->origin;
    if (!bh_script_vm_bind(ctx, entity, &state))
    {
        bh_entity_release(pool(ctx), entity);
        return 0;
    }
    if (out)
        *out = entity;
    return entity;
}
static void point(BHEntity *entity, BHVec3 *minimum, BHVec3 *maximum)
{
    memcpy(minimum, (char *)&entity->placement.minimum, 12);
    memcpy(maximum, (char *)&entity->placement.maximum, 12);
}
static void random_point(BHEntity *entity, BHVec3 *p, int inner)
{
    BHVec3 lo, hi;
    unsigned long i;
    double t;
    point(entity, &lo, &hi);
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
static void radar_query(BHEntity *entity, void *packet)
{
    BHScriptVMContext *ctx;
    memcpy(&ctx, &entity->bindings.radar_vm, 4);
    ctx->radar->head = ctx->launch->actors->response->vehicles->head;
    ctx->radar->count = ctx->launch->actors->response->vehicles->count;
    bh_radar_entity_query(entity, packet);
    ctx->launch->actors->response->vehicles->head = ctx->radar->head;
    ctx->launch->actors->response->vehicles->count = ctx->radar->count;
}
static int radar(BHScriptVMContext *ctx, BHEntity *entity, BHEntity *watched, const BHVec3 *p, long remove)
{
    BHEntity *e;
    BHActorVehicleLifetimeContext *v;
    if (!ctx->radar || !ctx->launch || !ctx->launch->actors)
    {
        ctx->error = 1;
        return 0;
    }
    e = bh_entity_acquire(pool(ctx));
    if (!e)
        return 0;
    e->position = *p;
    e->flags |= 0x100;
    e->actor.sound_frames = 0;
    e->actor.kind = 6;
    e->actor.radar_id = 0;
    v = ctx->launch->actors->response->vehicles;
    ctx->radar->head = v->head;
    ctx->radar->count = v->count;
    if (!bh_radar_query_bind(ctx->radar, e, watched, remove))
    {
        bh_entity_release(pool(ctx), e);
        ctx->error = 2;
        return 0;
    }
    v->head = ctx->radar->head;
    v->count = ctx->radar->count;
    memcpy(&e->bindings.radar_vm, &ctx, 4);
    bh_entity_set_query(pool(ctx), e, radar_query);
    return 1;
}
static int execute(BHScriptVMContext *ctx, BHEntity *node, BHEntity *target, BHScriptVMActor *a, BHScriptVMState *state)
{
    BHEntity *entity, *effective;
    BHScriptVMActor *binding;
    BHVec3 p, d, minimum, maximum, velocity, spin, anchor, angles;
    BHScriptLifetimeState life;
    BHScriptAttachedSoundState sound;
    BHScriptEmitterState emitter;
    BHScriptSensorState sensor;
    BHBlockDamageState damage;
    BHScriptVMWaypoint *waypoint;
    BHPortalNode *neighbor;
    long *args = state->args, n, kind;
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
    life.script_loaded = ctx->script_loading;
    memset(&packet, 0, sizeof(packet));
    packet.actor = target;
    switch (state->opcode)
    {
    case 0:
    case 16:
        effective = state->opcode == 16 ? owner(a) : target;
        binding = actor(ctx, effective);
        if (!binding)
        {
            ctx->error = 1;
            return 0;
        }
        flags = bh_field_u32(&effective->placement.query_flags);
        flags = args[0] ? flags | (unsigned long)args[1] : flags & ~(unsigned long)args[1];
        bh_field_store_u32(&effective->placement.query_flags, flags);
        if (binding->world)
            binding->world->query_flags = flags;
        break;
    case 1:
        if (!ctx->lifetime->sounds)
        {
            ctx->error = 1;
            return 0;
        }
        if (!args[2])
        {
            if (!args[1])
                bh_sound_request(ctx->lifetime->sounds, (unsigned long)args[0], 1, 1, &target->position, 0);
            else
            {
                entity = bh_entity_acquire(pool(ctx));
                if (!entity)
                    return 0;
                memset(&sound, 0, sizeof(sound));
                sound.actor = target;
                sound.script_loaded = ctx->script_loading;
                sound.handle = BH_SOUND_INVALID;
                sound.resource = (unsigned long)args[0];
                if (!bh_script_attached_sound_bind(ctx->lifetime, entity, &sound))
                {
                    ctx->error = 2;
                    return 0;
                }
                entity->step = bh_script_attached_sound_tick;
            }
        }
        else
        {
            entity = bh_entity_acquire(pool(ctx));
            if (!entity)
                return 0;
            life.sound = bh_sound_request(ctx->lifetime->sounds, (unsigned long)args[0], 1, 1, &target->position, 0);
            life.countdown = (float)args[2];
            if (!bh_script_lifetime_bind(ctx->lifetime, entity, &life, bh_script_sound_query))
            {
                ctx->error = 2;
                return 0;
            }
            entity->step = bh_script_sound_expiry;
        }
        break;
    case 2:
    case 3:
        packet.event = state->opcode == 2 ? 9 : 10;
        if (!bh_entities_query(pool(ctx), 0, 0, 0, 0, &packet))
        {
            ctx->error = 2;
            return 0;
        }
        break;
    case 4:
        if (!ctx->contacts || !ctx->flash)
        {
            ctx->error = 1;
            return 0;
        }
        n = (long)((unsigned long)rand() * ((unsigned long)args[1] - (unsigned long)args[0] + 1UL)) / 32768 + args[0];
        if (n < 0 || n > 65535)
        {
            ctx->error = 3;
            return 0;
        }
        random_point(target, &p, 0);
        while (n--)
        {
            t = rf(.8 - (double)rand() * (-.00001220703125));
            bh_contact_spark_create(ctx->contacts, &p, t, 1);
        }
        entity = bh_entity_acquire(pool(ctx));
        if (entity)
        {
            entity->position = p;
            if (!bh_flash_flare_bind(ctx->flash, entity, .1f, 25, .5f))
            {
                ctx->error = 2;
                return 0;
            }
        }
        break;
    case 5:
    case 12:
        if (!ctx->motion)
        {
            ctx->error = 1;
            return 0;
        }
        entity = bh_entity_acquire(pool(ctx));
        if (!entity)
            return 0;
        life.countdown = (float)args[0];
        if (state->opcode == 5)
        {
            if (!delta(a, args + 1, 1.0 / 30.0, &life.delta))
            {
                ctx->error = 3;
                return 0;
            }
        }
        else
        {
            life.delta.x = rf(args[1] * (1.0 / 300.0));
            life.delta.y = rf(args[2] * (1.0 / 300.0));
            life.delta.z = rf(args[3] * (1.0 / 300.0));
        }
        if (!bh_script_motion_bind(ctx->motion, entity, &life, state->opcode == 12))
        {
            ctx->error = 2;
            return 0;
        }
        if (state->opcode == 5)
        {
            target->origin.x = rf((double)target->origin.x + life.delta.x);
            target->origin.y = rf((double)target->origin.y + life.delta.y);
            target->origin.z = rf((double)target->origin.z + life.delta.z);
        }
        break;
    case 6:
    case 9:
        if (!ctx->emitters)
        {
            ctx->error = 1;
            return 0;
        }
        entity = bh_entity_acquire(pool(ctx));
        if (!entity)
            return 0;
        memset(&emitter, 0, sizeof(emitter));
        emitter.actor = target;
        emitter.script_loaded = ctx->script_loading;
        emitter.countdown = (float)args[0];
        emitter.size = args[4];
        if (!delta(a, args + 1, 1.0 / 30.0, &emitter.velocity))
        {
            ctx->error = 3;
            return 0;
        }
        if (state->opcode == 6)
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
        if (!bh_script_emitter_bind(ctx->emitters, entity, &emitter, state->opcode == 9))
        {
            ctx->error = 2;
            return 0;
        }
        break;
    case 7:
        if (!ctx->explosions)
        {
            ctx->error = 1;
            return 0;
        }
        random_point(target, &p, 0);
        bh_script_explosion_create(ctx->explosions, &p, 1, (float)args[0], .75f, 0);
        break;
    case 8:
        if (!ctx->targets)
        {
            ctx->error = 1;
            return 0;
        }
        bh_target_construct_basic(ctx->targets, &target->position, args[0], args[1]);
        break;
    case 10:
    case 14:
    case 22:
        if (!ctx->sensors)
        {
            ctx->error = 1;
            return 0;
        }
        entity = bh_entity_acquire(pool(ctx));
        if (!entity)
            return 0;
        memset(&sensor, 0, sizeof(sensor));
        sensor.actor = target;
        sensor.script_loaded = ctx->script_loading;
        sensor.remaining = args[0];
        sensor.threshold = (long)((unsigned long)args[1] * (unsigned long)args[1]);
        sensor.broadcast_radius = args[2];
        sensor.expected = args[3];
        sensor.countdown = rf((double)rand() * (state->opcode == 10 ? (1.0 / 131072.0) : (1.0 / 32768.0)));
        if (state->opcode != 10)
        {
            if (!a->world)
            {
                ctx->error = 1;
                return 0;
            }
            a->damage.world = a->world;
            a->damage.player = ctx->sensors->player;
            damage.health = bh_script_sensor_damage(target);
            memcpy(&damage.owner, (char *)&target->placement.damage_source, 4);
            if (!bh_block_damage_bind(pool(ctx), target, &a->damage, &damage, state->opcode == 22))
            {
                ctx->error = 2;
                return 0;
            }
        }
        if (!bh_script_sensor_bind(ctx->sensors, entity, &sensor, state->opcode != 10))
        {
            ctx->error = 2;
            return 0;
        }
        /* The fifth sensor argument is retained at +10c; no consumer is recovered. */
        if (state->opcode != 10)
            setword(entity, 0x10c, (unsigned long)args[4]);
        if (state->opcode == 22)
        {
            point(target, &minimum, &maximum);
            for (i = 0; i < 3; ++i)
                ((float *)&p)[i] = rf(((double)((float *)&minimum)[i] + ((float *)&maximum)[i]) * .5);
            if (!radar(ctx, target, target, &p, 0))
                return 0;
        }
        break;
    case 11:
        if (!ctx->script_actors)
        {
            ctx->error = 1;
            return 0;
        }
        bh_script_actor_construct(ctx->script_actors, &target->position, (unsigned long)args[0]);
        break;
    case 13:
        if (!ctx->pickups)
        {
            ctx->error = 1;
            return 0;
        }
        random_point(target, &p, 1);
        if (args[0] < 20)
            bh_pickup_spawn(ctx->pickups, &p, args[0] < 10 ? args[0] : args[0] - 10, args[0] < 10 ? 0 : 1, args[1],
                            args[2], args[3], (float)args[4]);
        else
            bh_pickup_basic_spawn(ctx->pickups, &p, args[0], args[1], args[2], args[3], (float)args[4]);
        break;
    case 15:
        if (!ctx->targets || !args[3])
        {
            ctx->error = 1;
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
        angles = target->euler;
        bh_target_construct_box(ctx->targets, &p, &angles, &velocity, &spin, a->resource, (float)args[3],
                                (float)args[0],
                                args[1] ? (BHEntity *)bh_field_u32(&target->placement.damage_source) : 0, args[2],
                                &anchor, &minimum, &maximum, 0);
        break;
    case 17:
        if (!ctx->launch)
        {
            ctx->error = 1;
            return 0;
        }
        binding = actor(ctx, owner(a));
        if (!binding || !binding->world || args[0] < 0 || args[0] > 65535)
        {
            ctx->error = 1;
            return 0;
        }
        for (n = args[0]; n; n--)
            bh_actor_launch(ctx->launch, binding->world, (unsigned long)args[1],
                            (long)((unsigned long)args[2] * 1000UL), (float)args[3], args[4]);
        break;
    case 18:
        packet.event = 14;
        packet.value = args[1];
        if (!bh_entities_query(pool(ctx), 0, 0, &target->position, (float)args[0], &packet))
        {
            ctx->error = 2;
            return 0;
        }
        break;
    case 19:
        effective = owner(a);
        bh_field_store_u32(&effective->placement.city_id, (unsigned long)args[0]);
        break;
    case 20:
        if (args[0] < 0 || (unsigned long)args[0] >= ctx->waypoint_count || !ctx->waypoints)
        {
            ctx->error = 1;
            return 0;
        }
        waypoint = ctx->waypoints + args[0];
        waypoint->position = target->position;
        waypoint->angles = target->euler;
        waypoint->speed = (float)args[1];
        waypoint->delay = (float)args[2];
        break;
    case 21:
        if (!ctx->rails)
        {
            ctx->error = 1;
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
            ctx->error = 3;
            return 0;
        }
        bh_rail_wagon_spawn(ctx->rails, &target->position, &d);
        break;
    case 23:
        binding = actor(ctx, owner(a));
        if (!binding || !binding->world || binding->world->navigation.count > 6)
        {
            ctx->error = 1;
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
                    ctx->error = 1;
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
        if (!radar(ctx, target, target, &p, args[0]))
            return 0;
        break;
    default:
        break;
    }
    return 1;
}
void bh_script_vm_query(BHEntity *entity, void *packet)
{
    BHScriptVMContext *ctx = context(entity);
    BHScriptTimerPacket *p = (BHScriptTimerPacket *)packet;
    BHScriptVMActor *a;
    BHScriptVMState state;
    unsigned long i, generation;
    BHEntity *next;
    if (!ctx || !packet || !live(pool(ctx), entity))
        return;
    bh_script_vm_read(entity, &state);
    if (p->event == 4 || (!state.loaded && p->event == 20))
    {
        bh_entity_release(pool(ctx), entity);
        return;
    }
    if (p->event != 8)
        return;
    if (ctx->depth >= 32 || !bh_math_environment_supported() || !(a = actor(ctx, p->actor)))
    {
        ctx->error = 1;
        return;
    }
    ++ctx->depth;
    ctx->error = 0;
    generation = entity->generation;
    if (state.variable_sum)
    {
        for (i = 0; i < 5; ++i)
            if (state.selectors[i] >= 0)
            {
                if (state.selectors[i] >= 22)
                {
                    ctx->error = 1;
                    --ctx->depth;
                    return;
                }
                state.args[i] = a->state->variables[state.selectors[i]];
                entity->script_vm.args[i] = state.args[i];
            }
    }
    ++ctx->executed;
    if (execute(ctx, entity, p->actor, a, &state) && live(pool(ctx), entity) && entity->generation == generation)
    {
        next = (BHEntity *)bh_field_u32(&entity->script_vm.next);
        if (next)
        {
            if (!live(pool(ctx), next) || !next->query)
                ctx->error = 2;
            else
                next->query(next, packet);
        }
    }
    --ctx->depth;
}
