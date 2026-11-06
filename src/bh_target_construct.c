#include "bh_target_construct.h"
#include "bh_fp.h"
#include "bh_geometry_radius.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <string.h>
static int live(BHEntityPool *pool, BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(pool, entity) && entity->generation == generation;
}
static float rounded(double x)
{
    float f;
    bh_round_float(&f, x);
    return f;
}
static void identity(BHMat34 *m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}
static BHTargetConstructSlot *slot(BHEntity *entity)
{
    BHTargetConstructSlot *target_slot;
    memcpy(&target_slot, entity->payload + 608, 4);
    return target_slot;
}
static void __cdecl refresh(BHEntity *entity, void *p)
{
    BHTargetConstructSlot *target_slot = slot(entity);
    (void)p;
    memcpy((char *)entity + 0x128, &target_slot->body.owner, 4);
    if (bh_target_draw(target_slot->context->draw, entity) < 1)
        target_slot->context->error = 1;
}
static void __cdecl step(BHEntity *entity)
{
    BHTargetConstructSlot *target_slot = slot(entity);
    BHTargetConstructContext *ctx = target_slot->context;
    if (ctx->synced_retirements != ctx->debris->pool->retirements && !bh_target_construct_sync(ctx))
        return;
    bh_target_step(entity);
    if (live(ctx->debris->pool, entity, target_slot->body.generation))
        bh_mat34_inverse_rigid(&target_slot->state.pose, &target_slot->inverse);
    if (ctx->synced_retirements != ctx->debris->pool->retirements && !bh_target_construct_sync(ctx))
        ctx->error = 1;
}
static int same_body(const BHPhysicsStepBody *body, const BHPhysicsStepBody *b)
{
    return body->entity == b->entity && body->generation == b->generation && body->physics == b->physics &&
           body->geometry == b->geometry && body->target_world == b->target_world &&
           body->target_local == b->target_local && body->model_index == b->model_index;
}
int __cdecl bh_target_construct_sync(BHTargetConstructContext *ctx)
{
    unsigned long i, n, m;
    BHPhysicsStepBody *body, next;
    BHTargetConstructSlot *target_slot;
    BHEntity *entity;
    int retain, rebind;
    if (!ctx || !ctx->debris || !ctx->physics || !ctx->roster || !ctx->models || !ctx->slots || !ctx->capacity ||
        !ctx->effects || (ctx->base_count && !ctx->base_bodies) || (ctx->base_model_count && !ctx->base_models) ||
        ctx->physics->busy)
    {
        if (ctx)
            ctx->error = 1;
        return 0;
    }
    n = 0;
    m = ctx->base_model_count;
    retain = ctx->physics->allocator.release && ctx->physics->pool == ctx->debris->pool &&
             ctx->physics->bodies == ctx->roster && ctx->physics->body_count == ctx->roster_count &&
             ctx->physics->models == ctx->models && ctx->physics->model_count == ctx->model_count &&
             ctx->physics->worlds == ctx->worlds && ctx->physics->world_count == ctx->world_count &&
             ctx->physics->effects == ctx->effects && ctx->physics->fallback == 1;
    rebind = ctx->physics->allocator.release && ctx->physics->pool == ctx->debris->pool &&
             ctx->physics->worlds == ctx->worlds && ctx->physics->world_count == ctx->world_count &&
             ctx->physics->effects == ctx->effects && ctx->physics->fallback == 1;
    for (i = 0; i < m; ++i)
    {
        if (retain && (i >= ctx->model_count || ctx->models[i] != ctx->base_models[i]))
            retain = 0;
        ctx->models[i] = ctx->base_models[i];
    }
    for (i = 0; i < ctx->base_count; ++i)
        if (live(ctx->debris->pool, ctx->base_bodies[i].entity, ctx->base_bodies[i].generation))
        {
            next = ctx->base_bodies[i];
            if (retain && (n >= ctx->roster_count || !same_body(ctx->roster + n, &next)))
                retain = 0;
            ctx->roster[n] = next;
            memcpy(&ctx->roster[n].world, (const char *)ctx->base_bodies[i].entity + 0xc4, 4);
            ++n;
        }
    for (i = 0; i < ctx->capacity; ++i)
    {
        target_slot = ctx->slots + i;
        entity = target_slot->body.entity;
        if (!entity)
            continue;
        if (!live(ctx->debris->pool, entity, target_slot->body.generation))
        {
            target_slot->body.entity = NULL;
            continue;
        }
        body = &next;
        memset(body, 0, sizeof(*body));
        body->entity = entity;
        body->generation = entity->generation;
        body->physics = &target_slot->state;
        body->geometry = &target_slot->collision.geometry;
        body->target_world = &target_slot->state.pose;
        body->target_local = &target_slot->inverse;
        body->mass = target_slot->body.mass;
        body->friction = target_slot->body.friction;
        /* Physics publishes its selected room into the shared actual entity.
       Membership rebuild must retain that hint, including sleeping actors. */
        memcpy(&body->world, (const char *)entity + 0xc4, 4);
        body->model_index = m;
        memcpy(&body->flags, (char *)entity + 0x114, 4);
        if (retain && (n >= ctx->roster_count || !same_body(ctx->roster + n, body) || m >= ctx->model_count ||
                       ctx->models[m] != body->geometry))
            retain = 0;
        ctx->roster[n++] = *body;
        ctx->models[m++] = body->geometry;
    }
    if (retain && n == ctx->roster_count && m == ctx->model_count)
    {
        /* State, room hints and force/query changes remain live; only topology
       workspaces are retained. Preserve init's common-field publication. */
        for (i = 0; i < n; ++i)
            if (!bh_physics_step_publish(ctx->roster + i))
            {
                ctx->error = 1;
                return 0;
            }
        ctx->synced_retirements = ctx->debris->pool->retirements;
        return 1;
    }
    if (rebind && n)
    {
        if (!bh_physics_step_rebind(ctx->physics, ctx->roster, n, ctx->models, m))
        {
            ctx->error = 1;
            return 0;
        }
        ctx->roster_count = n;
        ctx->model_count = m;
        ++ctx->rebuilds;
        ctx->synced_retirements = ctx->debris->pool->retirements;
        return 1;
    }
    /* Boundary rebuild preserves stable state/matrix pointers and rejects stale
   * generations. */
    if (ctx->physics->allocator.release && !bh_physics_step_dispose(ctx->physics))
    {
        ctx->error = 1;
        return 0;
    }
    ctx->roster_count = n;
    ctx->model_count = m;
    if (!n)
    {
        ++ctx->rebuilds;
        ctx->synced_retirements = ctx->debris->pool->retirements;
        return 1;
    }
    if (!m || !bh_physics_step_init(ctx->physics, ctx->debris->pool, ctx->roster, n, ctx->worlds, ctx->world_count,
                                    ctx->models, m, ctx->effects, 1, NULL))
    {
        ctx->error = 1;
        return 0;
    }
    ++ctx->rebuilds;
    ctx->synced_retirements = ctx->debris->pool->retirements;
    return 1;
}
int __cdecl bh_target_construct_prune_retired(BHTargetConstructContext *ctx)
{
    const BHPhysicsStepContext *physics = ctx->physics;
    BHEntityPool *pool = ctx->debris->pool;
    unsigned long i;
    if (physics->validated_retirements == pool->retirements || ctx->synced_retirements == pool->retirements)
        return 1;
    for (i = 0; i < physics->body_count; ++i)
        if (!live(pool, physics->bodies[i].entity, physics->bodies[i].generation))
            return bh_target_construct_sync(ctx);
    return 1;
}
int __cdecl bh_target_construct_dispose(BHTargetConstructContext *ctx)
{
    unsigned long i;
    BHTargetConstructSlot *target_slot;
    if (!ctx || !ctx->physics || ctx->physics->busy)
        return 0;
    for (i = 0; i < ctx->capacity; ++i)
    {
        target_slot = ctx->slots + i;
        if (target_slot->body.entity)
            return 0;
    }
    if (ctx->physics->allocator.release && !bh_physics_step_dispose(ctx->physics))
        return 0;
    for (i = 0; i < ctx->capacity; ++i)
    {
        target_slot = ctx->slots + i;
        if (target_slot->initialized && !bh_collision_model_dispose(&target_slot->collision))
            return 0;
        memset(target_slot, 0, sizeof(*target_slot));
    }
    ctx->roster_count = ctx->model_count = 0;
    return 1;
}
static int placement(BHTargetConstructContext *ctx, BHTargetConstructSlot *target_slot, BHGeometry *g, float radius)
{
    const BHPhysicsStepBody *body = ctx->placement_focus;
    BHSweepQuery q;
    BHCollisionHit hit;
    BHMat34 fm, fi;
    BHVec3 d;
    float distance, scale;
    int r;
    unsigned long count = 0;
    BHEntity *entity = target_slot->body.entity;
    if (!body)
        return 1;
    if (!body->entity || !body->physics || !live(ctx->debris->pool, body->entity, body->generation))
        return 0;
    d.x = rounded((double)entity->position.x - body->entity->position.x);
    d.y = rounded((double)entity->position.y - body->entity->position.y);
    d.z = rounded((double)entity->position.z - body->entity->position.z);
    distance = rounded(sqrt(((double)d.z * d.z + (double)d.y * d.y) + (double)d.x * d.x));
    if (!(distance < (double)body->entity->collision_radius + radius))
        return 1;
    scale = rounded(1.0 / distance);
    if (!_finite(scale))
        return 0;
    fm = body->physics->pose;
    bh_mat34_inverse_rigid(&fm, &fi);
    memset(&q, 0, sizeof(q));
    memset(&hit, 0, sizeof(hit));
    if (!bh_sweep_init(&q, g, body->geometry))
        return 0;
    r = bh_physics_overlap(&q, &target_slot->state.pose, &fm, &fi, &hit);
    while (r > 0)
    {
        do
        {
            if (++count > 65536)
            {
                r = -1;
                break;
            }
            bh_mat34_translate_world(rounded((double)d.x * scale), rounded((double)d.y * scale),
                                     rounded((double)d.z * scale), &target_slot->state.pose);
            entity->position.x = rounded((double)entity->position.x + d.x * scale);
            entity->position.y = rounded((double)entity->position.y + d.y * scale);
            entity->position.z = rounded((double)entity->position.z + d.z * scale);
            r = bh_collision_cached_contact(g->bsp, hit.a_face, &target_slot->state.pose, body->geometry->bsp,
                                            hit.b_face, hit.b_node, &fm, &fi, &hit.normal, &hit.contact);
        } while (r > 0);
        if (r < 0)
            break;
        r = bh_physics_overlap(&q, &target_slot->state.pose, &fm, &fi, &hit);
    }
    bh_sweep_dispose(&q);
    return r == 0;
}
static BHTargetConstructSlot *begin(BHTargetConstructContext *ctx, const BHVec3 *p, const BHVec3 *a, float mass,
                                    int box)
{
    BHTargetConstructSlot *target_slot = NULL;
    BHEntity *entity;
    BHMat34 rotation;
    unsigned long i, end;
    BHVec3 inertia;
    if (!ctx || !p || !a || !ctx->draw || !ctx->pickups || !ctx->particles || !ctx->explosions || !ctx->mode ||
        !ctx->debris || !ctx->slots || !ctx->roster || !ctx->models || !bh_math_environment_supported() ||
        !_finite(mass))
    {
        if (ctx)
            ctx->error = 1;
        return NULL;
    }
    ctx->error = 0;
    for (end = 0; end < ctx->debris->registry_capacity && ctx->debris->registry[end]; ++end)
        ;
    if (end + 1 >= ctx->debris->registry_capacity)
    {
        ctx->error = 1;
        return NULL;
    }
    for (i = 0; i < ctx->capacity; ++i)
        if (!ctx->slots[i].body.entity)
        {
            target_slot = ctx->slots + i;
            break;
        }
    if (!target_slot)
    {
        ctx->error = 1;
        return NULL;
    }
    if (box && ctx->placement_focus && !ctx->legacy_bound)
    {
        ctx->error = 1;
        return NULL;
    }
    /* Retired slots can still be borrowed by old sweep workspaces. End those
     borrows before replacing the slot's collision topology. */
    if (!bh_target_construct_sync(ctx))
        return NULL;
    if (!target_slot->initialized)
    {
        memset(target_slot, 0, sizeof(*target_slot));
        if (!bh_collision_model_init(&target_slot->collision, NULL))
            return NULL;
        target_slot->initialized = 1;
    }
    target_slot->context = ctx;
    entity = bh_entity_acquire(ctx->debris->pool);
    if (!entity)
        return NULL;
    target_slot->body.entity = entity;
    target_slot->body.generation = entity->generation;
    /* Every original constructor clears +c4, including recycled entity slots. */
    memset((char *)entity + 0xc4, 0, 4);
    memset(&target_slot->state, 0, sizeof(target_slot->state));
    identity(&rotation);
    bh_mat34_rotate_y(a->x, &rotation);
    bh_mat34_rotate_x(a->y, &rotation);
    bh_mat34_rotate_z(a->z, &rotation);
    target_slot->state.pose = rotation;
    for (i = 0; i < 3; ++i)
        target_slot->state.pose.m[i][3] = ((const float *)p)[i];
    entity->position = *p;
    if (!box)
    {
        if (!bh_collision_model_load(&target_slot->collision, "HUN_LOD1.BSP", ctx->prefix, ctx->search))
            goto rejected;
        entity->collision_radius = rounded(bh_geometry_radius(&target_slot->collision.bsp));
    }
    else
        entity->collision_radius = ctx->legacy_radius;
    if (!placement(ctx, target_slot, box ? ctx->legacy_geometry : &target_slot->collision.geometry,
                   entity->collision_radius))
        goto rejected;
    target_slot->state.last_clear = target_slot->state.pose;
    target_slot->state.euler = *a;
    bh_mat34_inverse_rigid(&target_slot->state.pose, &target_slot->inverse);
    inertia.x = inertia.y = inertia.z = rounded((double)mass * 1.73);
    /* The real binder requires a fresh body with no entity yet. */
    memset(&target_slot->body, 0, sizeof(target_slot->body));
    if (!bh_debris_bind(ctx->debris, &target_slot->body, entity, &target_slot->state, mass, .9f, &inertia))
        goto rejected;
    memcpy((char *)entity + 0x78, &inertia, 12);
    memcpy(entity->payload + 608, &target_slot, 4);
    return target_slot;
rejected:
    bh_entity_release(ctx->debris->pool, entity);
    target_slot->body.entity = NULL;
    ctx->error = 1;
    return NULL;
}
static BHEntity *finish(BHTargetConstructContext *ctx, BHTargetConstructSlot *target_slot, unsigned long resource,
                        const BHVec3 *velocity, const BHVec3 *spin, float countdown, BHEntity *owner, long token,
                        long loot, long counter, long emission, const BHVec3 *anchor, const BHVec3 *minimum,
                        const BHVec3 *maximum)
{
    BHEntity *entity = target_slot->body.entity;
    BHTargetStepState ts;
    BHTargetDrawState ds;
    unsigned long n;
    target_slot->state.velocity = *velocity;
    target_slot->state.angular_velocity = *spin;
    target_slot->body.owner = owner;
    target_slot->body.sleep_time = 0;
    memset(&ts, 0, sizeof(ts));
    ts.countdown = countdown;
    ts.token = token;
    ts.loot = loot;
    ts.counter = counter;
    ts.emission = emission;
    if (minimum)
        ts.minimum = *minimum;
    if (maximum)
        ts.maximum = *maximum;
    memset(&ds, 0, sizeof(ds));
    ds.resource = resource;
    ds.number = token;
    ds.flags = counter;
    ds.owner = owner;
    if (anchor)
        ds.local_anchor = *anchor;
    bh_target_draw_state_write(entity, &ds);
    bh_target_step_write(entity, &ts);
    memset((char *)entity + 0xe8, 0, 4);
    memcpy((char *)entity + 0xbc, &target_slot->collision.geometry.bsp, 4);
    for (n = 0; n < ctx->debris->registry_capacity && ctx->debris->registry[n]; ++n)
        ;
    ctx->debris->registry[n] = entity;
    ctx->debris->registry[n + 1] = NULL;
    if (!bh_target_construct_sync(ctx))
        goto rejected;
    memset(&target_slot->step, 0, sizeof(target_slot->step));
    target_slot->step.body = &target_slot->body;
    target_slot->step.physics = ctx->physics;
    target_slot->step.explosions = ctx->explosions;
    target_slot->step.particles = ctx->particles;
    target_slot->step.pickups = ctx->pickups;
    target_slot->step.mode = ctx->mode;
    target_slot->step.smoke_image = ctx->smoke_image;
    if (!bh_target_step_bind(&target_slot->step, &ts))
        goto rejected;
    entity->step = step;
    entity->refresh = refresh;
    return entity;
rejected:
    bh_debris_remove(entity);
    bh_target_construct_sync(ctx);
    ctx->error = 1;
    return NULL;
}
BHEntity *__cdecl bh_target_construct_basic(BHTargetConstructContext *ctx, const BHVec3 *p, long index, long mass)
{
    BHVec3 zero = {0, 0, 0};
    BHTargetConstructSlot *target_slot;
    if (!ctx || index < 0 || (unsigned long)index >= ctx->resource_count || !ctx->resources)
        return NULL;
    target_slot = begin(ctx, p, &zero, rounded((double)mass), 0);
    if (!target_slot)
        return NULL;
    /* Original FILD -> FST mass (without popping) -> FMUL keeps the integer
     for X inertia. Y/Z reload the already stored binary32 mass. */
    target_slot->body.inertia.x = rounded((double)mass * 1.73);
    memcpy((char *)target_slot->body.entity + 0x78, &target_slot->body.inertia.x, 4);
    return finish(ctx, target_slot, ctx->resources[index], &zero, &zero, 0, NULL, 0, -1, 0, 0, &zero, NULL, NULL);
}
BHEntity *__cdecl bh_target_construct_moving(BHTargetConstructContext *ctx, const BHVec3 *p, const BHVec3 *a,
                                             const BHVec3 *v, const BHVec3 *w, unsigned long resource, float mass,
                                             float countdown, BHEntity *owner, long token, long loot, long counter)
{
    BHVec3 zero = {0, 0, 0};
    BHTargetConstructSlot *target_slot;
    if (!v || !w)
        return NULL;
    target_slot = begin(ctx, p, a, mass, 0);
    if (!target_slot)
        return NULL;
    return finish(ctx, target_slot, resource, v, w, countdown, owner, token, loot, counter, 0x2000, &zero, NULL, NULL);
}
static void local_direction(const BHMat34 *m, const BHVec3 *v, BHVec3 *out)
{
    out->x = rounded(((double)m->m[2][0] * v->z + (double)m->m[1][0] * v->y) + (double)m->m[0][0] * v->x);
    out->y = rounded(((double)m->m[1][1] * v->y + (double)m->m[2][1] * v->z) + (double)m->m[0][1] * v->x);
    out->z = rounded(((double)m->m[2][2] * v->z + (double)m->m[1][2] * v->y) + (double)m->m[0][2] * v->x);
}
static void local_point(const BHMat34 *m, const BHVec3 *v, BHVec3 *out)
{
    float x = rounded((double)v->x - m->m[0][3]);
    double y = (double)v->y - m->m[1][3], z = (double)v->z - m->m[2][3];
    out->x = rounded(((double)m->m[2][0] * z + (double)m->m[1][0] * y) + (double)m->m[0][0] * x);
    out->y = rounded(((double)m->m[2][1] * z + (double)m->m[1][1] * y) + (double)m->m[0][1] * x);
    out->z = rounded(((double)m->m[2][2] * z + (double)m->m[1][2] * y) + (double)m->m[0][2] * x);
}
BHEntity *__cdecl bh_target_construct_box(BHTargetConstructContext *ctx, const BHVec3 *p, const BHVec3 *a,
                                          const BHVec3 *v, const BHVec3 *w, unsigned long resource, float mass,
                                          float countdown, BHEntity *owner, long token, const BHVec3 *anchor,
                                          const BHVec3 *first, const BHVec3 *second, long counter)
{
    BHTargetConstructSlot *target_slot;
    BHVec3 an, lo, hi, extent, padding;
    unsigned long i;
    if (!v || !w || !anchor || !first || !second)
        return NULL;
    target_slot = begin(ctx, p, a, mass, 1);
    if (!target_slot)
        return NULL;
    local_direction(&target_slot->state.pose, anchor, &an);
    local_point(&target_slot->state.pose, first, &lo);
    local_point(&target_slot->state.pose, second, &hi);
    for (i = 0; i < 3; ++i)
    {
        ((float *)&extent)[i] = rounded(fabs(rounded((double)((float *)&hi)[i] - ((float *)&lo)[i])) * .8);
        ((float *)&padding)[i] = rounded((double)((float *)&extent)[i] * .5);
    }
    if (!bh_collision_box_build(&target_slot->collision, &extent, &padding))
    {
        {
            BHEntity *entity = target_slot->body.entity;
            bh_debris_unbind(&target_slot->body);
            bh_entity_release(ctx->debris->pool, entity);
        }
        target_slot->body.entity = NULL;
        ctx->error = 1;
        return NULL;
    }
    target_slot->body.entity->collision_radius = rounded(bh_geometry_radius(&target_slot->collision.bsp));
    return finish(ctx, target_slot, resource, v, w, countdown, owner, token, -1, counter, 0x800, &an, &lo, &hi);
}
