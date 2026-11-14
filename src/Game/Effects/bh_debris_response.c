#include "bh_debris_response.h"
#include "../../Math/bh_fp.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <math.h>
#include <string.h>

static_assert(sizeof(BHDebrisBlastPacket) == 24, "BHDebrisBlastPacket size");
static_assert(offsetof(BHDebrisBlastPacket, radius) == 20, "BHDebrisBlastPacket.radius offset");
static_assert(sizeof(((BHEntity *)0)->payload) == 636 && sizeof(void *) == 4, "bridge size");
static BHDebrisBody *body_of(BHEntity *entity)
{
    BHDebrisBody *body;
    if (!entity)
        return NULL;
    memcpy(&body, &entity->bindings.debris_body, sizeof(body));
    return body;
}
static int live(BHDebrisContext *ctx, BHEntity *entity)
{
    return ctx && bh_entity_is_active(ctx->pool, entity);
}
static int registry_end(BHDebrisContext *ctx, unsigned long *end)
{
    unsigned long i;
    if (!ctx || !ctx->registry)
        return 0;
    for (i = 0; i < ctx->registry_capacity; ++i)
        if (!ctx->registry[i])
        {
            *end = i;
            return 1;
        }
    ctx->error = 1;
    return 0;
}
static BHDebrisBody *checked(BHEntity *entity)
{
    BHDebrisBody *body;
    if (!entity || (entity->query != bh_debris_receive && entity->step != bh_debris_remove))
        return NULL;
    body = body_of(entity);
    if (!body || body->entity != entity || body->generation != entity->generation || !live(body->context, entity))
        return NULL;
    return body;
}
int bh_debris_context_init(BHDebrisContext *ctx, BHEntityPool *pool, BHSoundQueue *sound, unsigned long id,
                           BHEntity **registry, unsigned long capacity)
{
    BHDebrisContext next;
    unsigned long end;
    if (!ctx || !pool || !sound || !registry || !capacity || capacity > 65536 || !bh_entities_valid(pool))
        return 0;
    memset(&next, 0, sizeof(next));
    next.pool = pool;
    next.sound = sound;
    next.contact_sound = id;
    next.registry = registry;
    next.registry_capacity = capacity;
    if (!registry_end(&next, &end))
        return 0;
    *ctx = next;
    return 1;
}
int bh_debris_context_dispose(BHDebrisContext *ctx)
{
    BHEntity *entity;
    BHDebrisBody *body;
    unsigned long steps = 0;
    if (!ctx || !ctx->pool)
        return 0;
    for (entity = ctx->pool->active; entity && steps++ < ctx->pool->capacity; entity = entity->next)
    {
        body = checked(entity);
        if (body && body->context == ctx)
        {
            ctx->error = 1;
            return 0;
        }
    }
    memset(ctx, 0, sizeof(*ctx));
    return 1;
}
int bh_debris_bind(BHDebrisContext *ctx, BHDebrisBody *body, BHEntity *entity, BHPhysicsState *physics, float mass,
                   float friction, const BHVec3 *inertia)
{
    BHDebrisBody next;
    if (!body || body->entity || !physics || !inertia || !live(ctx, entity) || entity->query ||
        entity->step == bh_debris_remove)
        return 0;
    memset(&next, 0, sizeof(next));
    next.context = ctx;
    next.entity = entity;
    next.generation = entity->generation;
    next.physics = physics;
    next.mass = mass;
    next.friction = friction;
    next.inertia = *inertia;
    next.last_sound = BH_SOUND_INVALID;
    if (!bh_entity_set_query(ctx->pool, entity, bh_debris_receive))
        return 0;
    *body = next;
    memcpy(&entity->bindings.debris_body, &body, sizeof(body));
    return 1;
}
int bh_debris_unbind(BHDebrisBody *body)
{
    BHEntity *entity;
    if (!body || !(entity = body->entity) || checked(entity) != body)
        return 0;
    if (!bh_entity_set_query(body->context->pool, entity, NULL))
        return 0;
    if (entity->step == bh_debris_remove)
        entity->step = NULL;
    body->entity = NULL;
    return 1;
}
void bh_debris_remove(BHEntity *entity)
{
    BHDebrisBody *body = checked(entity);
    unsigned long end, i, j;
    if (!body || !registry_end(body->context, &end))
        return;
    for (i = 0; i < end; ++i)
        if (body->context->registry[i] == entity)
        {
            for (j = i; j < end; ++j)
                body->context->registry[j] = body->context->registry[j + 1];
            break;
        }
    if (!bh_entity_release(body->context->pool, entity))
    {
        body->context->error = 1;
        return;
    }
    body->entity = NULL;
}
void bh_debris_receive(BHEntity *entity, void *packet)
{
    BHDebrisBody *body = checked(entity);
    BHPhysicsState *physics;
    const BHMat34 *pose;
    BHPhysicsContactPacket contact;
    BHDebrisBlastPacket blast;
    BHDebrisOwnerPacket owner;
    BHVec3 n, arm, impulse, torque, zero = {0, 0, 0};
    double x, y, z, length, factor, normal_factor, mass_impulse;
    float x_force, y_force, pitch;
    long event;
    unsigned long i;
    if (!body || !packet)
        return;
    memcpy(&event, packet, 4);
    physics = body->physics;
    pose = &physics->pose;
    switch (event)
    {
    case 6:
        entity->step = NULL;
        return;
    case 17:
        memcpy(&owner, packet, sizeof(owner));
        if (owner.owner == body->owner)
            body->owner = NULL;
        return;
    case 20:
        entity->step = bh_debris_remove;
        if (!bh_entity_set_query(body->context->pool, entity, NULL))
        {
            body->context->error = 1;
            return;
        }
        entity->refresh = NULL;
        return;
    case 1:
        if (!bh_math_environment_supported())
        {
            body->context->error = 1;
            return;
        }
        memcpy(&blast, packet, sizeof(blast));
        x = (double)entity->position.x - blast.center.x;
        y = (double)entity->position.y - blast.center.y;
        z = (double)entity->position.z - blast.center.z;
        length = sqrt((x * x + y * y) + z * z);
        if (blast.radius <= length)
            return;
        factor = (((double)blast.strength / body->mass) * (1.0 - length / blast.radius) * 30.0) / length;
        bh_round_float(&physics->acceleration.x, x * factor + physics->acceleration.x);
        bh_round_float(&physics->acceleration.y, y * factor + physics->acceleration.y);
        bh_round_float(&physics->acceleration.z, z * factor + physics->acceleration.z);
        break;
    case 2:
        if (!bh_math_environment_supported())
        {
            body->context->error = 1;
            return;
        }
        memcpy(&contact, packet, sizeof(contact));
        /* Original column dots have different addition orders. */
        bh_round_float(&n.x, ((double)contact.normal.y * pose->m[1][0] + (double)contact.normal.z * pose->m[2][0]) +
                                 (double)contact.normal.x * pose->m[0][0]);
        bh_round_float(&n.y, ((double)contact.normal.x * pose->m[0][1] + (double)contact.normal.z * pose->m[2][1]) +
                                 (double)contact.normal.y * pose->m[1][1]);
        bh_round_float(&n.z, ((double)contact.normal.y * pose->m[1][2] + (double)contact.normal.z * pose->m[2][2]) +
                                 (double)contact.normal.x * pose->m[0][2]);
        x = (double)contact.point.x - entity->position.x;
        y = (double)contact.point.y - entity->position.y;
        /* Z is spilled to float before the inverse column dots. */
        bh_round_float(&y_force, (double)contact.point.z - entity->position.z);
        z = y_force;
        bh_round_float(&arm.x, ((double)pose->m[0][0] * x + (double)pose->m[1][0] * y) + (double)pose->m[2][0] * z);
        bh_round_float(&arm.y, ((double)pose->m[0][1] * x + (double)pose->m[1][1] * y) + (double)pose->m[2][1] * z);
        bh_round_float(&arm.z, ((double)pose->m[0][2] * x + (double)pose->m[2][2] * z) + (double)pose->m[1][2] * y);
        bh_vec3_normalize(&arm);
        normal_factor = ((double)body->friction * contact.impulse) * 900.0;
        mass_impulse = (double)contact.impulse * body->mass;
        for (i = 0; i < 3; ++i)
            bh_round_float(((float *)&impulse) + i, ((double)((float *)&n)[i] * mass_impulse) * 270.0);
        bh_round_float(&y_force, (double)contact.normal.y * normal_factor);
        /* 0043cacd/0043cad7 reload the X force from its binary32 spill. */
        bh_round_float(&x_force, (double)contact.normal.x * normal_factor);
        bh_round_float(&physics->acceleration.x, (double)physics->acceleration.x - x_force);
        bh_round_float(&physics->acceleration.y, (double)physics->acceleration.y - y_force);
        bh_round_float(&physics->acceleration.z, (double)physics->acceleration.z - contact.normal.z * normal_factor);
        bh_vec3_cross(&arm, &impulse, &torque);
        for (i = 0; i < 3; ++i)
        {
            bh_round_float(((float *)&torque) + i, (double)((float *)&torque)[i] / ((float *)&body->inertia)[i]);
            bh_round_float(((float *)&physics->angular_acceleration) + i,
                           (double)((float *)&physics->angular_acceleration)[i] - ((float *)&torque)[i]);
            bh_round_float(((float *)&physics->angular_velocity) + i,
                           (double)((float *)&physics->angular_velocity)[i] * .85);
        }
        if (contact.severity > .05)
        {
            bh_round_float(&pitch, .9 - (double)contact.severity * -.2);
            body->last_sound = bh_sound_request(body->context->sound, body->context->contact_sound, contact.severity, pitch,
                                             &contact.point, &zero);
        }
        break;
    default:
        return;
    }
    /* Original response writes these common fields immediately. Keep the
       typed component and callback-visible entity coherent without republishing
       every body before every physics step. */
    entity->angular_velocity = physics->angular_velocity;
    entity->acceleration = physics->acceleration;
    entity->angular_acceleration = physics->angular_acceleration;
    if (body->sleep_time > 2)
        body->sleep_time = 0;
}
