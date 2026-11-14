#include "bh_projectile_lifetime.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
static_assert(sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 &&
                  offsetof(BHProjectileSyncPacket, position) == 16,
              "projectile layout");
static BHProjectileLifetime *context(BHEntity *entity)
{
    BHProjectileLifetime *c;
    memcpy(&c, &entity->bindings.projectile_lifetime, 4);
    return c;
}
static int live(BHProjectileLifetime *c, const BHEntity *entity, int bound)
{
    unsigned long g;
    if (!c || !c->pool || !c->sound || !entity)
        return 0;
    if (bound)
        memcpy(&g, &entity->bindings.lifetime_generation, 4);
    return bh_entity_is_active(c->pool, entity) && (!bound || entity->generation == g);
}
int bh_projectile_query_state(const BHEntity *entity, BHProjectileQueryState *state)
{
    if (!entity || !state)
        return 0;
    memcpy(&state->countdown, &entity->projectile.countdown, 4);
    memcpy(&state->sound, &entity->projectile.sound, 4);
    memcpy(&state->identity, &entity->projectile.identity, 4);
    memcpy(&state->target, &entity->projectile.target, 4);
    memcpy(&state->secondary, &entity->projectile.secondary, 4);
    return 1;
}
int bh_projectile_lifetime_bind(BHProjectileLifetime *c, BHEntity *entity, const BHProjectileQueryState *state)
{
    if (!state || !live(c, entity, 0))
        return 0;
    memcpy(&entity->projectile.countdown, &state->countdown, 4);
    memcpy(&entity->projectile.sound, &state->sound, 4);
    memcpy(&entity->projectile.identity, &state->identity, 4);
    memcpy(&entity->projectile.target, &state->target, 4);
    memcpy(&entity->projectile.secondary, &state->secondary, 4);
    memcpy(&entity->bindings.projectile_lifetime, &c, 4);
    memcpy(&entity->bindings.lifetime_generation, &entity->generation, 4);
    c->error = 0;
    return bh_entity_set_query(c->pool, entity, bh_projectile_query);
}
void bh_projectile_cleanup(BHEntity *entity)
{
    BHProjectileLifetime *c = context(entity);
    unsigned long handle;
    if (!live(c, entity, 1))
    {
        if (c)
            c->error = 1;
        return;
    }
    memcpy(&handle, &entity->projectile.sound, 4);
    bh_sound_stop(c->sound, handle);
    bh_entity_release(c->pool, entity);
}
void bh_projectile_query(BHEntity *entity, void *packet)
{
    BHProjectileLifetime *c = context(entity);
    BHProjectileQueryState state;
    long event;
    unsigned long identity, kind;
    BHEntity *removed;
    if (!packet || !live(c, entity, 1))
    {
        if (c)
            c->error = 1;
        return;
    }
    memcpy(&event, packet, 4);
    bh_projectile_query_state(entity, &state);
    if (event == 20)
    {
        entity->step = bh_projectile_cleanup;
        bh_entity_set_query(c->pool, entity, NULL);
        entity->refresh = NULL;
        return;
    }
    if (event == 17)
    {
        memcpy(&removed, (unsigned char *)packet + 4, 4);
        if (removed == state.target)
        {
            state.target = NULL;
            memcpy(&entity->projectile.target, &state.target, 4);
        }
        if (removed == state.secondary)
        {
            state.secondary = NULL;
            memcpy(&entity->projectile.secondary, &state.secondary, 4);
        }
        return;
    }
    if (event != 18)
        return;
    memcpy(&identity, (unsigned char *)packet + 4, 4);
    if (identity != state.identity)
        return;
    identity = 0xffffffffUL;
    memcpy((unsigned char *)packet + 4, &identity, 4);
    memcpy(&kind, (unsigned char *)packet + 8, 4);
    if (kind != 3)
        return;
    if (!_isnan(state.countdown) && (double)state.countdown > .2)
    {
        state.countdown = .2f;
        memcpy(&entity->projectile.countdown, &state.countdown, 4);
    }
    memcpy(&entity->position, (unsigned char *)packet + 16, 12);
    identity = 1;
    memcpy(&entity->projectile.identity, &identity, 4);
}
