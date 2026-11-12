#include "bh_player_create.h"
#include "bh_geometry_radius.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
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
        entity->player.spawn_delay = delay;
        entity->player.vehicle = (unsigned char)vehicle;
        entity->player.model_resource = model;
        entity->player.propulsion.delayed.creation = creation;
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
    entity->player.input_list = ctx->input;
    if (!ctx->input)
        return failed(creation, 2);
    bh_player_input_setup(ctx->input, creation->profile);
    bh_player_input_bind_string(ctx->input, 29, "IAKFA");
    bh_player_input_bind_string(ctx->input, 30, "IADQD");
    bh_player_input_bind_string(ctx->input, 31, "IABAT");
    if (!bh_collision_model_load(creation->collision, vehicle ? "hun_lod2.BSP" : "hun_lod1.BSP", creation->prefix,
                                 creation->search))
        return failed(creation, 3);
    entity->collision_geometry = &creation->collision->geometry;
    entity->collision_radius = rf(bh_geometry_radius(&creation->collision->bsp));
    entity->world = 0;
    if (!bh_actor_basis_acquire(ctx->force->basis))
        return failed(creation, 3);
    entity->lifetime = 1000;
    entity->inertia.x = 1416.6666259765625f;
    entity->inertia.y = rf(1000.0 * 1.6666666269302368);
    entity->inertia.z = rf(1000.0 * .4166666567325592);
    entity->scale = .9f;
    entity->position = *position;
    memset(&matrix, 0, sizeof(matrix));
    for (i = 0; i < 3; ++i)
    {
        v = zero;
        ((float *)&v)[i] = 1;
        entity->basis[i] = v;
        matrix.m[i][i] = 1;
        matrix.m[i][3] = ((const float *)position)[i];
    }
    entity->last_clear = matrix;
    entity->euler = zero;
    entity->velocity = zero;
    entity->angular_velocity = zero;
    entity->acceleration = zero;
    entity->angular_acceleration = zero;
    entity->player.recoil_position = *position;
    entity->player.recoil_velocity = zero;
    ctx->state->death_frames = 0;
    if (vehicle)
    {
        entity->player.camera_mode = 1;
        entity->player.camera_countdown = 0;
        if (!creation->correction || !bh_camera_state_init(bh_player_camera_state(entity), 1) ||
            !bh_camera_state_bind_correction(ctx->camera->camera, bh_camera_correct_callback, creation->correction))
            return failed(creation, 3);
    }
    entity->player.disabled_frames = 0;
    entity->player.damage = 0;
    entity->player.engine_output = 0;
    memset(entity->player.propulsion.channels, 0, sizeof(entity->player.propulsion.channels));
    entity->player.engine_sound = bh_sound_request(ctx->sound, creation->engine_loop, 0, 1, &zero, &zero);
    entity->player.ground_sound = BH_SOUND_INVALID;
    entity->player.shield_sound = BH_SOUND_INVALID;
    entity->player.death_sound = BH_SOUND_INVALID;
    entity->player.damage_flash = 0;
    entity->player.instability = 1;
    entity->player.wear = 0;
    entity->player.frame_phase = 0;
    entity->player.cash_flash = 0;
    entity->player.fine_flash = 0;
    entity->player.cash = 10000;
    entity->player.fine = 0;
    entity->player.bounty_pending = 0;
    entity->player.vehicle = (unsigned char)vehicle;
    if (!bh_player_tick_bind(ctx, entity))
        return failed(creation, 3);
    entity->refresh = refresh;
    creation->query->player = entity;
    creation->query->contact_physics = ctx->body->physics;
    creation->query->weapons = &ctx->weapons->factory->registry;
    if (!bh_player_query_bind(creation->query))
        return failed(creation, 3);
    entity->player.model_resource = model;
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
        entity->player.weapons[i] = w;
        if (!w)
            return failed(creation, 2);
    }
    entity->player.selected_weapon[0] = 0;
    entity->player.selected_weapon[1] = 9;
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
    creation = entity->player.propulsion.delayed.creation;
    if (!creation || !creation->tick || !creation->tick->pool)
        return;
    delay = entity->player.spawn_delay;
    delay = rf((double)delay - 1.0 / 30);
    entity->player.spawn_delay = delay;
    if (delay <= 0)
    {
        position = entity->position;
        vehicle = entity->player.vehicle;
        model = entity->player.model_resource;
        bh_player_create(creation, 0, &position, vehicle, model);
        bh_entity_release(creation->tick->pool, entity);
        ++creation->expired;
    }
}
