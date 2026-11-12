#include "bh_physics_step.h"
#include "bh_fp.h"
#include "bh_round.h"
#include "bh_rotation.h"
#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
typedef char physics_common_layout[sizeof(void *) == 4 && offsetof(BHEntity, position) == 12 &&
                                           offsetof(BHEntity, origin) == 0x48 && offsetof(BHEntity, lifetime) == 0xb4
                                       ? 1
                                       : -1];
static void *allocate(unsigned long n, void *u)
{
    (void)u;
    return malloc(n);
}
static void release(void *p, void *u)
{
    (void)u;
    free(p);
}
static int fail(BHPhysicsStepContext *ctx, BHPhysicsStepError error)
{
    if (ctx)
        ctx->error = error;
    return 0;
}
static int live(BHEntityPool *pool, const BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(pool, entity) && entity->generation == generation;
}
static BHPhysicsStepBody *body(BHPhysicsStepContext *ctx, const BHEntity *entity)
{
    long i = bh_binding_find(&ctx->body_index, entity, NULL);
    return i < 0 ? NULL : ctx->bodies + i;
}
static const BHPhysicsStepWorld *world(BHPhysicsStepContext *ctx, const BHWorldGeometry *world)
{
    long i = bh_binding_find(&ctx->world_index, world, NULL);
    return i < 0 ? NULL : ctx->worlds + i;
}
static BHPhysicsStepPair *pair(BHPhysicsStepContext *ctx, BHGeometry *a, BHGeometry *b)
{
    long i = bh_binding_find(&ctx->pair_index, a, b);
    return i < 0 ? NULL : ctx->pairs + i;
}
int bh_physics_step_read_entity(BHPhysicsState *physics, const BHEntity *entity)
{
    unsigned long r, k;
    float basis[9];
    if (!physics || !entity)
        return 0;
    memcpy(basis, entity->basis, sizeof(basis));
    for (r = 0; r < 3; ++r)
    {
        for (k = 0; k < 3; ++k)
            physics->pose.m[r][k] = basis[k * 3 + r];
        physics->pose.m[r][3] = ((const float *)&entity->position)[r];
    }
    physics->last_clear = entity->last_clear;
    physics->velocity = entity->velocity;
    physics->angular_velocity = entity->angular_velocity;
    physics->acceleration = entity->acceleration;
    physics->angular_acceleration = entity->angular_acceleration;
    physics->euler = entity->euler;
    return 1;
}
int bh_physics_step_publish(BHPhysicsStepBody *body)
{
    unsigned long r, k;
    float basis[9];
    BHPhysicsState *physics;
    BHEntity *entity;
    if (!body || !(physics = body->physics) || !(entity = body->entity))
        return 0;
    for (r = 0; r < 3; ++r)
    {
        for (k = 0; k < 3; ++k)
            basis[k * 3 + r] = physics->pose.m[r][k];
        ((float *)&entity->position)[r] = physics->pose.m[r][3];
    }
    memcpy(entity->basis, basis, sizeof(basis));
    entity->last_clear = physics->last_clear;
    entity->velocity = physics->velocity;
    entity->angular_velocity = physics->angular_velocity;
    entity->acceleration = physics->acceleration;
    entity->angular_acceleration = physics->angular_acceleration;
    entity->euler = physics->euler;
    entity->world = body->world;
    entity->lifetime = body->mass;
    entity->scale = body->friction;
    return 1;
}
int bh_physics_step_dispose(BHPhysicsStepContext *ctx)
{
    unsigned long i;
    BHSceneAllocator a;
    if (!ctx || ctx->busy || ctx->broad.busy)
        return 0;
    a = ctx->allocator;
    if (!a.release)
        return 0;
    for (i = 0; i < ctx->pair_count; ++i)
        if (ctx->pairs[i].sweep.busy || ctx->pairs[i].sweep.collision.busy || ctx->pairs[i].sweep.segment.busy)
            return 0;
    bh_binding_dispose(&ctx->body_index, &a);
    bh_binding_dispose(&ctx->world_index, &a);
    bh_binding_dispose(&ctx->pair_index, &a);
    for (i = 0; i < ctx->pair_count; ++i)
        if (!bh_sweep_dispose(&ctx->pairs[i].sweep))
            return 0;
    bh_world_query_dispose(&ctx->broad);
    if (ctx->pairs)
        a.release(ctx->pairs, a.user);
    if (ctx->actors)
        a.release(ctx->actors, a.user);
    if (ctx->candidates)
        a.release(ctx->candidates, a.user);
    memset(ctx, 0, sizeof(*ctx));
    return 1;
}
static unsigned long unique(BHGeometry **list, unsigned long count, BHGeometry *g)
{
    unsigned long i;
    for (i = 0; i < count; ++i)
        if (list[i] == g)
            return count;
    list[count++] = g;
    return count;
}
int bh_physics_step_init(BHPhysicsStepContext *ctx, BHEntityPool *pool, BHPhysicsStepBody *body, unsigned long nb,
                         const BHPhysicsStepWorld *w, unsigned long nw, BHGeometry *const *models, unsigned long nm,
                         BHPhysicsEffects *effects, int fallback, const BHSceneAllocator *a)
{
    BHPhysicsStepContext next;
    BHGeometry **sources = NULL, **targets = NULL;
    unsigned long ns = 0, nt = 0, i, j, count;
    BHSceneAllocator mem;
    if (!ctx || !pool || !body || !nb || nb > 65535 || nw > 65535 || nb + nw > 65535 || (nw && !w) || !models || !nm ||
        nm > 65535 || !effects || (a && (!a->allocate || !a->release)) || !bh_entities_valid(pool))
        return 0;
    if (a)
        mem = *a;
    else
    {
        mem.allocate = allocate;
        mem.release = release;
        mem.user = NULL;
    }
    memset(&next, 0, sizeof(next));
    next.allocator = mem;
    next.pool = pool;
    next.validated_retirements = pool->retirements;
    next.bodies = body;
    next.body_count = nb;
    next.worlds = w;
    next.world_count = nw;
    next.models = models;
    next.model_count = nm;
    next.effects = effects;
    next.fallback = fallback;
    sources = (BHGeometry **)mem.allocate(nb * 4, mem.user);
    targets = (BHGeometry **)mem.allocate((nb + nw) * 4, mem.user);
    next.actors = (BHEntity **)mem.allocate(nb * 4, mem.user);
    if (!sources || !targets || !next.actors || !bh_binding_init(&next.body_index, nb, &mem) ||
        !bh_binding_init(&next.world_index, nw, &mem))
        goto rejected;
    for (i = 0; i < nb; ++i)
    {
        if (!body[i].entity || !body[i].physics || !body[i].target_world || !body[i].target_local ||
            body[i].model_index >= nm || !_finite(body[i].mass) || !_finite(body[i].friction) ||
            !live(pool, body[i].entity, body[i].generation))
            goto rejected;
        if (!bh_binding_insert(&next.body_index, body[i].entity, NULL, i))
            goto rejected;
        next.actors[i] = body[i].entity;
        ns = unique(sources, ns, body[i].geometry);
        nt = unique(targets, nt, body[i].geometry);
    }
    for (i = 0; i < nw; ++i)
    {
        if (!w[i].world || !_finite(w[i].mass))
            goto rejected;
        for (j = 0; j < i; ++j)
            if (w[j].world == w[i].world || w[j].index == w[i].index)
                goto rejected;
        if (!bh_binding_insert(&next.world_index, w[i].world, NULL, i))
            goto rejected;
        nt = unique(targets, nt, w[i].world->model ? w[i].world->model->geometry : NULL);
    }
    if (nt && ns > 0x7fffffffUL / nt / sizeof(BHPhysicsStepPair))
        goto rejected;
    count = ns * nt;
    next.pairs = (BHPhysicsStepPair *)mem.allocate(count * sizeof(*next.pairs), mem.user);
    if (!next.pairs || !bh_binding_init(&next.pair_index, count, &mem))
        goto rejected;
    memset(next.pairs, 0, count * sizeof(*next.pairs));
    for (i = 0; i < ns; ++i)
        for (j = 0; j < nt; ++j)
        {
            BHPhysicsStepPair *p = next.pairs + next.pair_count;
            p->source = sources[i];
            p->target = targets[j];
            if (!bh_sweep_init(&p->sweep, p->source, p->target))
                goto rejected;
            bh_binding_insert(&next.pair_index, p->source, p->target, next.pair_count);
            ++next.pair_count;
        }
    if (!bh_world_query_init(&next.broad, nw ? w[0].world : NULL, nw, next.actors, nb, &mem))
        goto rejected;
    next.candidate_capacity = next.broad.capacity;
    next.candidates =
        (BHCollisionCandidate *)mem.allocate(next.candidate_capacity * sizeof(*next.candidates), mem.user);
    if (!next.candidates)
        goto rejected;
    for (i = 0; i < nb; ++i)
        bh_physics_step_publish(body + i);
    mem.release(sources, mem.user);
    mem.release(targets, mem.user);
    *ctx = next;
    return 1;
rejected:
    if (sources)
        mem.release(sources, mem.user);
    if (targets)
        mem.release(targets, mem.user);
    bh_physics_step_dispose(&next);
    return 0;
}
int bh_physics_step_rebind(BHPhysicsStepContext *ctx, BHPhysicsStepBody *body, unsigned long nb,
                           BHGeometry *const *models, unsigned long nm)
{
    BHPhysicsStepContext next;
    BHGeometry **sources = NULL, **targets = NULL;
    long *borrowed = NULL;
    unsigned long ns = 0, nt = 0, i, j, count, prepared = 0;
    BHPhysicsStepPair *old;
    BHSceneAllocator mem;
    if (!ctx || ctx->busy || ctx->broad.busy || !ctx->allocator.release || !body || !nb || nb > 65535 ||
        nb + ctx->world_count > 65535 || !models || !nm || nm > 65535)
        return 0;
    for (i = 0; i < ctx->pair_count; ++i)
        if (ctx->pairs[i].sweep.busy || ctx->pairs[i].sweep.collision.busy || ctx->pairs[i].sweep.segment.busy)
            return 0;
    mem = ctx->allocator;
    next = *ctx;
    memset(&next.body_index, 0, sizeof(next.body_index));
    memset(&next.pair_index, 0, sizeof(next.pair_index));
    next.validated_retirements = ctx->pool->retirements;
    next.bodies = body;
    next.body_count = nb;
    next.models = models;
    next.model_count = nm;
    next.actors = NULL;
    next.pairs = NULL;
    next.pair_count = 0;
    next.candidates = NULL;
    next.candidate_count = 0;
    next.error = BH_PHYSICS_STEP_OK;
    next.broad.hits = NULL;
    next.broad.work = NULL;
    next.broad.hit_count = 0;
    next.broad.actor_count = nb;
    next.broad.capacity = ctx->world_count + nb + 1;
    next.candidate_capacity = next.broad.capacity;
    sources = (BHGeometry **)mem.allocate(nb * sizeof(*sources), mem.user);
    targets = (BHGeometry **)mem.allocate((nb + ctx->world_count) * sizeof(*targets), mem.user);
    next.actors = (BHEntity **)mem.allocate(nb * sizeof(*next.actors), mem.user);
    if (!sources || !targets || !next.actors || !bh_binding_init(&next.body_index, nb, &mem))
        goto rejected_rebind;
    next.broad.actors = next.actors;
    for (i = 0; i < nb; ++i)
    {
        if (!body[i].entity || !body[i].physics || !body[i].target_world || !body[i].target_local ||
            body[i].model_index >= nm || !_finite(body[i].mass) || !_finite(body[i].friction) ||
            !live(ctx->pool, body[i].entity, body[i].generation))
            goto rejected_rebind;
        if (!bh_binding_insert(&next.body_index, body[i].entity, NULL, i))
            goto rejected_rebind;
        next.actors[i] = body[i].entity;
        ns = unique(sources, ns, body[i].geometry);
        nt = unique(targets, nt, body[i].geometry);
    }
    /* The same immutable scene was validated by init. Only roster membership
       changes here; do not repeat its quadratic world-ID validation. */
    for (i = 0; i < ctx->world_count; ++i)
        nt = unique(targets, nt, ctx->worlds[i].world->model ? ctx->worlds[i].world->model->geometry : NULL);
    if (nt && ns > 0x7fffffffUL / nt / sizeof(BHPhysicsStepPair))
        goto rejected_rebind;
    count = ns * nt;
    next.pairs = (BHPhysicsStepPair *)mem.allocate(count * sizeof(*next.pairs), mem.user);
    borrowed = (long *)mem.allocate(count * sizeof(*borrowed), mem.user);
    if (!next.pairs || !borrowed || !bh_binding_init(&next.pair_index, count, &mem))
        goto rejected_rebind;
    memset(next.pairs, 0, count * sizeof(*next.pairs));
    for (i = 0; i < ns; ++i)
        for (j = 0; j < nt; ++j)
        {
            BHPhysicsStepPair *p = next.pairs + prepared;
            p->source = sources[i];
            p->target = targets[j];
            borrowed[prepared] = -1;
            old = pair(ctx, p->source, p->target);
            if (old)
            {
                p->sweep = old->sweep;
                borrowed[prepared] = (long)(old - ctx->pairs);
            }
            else if (!bh_sweep_init(&p->sweep, p->source, p->target))
                goto rejected_rebind;
            bh_binding_insert(&next.pair_index, p->source, p->target, prepared);
            ++prepared;
            next.pair_count = prepared;
        }
    next.broad.hits = (BHWorldHit *)mem.allocate(next.broad.capacity * sizeof(BHWorldHit), mem.user);
    next.broad.work = (BHWorldHit *)mem.allocate(next.broad.capacity * sizeof(BHWorldHit), mem.user);
    next.candidates =
        (BHCollisionCandidate *)mem.allocate(next.candidate_capacity * sizeof(*next.candidates), mem.user);
    if (!next.broad.hits || !next.broad.work || !next.candidates)
        goto rejected_rebind;
    next.broad.hits[0].kind = BH_WORLD_HIT_END;
    next.broad.hits[0].object.geometry = NULL;
    next.broad.error = BH_QUERY_OK;
    /* Commit transfers only after every allocation succeeded. No failure path
       may free an old sweep or the borrowed static scene workspace. */
    for (i = 0; i < prepared; ++i)
        if (borrowed[i] >= 0)
            memset(&ctx->pairs[borrowed[i]].sweep, 0, sizeof(BHSweepQuery));
    ctx->broad.members = ctx->broad.visited = ctx->broad.frames = NULL;
    memset(&ctx->broad.member_index, 0, sizeof(ctx->broad.member_index));
    memset(&ctx->world_index, 0, sizeof(ctx->world_index));
    bh_physics_step_dispose(ctx);
    *ctx = next;
    for (i = 0; i < nb; ++i)
        bh_physics_step_publish(body + i);
    mem.release(sources, mem.user);
    mem.release(targets, mem.user);
    mem.release(borrowed, mem.user);
    return 1;
rejected_rebind:
    if (borrowed)
        for (i = 0; i < prepared; ++i)
            if (borrowed[i] >= 0)
                memset(&next.pairs[i].sweep, 0, sizeof(BHSweepQuery));
    next.broad.members = next.broad.visited = next.broad.frames = NULL;
    memset(&next.broad.member_index, 0, sizeof(next.broad.member_index));
    memset(&next.world_index, 0, sizeof(next.world_index));
    bh_physics_step_dispose(&next);
    if (sources)
        mem.release(sources, mem.user);
    if (targets)
        mem.release(targets, mem.user);
    if (borrowed)
        mem.release(borrowed, mem.user);
    return 0;
}
static int candidates(BHPhysicsStepContext *ctx, BHPhysicsStepBody *self)
{
    unsigned long i;
    BHPhysicsStepPair *p;
    if (ctx->broad.hit_count > ctx->candidate_capacity)
        return fail(ctx, BH_PHYSICS_STEP_BINDING);
    for (i = 0; i < ctx->broad.hit_count; ++i)
    {
        BHWorldHit *hit = ctx->broad.hits + i;
        if (hit->kind == BH_WORLD_HIT_GEOMETRY)
        {
            BHWorldGeometry *w = hit->object.geometry;
            const BHPhysicsStepWorld *binding = world(ctx, w);
            p = pair(ctx, self->geometry, w->model ? w->model->geometry : NULL);
            if (!binding || !p || !bh_collision_candidate_world(ctx->candidates + i, w, &p->sweep))
                return fail(ctx, BH_PHYSICS_STEP_BINDING);
            if (binding->response)
                ctx->candidates[i].movement = binding->response->origin;
        }
        else if (hit->kind == BH_WORLD_HIT_ACTOR)
        {
            BHPhysicsStepBody *b = body(ctx, hit->object.actor);
            BHMat34 inverse;
            if (!b || !live(ctx->pool, b->entity, b->generation) || b->model_index >= ctx->model_count)
                return fail(ctx, BH_PHYSICS_STEP_BINDING);
            p = pair(ctx, self->geometry, b->geometry);
            memcpy(&b->flags, (const char *)b->entity + 0x114, 4);
            bh_mat34_inverse_rigid(&b->physics->pose, &inverse);
            if (!p || !bh_collision_candidate_actor(ctx->candidates + i, b->entity, &p->sweep, &b->physics->pose,
                                                    &inverse, &b->physics->velocity, b->flags))
                return fail(ctx, BH_PHYSICS_STEP_BINDING);
        }
        else
            return fail(ctx, BH_PHYSICS_STEP_BINDING);
    }
    ctx->candidate_count = ctx->broad.hit_count;
    return 1;
}
static int selected(BHPhysicsStepContext *ctx, float *mass, BHEntity **recipient)
{
    BHWorldHit *h = &ctx->candidates[0].object;
    ctx->selected = *h;
    if (h->kind == BH_WORLD_HIT_ACTOR)
    {
        BHPhysicsStepBody *b = body(ctx, h->object.actor);
        if (!b)
            return 0;
        *mass = b->mass;
        *recipient = b->entity;
    }
    else
    {
        const BHPhysicsStepWorld *w = world(ctx, h->object.geometry);
        if (!w)
            return 0;
        *mass = w->mass;
        *recipient = w->response;
    }
    return 1;
}
int bh_physics_step(BHPhysicsStepContext *ctx, BHEntity *entity, float dt)
{
    BHPhysicsStepBody *b;
    BHPhysicsState *physics;
    BHMat34 previous, pose;
    BHVec3 movement, angles, slide;
    BHPhysicsResponse response;
    BHEntity *recipient = NULL;
    const BHPhysicsStepWorld *current;
    unsigned long i, g, recipient_generation = 0;
    float margin, second_mass;
    int hit, result, saved_fallback, complete = 0;
    if (!ctx || !entity || !ctx->pool || !ctx->effects || !_finite(dt) || dt <= 0 || !bh_math_environment_supported())
        return fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
    if (ctx->busy)
        return fail(ctx, BH_PHYSICS_STEP_BUSY);
    if (ctx->zero.x != 0 || ctx->zero.y != 0 || ctx->zero.z != 0)
        return fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
    b = body(ctx, entity);
    if (!b || !live(ctx->pool, entity, b->generation))
        return fail(ctx, BH_PHYSICS_STEP_BINDING);
    if (ctx->validated_retirements != ctx->pool->retirements)
    {
        for (i = 0; i < ctx->body_count; ++i)
            if (!live(ctx->pool, ctx->bodies[i].entity, ctx->bodies[i].generation))
                return fail(ctx, BH_PHYSICS_STEP_BINDING);
        ctx->validated_retirements = ctx->pool->retirements;
    }
    /* Every step publishes its own state. Other bodies were published by their
       constructor, preceding step or explicit external-state boundary. */
    bh_physics_step_publish(b);
    ctx->busy = 1;
    ctx->error = BH_PHYSICS_STEP_OK;
    physics = b->physics;
    g = entity->generation;
    previous = physics->pose;
    pose = previous;
    if (!bh_physics_prepare(physics, dt, entity->collision_radius, &movement, &angles, &margin))
    {
        fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
        goto done;
    }
    physics->acceleration = ctx->zero;
    physics->angular_acceleration = ctx->zero;
    bh_physics_step_publish(b);
    if (!bh_world_query_run(&ctx->broad, entity, b->world, &entity->position, margin, &b->world) || !candidates(ctx, b))
    {
        if (ctx->error == BH_PHYSICS_STEP_OK)
            fail(ctx, BH_PHYSICS_STEP_COLLISION);
        goto done;
    }
    entity->world = b->world;
    memset(&ctx->first, 0, sizeof(ctx->first));
    hit = bh_entity_collision_dispatch(ctx->candidates, ctx->candidate_count, &pose, &movement, &angles, ctx->fallback,
                                       &ctx->first);
    if (hit < 0)
    {
        fail(ctx, BH_PHYSICS_STEP_COLLISION);
        goto done;
    }
    ++ctx->steps;
    if (!hit)
    {
        ++ctx->misses;
        physics->pose = pose;
        if (!bh_physics_finish(physics, &previous, dt, 1))
        {
            fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
            goto done;
        }
        bh_physics_step_publish(b);
        complete = 1;
        goto done;
    }
    if (!ctx->candidate_count || !selected(ctx, &second_mass, &recipient))
    {
        fail(ctx, BH_PHYSICS_STEP_BINDING);
        goto done;
    }
    ctx->first_selected = ctx->selected;
    /* Original00412788 tests selected entity+8 and skips this branch when
       kind==0 (physical actors). Only world/prop geometry uses last-clear
       recovery, with its world transform and model-catalog geometry. */
    if (ctx->first.fraction == 0 && ctx->selected.kind == BH_WORLD_HIT_GEOMETRY)
    {
        BHCollisionCandidate *target = ctx->candidates;
        BHCollisionHit overlap;
        memset(&overlap, 0, sizeof(overlap));
        result = bh_physics_overlap(target->sweep, &previous, &target->world, &target->local, &overlap);
        if (result < 0)
        {
            fail(ctx, BH_PHYSICS_STEP_COLLISION);
            goto done;
        }
        if (result)
        {
            ++ctx->rollbacks;
            physics->pose = physics->last_clear;
            if (!bh_physics_finish(physics, &previous, dt, 0))
            {
                fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
                goto done;
            }
            bh_physics_step_publish(b);
            complete = 1;
            goto done;
        }
    }
    if (!ctx->first.surface_present)
    {
        fail(ctx, BH_PHYSICS_STEP_CONTACT);
        goto done;
    }
    if (!bh_physics_slide(&movement, &ctx->first, b->friction, &slide))
    {
        fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
        goto done;
    }
    saved_fallback = ctx->fallback;
    ctx->fallback = 0;
    memset(&ctx->slide, 0, sizeof(ctx->slide));
    ctx->slide.fraction = 1;
    angles = ctx->zero;
    result = bh_entity_collision_dispatch(ctx->candidates, ctx->candidate_count, &pose, &slide, &angles, ctx->fallback,
                                          &ctx->slide);
    ctx->fallback = saved_fallback;
    if (result < 0)
    {
        fail(ctx, BH_PHYSICS_STEP_COLLISION);
        goto done;
    }
    if (!selected(ctx, &second_mass, &recipient))
    {
        fail(ctx, BH_PHYSICS_STEP_BINDING);
        goto done;
    }
    if (recipient)
    {
        BHPhysicsStepBody *other = body(ctx, recipient);
        if (other)
            recipient_generation = other->generation;
        else
            recipient_generation = recipient->generation;
        if (!live(ctx->pool, recipient, recipient_generation))
        {
            fail(ctx, BH_PHYSICS_STEP_BINDING);
            goto done;
        }
    }
    if (!bh_physics_contact_response(&ctx->first, ctx->slide.fraction, dt, b->mass, second_mass, &physics->acceleration,
                                     &response))
    {
        fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
        goto done;
    }
    bh_physics_step_publish(b);
    current = world(ctx, b->world);
    if (!bh_physics_effects_run(ctx->effects, &ctx->first, response.audio_level, b->world,
                                (unsigned short)(current ? current->index : 0)))
        ctx->error = BH_PHYSICS_STEP_EFFECT;
    ++ctx->contacts;
    if (response.emit_first && entity->query)
    {
        ++ctx->first_events;
        entity->query(entity, &response.first);
        if (!live(ctx->pool, entity, g))
        {
            fail(ctx, BH_PHYSICS_STEP_MUTATION);
            goto done;
        }
    }
    if (response.emit_second && recipient)
    {
        if (!live(ctx->pool, recipient, recipient_generation))
        {
            fail(ctx, BH_PHYSICS_STEP_MUTATION);
            goto done;
        }
        if (recipient->query)
        {
            ++ctx->second_events;
            recipient->query(recipient, &response.second);
        }
        if (!live(ctx->pool, entity, g))
        {
            fail(ctx, BH_PHYSICS_STEP_MUTATION);
            goto done;
        }
    }
    physics->pose = pose;
    if (!bh_physics_finish(physics, &previous, dt, 0))
    {
        fail(ctx, BH_PHYSICS_STEP_ARGUMENT);
        goto done;
    }
    bh_physics_step_publish(b);
    complete = 1;
done:
    ctx->busy = 0;
    return complete;
}
