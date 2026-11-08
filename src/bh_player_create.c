#include "bh_player_create.h"
#include "bh_geometry_radius.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static void word(BHEntity *entity, unsigned long o, unsigned long v)
{
    memcpy((char *)entity + o, &v, 4);
}
static void real(BHEntity *entity, unsigned long o, float v)
{
    memcpy((char *)entity + o, &v, 4);
}
static float rf(double v)
{
    float out;
    bh_round_float(&out, v);
    return out;
}
static BHEntity *failed(BHPlayerCreate *creation, int error)
{
    creation->error = error;
    return 0;
}
static void refresh(BHEntity *entity, void *packet)
{
    BHPlayerTick *ctx = bh_player_tick_context(entity);
    (void)packet;
    if (ctx && ctx->scene)
    {
        ctx->scene->player = entity;
        bh_player_scene_draw(ctx->scene);
    }
}
BHEntity *bh_player_create(BHPlayerCreate *creation, float delay, const BHVec3 *position, unsigned long vehicle,
                           unsigned long model)
{
    BHEntity *entity, *w;
    BHPlayerTick *ctx;
    BHPhysicsStepBody *body;
    BHWeaponObjectState *state;
    BHVec3 v;
    BHMat34 matrix;
    unsigned long i, n;
    unsigned short short_value;
    long ammo;
    if (!creation || !creation->tick || !creation->tick->pool || !position || !_finite(delay) ||
        !_finite(position->x) || !_finite(position->y) || !_finite(position->z))
        return creation ? failed(creation, 1) : 0;
    creation->error = 0;
    ctx = creation->tick;
    entity = bh_entity_acquire(ctx->pool);
    if (!entity)
        return 0;
    if (delay != 0)
    {
        entity->position = *position;
        real(entity, 0x23c, delay);
        *((unsigned char *)entity + 0x2bb) = (unsigned char)vehicle;
        word(entity, 0x21c, model);
        memcpy(entity->payload + 516, &creation, 4);
        entity->step = bh_player_create_delayed;
        ++creation->deferred;
        return entity;
    }
    if (!creation->current || !creation->input_owner || !creation->profile || !creation->collision ||
        !creation->query || !creation->actors || creation->actor_capacity < 2 || !ctx->force || !ctx->force->basis ||
        !ctx->body || !ctx->body->physics || !ctx->weapons || !ctx->weapons->factory || !ctx->state || !ctx->sound ||
        !ctx->camera || !ctx->camera->camera)
        return failed(creation, 1);
    *creation->current = entity;
    ctx->player = entity;
    ctx->input = bh_player_input_create(creation->input_owner, 0);
    word(entity, 0x188, (unsigned long)ctx->input);
    if (!ctx->input)
        return failed(creation, 2);
    bh_player_input_setup(ctx->input, creation->profile);
    bh_player_input_bind_string(ctx->input, 29, "IAKFA");
    bh_player_input_bind_string(ctx->input, 30, "IADQD");
    bh_player_input_bind_string(ctx->input, 31, "IABAT");
    if (!bh_collision_model_load(creation->collision, vehicle ? "hun_lod2.BSP" : "hun_lod1.BSP", creation->prefix,
                                 creation->search))
        return failed(creation, 3);
    word(entity, 0xbc, (unsigned long)&creation->collision->geometry);
    entity->collision_radius = rf(bh_geometry_radius(&creation->collision->bsp));
    word(entity, 0xc4, 0);
    if (!bh_actor_basis_acquire(ctx->force->basis))
        return failed(creation, 3);
    real(entity, 0xb4, 1000);
    word(entity, 0x78, 0x44b11555UL);
    real(entity, 0x7c, rf(1000.0 * 1.6666666269302368));
    real(entity, 0x80, rf(1000.0 * .4166666567325592));
    real(entity, 0xb8, .9f);
    entity->position = *position;
    memset(&matrix, 0, sizeof(matrix));
    for (i = 0; i < 3; ++i)
    {
        v = zero;
        ((float *)&v)[i] = 1;
        memcpy((char *)entity + 0x18 + i * 12, &v, 12);
        matrix.m[i][i] = 1;
        matrix.m[i][3] = ((const float *)position)[i];
    }
    memcpy((char *)entity + 0x84, &matrix, 48);
    for (i = 0; i < 5; ++i)
        memcpy((char *)entity + (i ? 0x48 + (i - 1) * 12 : 0x3c), &zero, 12);
    memcpy((char *)entity + 0x298, position, 12);
    memcpy((char *)entity + 0x2a4, &zero, 12);
    ctx->state->death_frames = 0;
    if (vehicle)
    {
        short_value = 1;
        memcpy((char *)entity + 0x2b0, &short_value, 2);
        short_value = 0;
        memcpy((char *)entity + 0x2b2, &short_value, 2);
        if (!creation->correction || !bh_camera_state_init(bh_player_camera_state(entity), 1) ||
            !bh_camera_state_bind_correction(ctx->camera->camera, bh_camera_correct_callback, creation->correction))
            return failed(creation, 3);
    }
    short_value = 0;
    memcpy((char *)entity + 0x2b4, &short_value, 2);
    real(entity, 0x23c, 0);
    real(entity, 0x2f8, 0);
    memset((char *)entity + 0x2bc, 0, 60);
    word(entity, 0x220, bh_sound_request(ctx->sound, creation->engine_loop, 0, 1, &zero, &zero));
    word(entity, 0x228, BH_SOUND_INVALID);
    word(entity, 0x22c, BH_SOUND_INVALID);
    word(entity, 0x224, BH_SOUND_INVALID);
    real(entity, 0x254, 0);
    real(entity, 0x250, 1);
    real(entity, 0x24c, 0);
    memset((char *)entity + 0x2b8, 0, 3);
    word(entity, 0x230, 10000);
    word(entity, 0x234, 0);
    word(entity, 0x238, 0);
    *((unsigned char *)entity + 0x2bb) = (unsigned char)vehicle;
    if (!bh_player_tick_bind(ctx, entity))
        return failed(creation, 3);
    entity->refresh = refresh;
    creation->query->player = entity;
    creation->query->contact_physics = ctx->body->physics;
    creation->query->weapons = &ctx->weapons->factory->registry;
    if (!bh_player_query_bind(creation->query))
        return failed(creation, 3);
    word(entity, 0x21c, model);
    for (n = 0; n < creation->actor_capacity && creation->actors[n]; ++n)
    {
    }
    if (n + 1 >= creation->actor_capacity)
        return failed(creation, 4);
    creation->actors[n] = entity;
    creation->actors[n + 1] = 0;
    ctx->weapons->player = entity;
    for (i = 0; i < 10; ++i)
    {
        w = bh_weapon_factory_create(ctx->weapons->factory, i);
        word(entity, 0x258 + i * 4, (unsigned long)w);
        if (!w)
            return failed(creation, 2);
    }
    *((unsigned char *)entity + 0x2b6) = 0;
    *((unsigned char *)entity + 0x2b7) = 9;
    for (i = 0; i < 2; ++i)
    {
        if (!bh_weapon_registry_resolve(&ctx->weapons->factory->registry, i ? 9 : 0, &w, &state))
            return failed(creation, 3);
        state->common.available_ec = 1;
        ammo = state->common.ammo + (i ? 1 : 300);
        state->common.ammo = ammo > state->common.ammo_limit ? state->common.ammo_limit : ammo;
    }
    body = ctx->body;
    body->entity = entity;
    body->generation = entity->generation;
    body->geometry = &creation->collision->geometry;
    body->mass = 1000;
    body->friction = .9f;
    body->world = 0;
    if (!bh_physics_step_read_entity(body->physics, entity))
        return failed(creation, 3);
    ctx->force->body = body;
    ctx->camera->player = entity;
    if (ctx->scene)
        ctx->scene->player = entity;
    ++creation->created;
    return entity;
}
void bh_player_create_delayed(BHEntity *entity)
{
    BHPlayerCreate *creation;
    float delay;
    unsigned long model;
    BHVec3 position;
    unsigned char vehicle;
    if (!entity)
        return;
    memcpy(&creation, entity->payload + 516, 4);
    if (!creation || !creation->tick || !creation->tick->pool)
        return;
    memcpy(&delay, (char *)entity + 0x23c, 4);
    delay = rf((double)delay - 1.0 / 30);
    real(entity, 0x23c, delay);
    if (delay <= 0)
    {
        position = entity->position;
        vehicle = *((unsigned char *)entity + 0x2bb);
        memcpy(&model, (char *)entity + 0x21c, 4);
        bh_player_create(creation, 0, &position, vehicle, model);
        bh_entity_release(creation->tick->pool, entity);
        ++creation->expired;
    }
}
