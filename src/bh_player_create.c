#include "bh_player_create.h"
#include "bh_geometry_radius.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static const BHVec3 zero = {0, 0, 0};
static void word(BHEntity *e, unsigned long o, unsigned long v)
{
    memcpy((char *)e + o, &v, 4);
}
static void real(BHEntity *e, unsigned long o, float v)
{
    memcpy((char *)e + o, &v, 4);
}
static float rf(double v)
{
    float out;
    bh_round_float(&out, v);
    return out;
}
static BHEntity *failed(BHPlayerCreate *c, int error)
{
    c->error = error;
    return 0;
}
static void __cdecl refresh(BHEntity *e, void *packet)
{
    BHPlayerTick *c = bh_player_tick_context(e);
    (void)packet;
    if (c && c->scene)
    {
        c->scene->player = e;
        bh_player_scene_draw(c->scene);
    }
}
BHEntity *__cdecl bh_player_create(BHPlayerCreate *c, float delay, const BHVec3 *position, unsigned long vehicle,
                                   unsigned long model)
{
    BHEntity *e, *w;
    BHPlayerTick *t;
    BHPhysicsStepBody *body;
    BHWeaponObjectState *state;
    BHVec3 v;
    BHMat34 matrix;
    unsigned long i, n;
    unsigned short short_value;
    long ammo;
    if (!c || !c->tick || !c->tick->pool || !position || !_finite(delay) || !_finite(position->x) ||
        !_finite(position->y) || !_finite(position->z))
        return c ? failed(c, 1) : 0;
    c->error = 0;
    t = c->tick;
    e = bh_entity_acquire(t->pool);
    if (!e)
        return 0;
    if (delay != 0)
    {
        e->position = *position;
        real(e, 0x23c, delay);
        *((unsigned char *)e + 0x2bb) = (unsigned char)vehicle;
        word(e, 0x21c, model);
        memcpy(e->payload + 516, &c, 4);
        e->step = bh_player_create_delayed;
        ++c->deferred;
        return e;
    }
    if (!c->current || !c->input_owner || !c->profile || !c->collision || !c->query || !c->actors ||
        c->actor_capacity < 2 || !t->force || !t->force->basis || !t->body || !t->body->physics || !t->weapons ||
        !t->weapons->factory || !t->state || !t->sound || !t->camera || !t->camera->camera)
        return failed(c, 1);
    *c->current = e;
    t->player = e;
    t->input = bh_player_input_create(c->input_owner, 0);
    word(e, 0x188, (unsigned long)t->input);
    if (!t->input)
        return failed(c, 2);
    bh_player_input_setup(t->input, c->profile);
    bh_player_input_bind_string(t->input, 29, "IAKFA");
    bh_player_input_bind_string(t->input, 30, "IADQD");
    bh_player_input_bind_string(t->input, 31, "IABAT");
    if (!bh_collision_model_load(c->collision, vehicle ? "hun_lod2.BSP" : "hun_lod1.BSP", c->prefix, c->search))
        return failed(c, 3);
    word(e, 0xbc, (unsigned long)&c->collision->geometry);
    e->collision_radius = rf(bh_geometry_radius(&c->collision->bsp));
    word(e, 0xc4, 0);
    if (!bh_actor_basis_acquire(t->force->basis))
        return failed(c, 3);
    real(e, 0xb4, 1000);
    word(e, 0x78, 0x44b11555UL);
    real(e, 0x7c, rf(1000.0 * 1.6666666269302368));
    real(e, 0x80, rf(1000.0 * .4166666567325592));
    real(e, 0xb8, .9f);
    e->position = *position;
    memset(&matrix, 0, sizeof(matrix));
    for (i = 0; i < 3; ++i)
    {
        v = zero;
        ((float *)&v)[i] = 1;
        memcpy((char *)e + 0x18 + i * 12, &v, 12);
        matrix.m[i][i] = 1;
        matrix.m[i][3] = ((const float *)position)[i];
    }
    memcpy((char *)e + 0x84, &matrix, 48);
    for (i = 0; i < 5; ++i)
        memcpy((char *)e + (i ? 0x48 + (i - 1) * 12 : 0x3c), &zero, 12);
    memcpy((char *)e + 0x298, position, 12);
    memcpy((char *)e + 0x2a4, &zero, 12);
    t->state->death_frames = 0;
    if (vehicle)
    {
        short_value = 1;
        memcpy((char *)e + 0x2b0, &short_value, 2);
        short_value = 0;
        memcpy((char *)e + 0x2b2, &short_value, 2);
        if (!c->correction || !bh_camera_state_init(bh_player_camera_state(e), 1) ||
            !bh_camera_state_bind_correction(t->camera->camera, bh_camera_correct_callback, c->correction))
            return failed(c, 3);
    }
    short_value = 0;
    memcpy((char *)e + 0x2b4, &short_value, 2);
    real(e, 0x23c, 0);
    real(e, 0x2f8, 0);
    memset((char *)e + 0x2bc, 0, 60);
    word(e, 0x220, bh_sound_request(t->sound, c->engine_loop, 0, 1, &zero, &zero));
    word(e, 0x228, BH_SOUND_INVALID);
    word(e, 0x22c, BH_SOUND_INVALID);
    word(e, 0x224, BH_SOUND_INVALID);
    real(e, 0x254, 0);
    real(e, 0x250, 1);
    real(e, 0x24c, 0);
    memset((char *)e + 0x2b8, 0, 3);
    word(e, 0x230, 10000);
    word(e, 0x234, 0);
    word(e, 0x238, 0);
    *((unsigned char *)e + 0x2bb) = (unsigned char)vehicle;
    if (!bh_player_tick_bind(t, e))
        return failed(c, 3);
    e->refresh = refresh;
    c->query->player = e;
    c->query->contact_physics = t->body->physics;
    c->query->weapons = &t->weapons->factory->registry;
    if (!bh_player_query_bind(c->query))
        return failed(c, 3);
    word(e, 0x21c, model);
    for (n = 0; n < c->actor_capacity && c->actors[n]; ++n)
    {
    }
    if (n + 1 >= c->actor_capacity)
        return failed(c, 4);
    c->actors[n] = e;
    c->actors[n + 1] = 0;
    t->weapons->player = e;
    for (i = 0; i < 10; ++i)
    {
        w = bh_weapon_factory_create(t->weapons->factory, i);
        word(e, 0x258 + i * 4, (unsigned long)w);
        if (!w)
            return failed(c, 2);
    }
    *((unsigned char *)e + 0x2b6) = 0;
    *((unsigned char *)e + 0x2b7) = 9;
    for (i = 0; i < 2; ++i)
    {
        if (!bh_weapon_registry_resolve(&t->weapons->factory->registry, i ? 9 : 0, &w, &state))
            return failed(c, 3);
        state->common.available_ec = 1;
        ammo = state->common.ammo + (i ? 1 : 300);
        state->common.ammo = ammo > state->common.ammo_limit ? state->common.ammo_limit : ammo;
    }
    body = t->body;
    body->entity = e;
    body->generation = e->generation;
    body->geometry = &c->collision->geometry;
    body->mass = 1000;
    body->friction = .9f;
    body->world = 0;
    if (!bh_physics_step_read_entity(body->physics, e))
        return failed(c, 3);
    t->force->body = body;
    t->camera->player = e;
    if (t->scene)
        t->scene->player = e;
    ++c->created;
    return e;
}
void __cdecl bh_player_create_delayed(BHEntity *e)
{
    BHPlayerCreate *c;
    float delay;
    unsigned long model;
    BHVec3 position;
    unsigned char vehicle;
    if (!e)
        return;
    memcpy(&c, e->payload + 516, 4);
    if (!c || !c->tick || !c->tick->pool)
        return;
    memcpy(&delay, (char *)e + 0x23c, 4);
    delay = rf((double)delay - 1.0 / 30);
    real(e, 0x23c, delay);
    if (delay <= 0)
    {
        position = e->position;
        vehicle = *((unsigned char *)e + 0x2bb);
        memcpy(&model, (char *)e + 0x21c, 4);
        bh_player_create(c, 0, &position, vehicle, model);
        bh_entity_release(c->tick->pool, e);
        ++c->expired;
    }
}
