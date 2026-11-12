#include "bh_flyby_construct.h"
#include "bh_fp.h"
#include <string.h>
#include <float.h>
static void word(BHEntity *e, unsigned long at, unsigned long value)
{
    memcpy((unsigned char *)e + at, &value, 4);
}
static void publish(BHEntity *e, const BHFlybyCameraState *state)
{
    e->position = state->position;
    e->flyby.previous_position = state->previous_position;
    e->flyby.older_position = state->older_position;
    e->basis[0] = state->basis_x;
    e->basis[1] = state->basis_y;
    e->basis[2] = state->basis_z;
    e->euler = state->angles;
    e->velocity = state->velocity;
    e->flyby.camera = state->camera;
    e->world = state->route.current;
}
BHEntity *bh_flyby_construct(BHFlybyConstruct *c)
{
    BHFlybyScene *s;
    BHFlybyCameraState *state;
    BHFlybyCameraContext *camera;
    BHEntity *e, *previous;
    BHVec3 zero = {0, 0, 0}, position;
    long selected;
    unsigned long i;
    float pi = 3.1415927410125732421875f;
    if (!c || !(s = c->scene) || !c->current_camera || !s->entities || !s->flyby_state || !(camera = s->flyby_camera) ||
        !camera->route || !camera->camera || !s->starts || s->start_count <= 23 || !s->selected_waypoint ||
        !s->mode_count || !s->player_slot || !s->player_camera || !s->current_player || !*s->current_player ||
        s->bound_entity || s->player_camera->starts != s->starts || s->player_camera->start_count != s->start_count ||
        !bh_math_environment_supported())
    {
        if (c)
            c->error = 1;
        return NULL;
    }
    selected = *s->selected_waypoint;
    if (selected < 0 || (unsigned long)selected >= s->start_count || camera->starts != s->starts ||
        camera->start_count != s->start_count)
    {
        c->error = 1;
        return NULL;
    }
    position = s->starts[selected].position;
    if (!_finite(position.x) || !_finite(position.y) || !_finite(position.z))
    {
        c->error = 1;
        return NULL;
    }
    e = bh_entity_acquire(s->entities);
    if (!e)
    {
        c->error = 2;
        return NULL;
    }
    previous = *c->current_camera;
    *c->current_camera = e;
    s->camera_entity = e;
    c->settled = 0;
    c->error = 0;
    e->position = position;
    e->world = NULL;
    e->flyby.previous_position = position;
    e->flyby.older_position = position;
    e->flyby.waypoint_index = selected;
    e->basis[0] = (BHVec3){1, 0, 0};
    e->basis[1] = (BHVec3){0, 1, 0};
    e->basis[2] = (BHVec3){0, 0, 1};
    e->velocity = zero;
    e->angular_velocity = zero;
    e->acceleration = zero;
    e->angular_acceleration = zero;
    e->euler = (BHVec3){pi, 0, 0};
    state = s->flyby_state;
    state->position = state->previous_position = state->older_position = position;
    state->angles.x = pi;
    state->angles.y = state->angles.z = 0;
    state->velocity = zero;
    state->basis_x.x = state->basis_y.y = state->basis_z.z = 1;
    state->basis_x.y = state->basis_x.z = state->basis_y.x = state->basis_y.z = state->basis_z.x = state->basis_z.y = 0;
    state->route.waypoint_index = (unsigned long)selected;
    state->route.phase = 0;
    state->route.parameter_rate = 0;
    state->orbit_time = 0;
    if (!bh_camera_state_init(&state->camera, 10))
        goto failed;
    e->flyby.camera = state->camera;
    e->flyby.camera.position = position;
    e->flyby.camera.angles = state->angles;
    word(e, 0x1f8, 0);
    word(e, 0x1fc, 0);
    word(e, 0x1dc, 0);
    state->route.current = bh_world_locate(camera->route->first, camera->route->scene_count, NULL, &position, 0);
    e->world = state->route.current;
    if (!bh_flyby_scene_bind(s))
        goto failed;
    for (i = 0; i < 50; ++i)
    {
        e->step(e);
        if (s->error != BH_FLYBY_SCENE_OK)
            goto failed;
        ++c->settled;
        publish(e, state);
    }
    s->player_camera->player = *s->current_player;
    s->player_camera->player_count = *s->mode_count;
    s->player_camera->local_player_index = *s->player_slot;
    if (!bh_player_camera_reset(s->player_camera, 23))
        goto failed;
    return e;
failed:
    c->error = 3;
    bh_flyby_scene_unbind(s);
    bh_entity_release(s->entities, e);
    s->camera_entity = NULL;
    *c->current_camera = previous;
    return NULL;
}
