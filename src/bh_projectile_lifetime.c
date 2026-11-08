#include "bh_projectile_lifetime.h"
#include <float.h>
#include <stddef.h>
#include <string.h>
typedef char projectile_layout[sizeof(void *) == 4 && offsetof(BHEntity, payload) == 0xe4 &&
                                       offsetof(BHProjectileSyncPacket, position) == 16
                                   ? 1
                                   : -1];
static void get(const BHEntity *entity, unsigned long offset, void *out)
{
    memcpy(out, (const unsigned char *)entity + offset, 4);
}
static void put(BHEntity *entity, unsigned long offset, const void *value)
{
    memcpy((unsigned char *)entity + offset, value, 4);
}
static BHProjectileLifetime *context(BHEntity *entity)
{
    BHProjectileLifetime *c;
    memcpy(&c, entity->payload + 468, 4);
    return c;
}
static int live(BHProjectileLifetime *c, const BHEntity *entity, int bound)
{
    unsigned long g;
    if (!c || !c->pool || !c->sound || !entity)
        return 0;
    if (bound)
        memcpy(&g, entity->payload + 472, 4);
    return bh_entity_is_active(c->pool, entity) && (!bound || entity->generation == g);
}
int bh_projectile_query_state(const BHEntity *entity, BHProjectileQueryState *state)
{
    if (!entity || !state)
        return 0;
    get(entity, 0xe8, &state->countdown);
    get(entity, 0x118, &state->sound);
    get(entity, 0x11c, &state->identity);
    get(entity, 0x12c, &state->target);
    get(entity, 0x130, &state->secondary);
    return 1;
}
int bh_projectile_lifetime_bind(BHProjectileLifetime *c, BHEntity *entity, const BHProjectileQueryState *state)
{
    if (!state || !live(c, entity, 0))
        return 0;
    put(entity, 0xe8, &state->countdown);
    put(entity, 0x118, &state->sound);
    put(entity, 0x11c, &state->identity);
    put(entity, 0x12c, &state->target);
    put(entity, 0x130, &state->secondary);
    memcpy(entity->payload + 468, &c, 4);
    memcpy(entity->payload + 472, &entity->generation, 4);
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
    get(entity, 0x118, &handle);
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
            put(entity, 0x12c, &state.target);
        }
        if (removed == state.secondary)
        {
            state.secondary = NULL;
            put(entity, 0x130, &state.secondary);
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
        put(entity, 0xe8, &state.countdown);
    }
    memcpy(&entity->position, (unsigned char *)packet + 16, 12);
    identity = 1;
    put(entity, 0x11c, &identity);
}
