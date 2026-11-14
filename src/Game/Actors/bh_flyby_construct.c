#include "bh_flyby_construct.h"
#include "../../Math/bh_fp.h"
#include <string.h>
#include <float.h>
static void publish(BHEntity *flyby, const BHFlybyCameraState *state)
{
    flyby->position = state->position;
    flyby->flyby.previous_position = state->previous_position;
    flyby->flyby.older_position = state->older_position;
    flyby->basis[0] = state->basis_x;
    flyby->basis[1] = state->basis_y;
    flyby->basis[2] = state->basis_z;
    flyby->euler = state->angles;
    flyby->velocity = state->velocity;
    flyby->flyby.camera = state->camera;
    flyby->world = state->route.current;
}
BHEntity *bh_flyby_construct(BHFlybyConstruct *flyby_construct)
{
    BHFlybyScene *flyby_scene;
    BHFlybyCameraState *state;
    BHFlybyCameraContext *camera;
    BHEntity *flyby, *previous;
    BHVec3 zero = {0, 0, 0}, position;
    long selected;
    unsigned long i;
    float pi = 3.1415927410125732421875f;
    if (!flyby_construct || !(flyby_scene = flyby_construct->scene) || !flyby_construct->current_camera || !flyby_scene->entities || !flyby_scene->flyby_state || !(camera = flyby_scene->flyby_camera) ||
        !camera->route || !camera->camera || !flyby_scene->starts || flyby_scene->start_count <= 23 || !flyby_scene->selected_waypoint ||
        !flyby_scene->mode_count || !flyby_scene->player_slot || !flyby_scene->player_camera || !flyby_scene->current_player || !*flyby_scene->current_player ||
        flyby_scene->bound_entity || flyby_scene->player_camera->starts != flyby_scene->starts || flyby_scene->player_camera->start_count != flyby_scene->start_count ||
        !bh_math_environment_supported())
    {
        if (flyby_construct)
            flyby_construct->error = 1;
        return NULL;
    }
    selected = *flyby_scene->selected_waypoint;
    if (selected < 0 || (unsigned long)selected >= flyby_scene->start_count || camera->starts != flyby_scene->starts ||
        camera->start_count != flyby_scene->start_count)
    {
        flyby_construct->error = 1;
        return NULL;
    }
    position = flyby_scene->starts[selected].position;
    if (!_finite(position.x) || !_finite(position.y) || !_finite(position.z))
    {
        flyby_construct->error = 1;
        return NULL;
    }
    flyby = bh_entity_acquire(flyby_scene->entities);
    if (!flyby)
    {
        flyby_construct->error = 2;
        return NULL;
    }
    previous = *flyby_construct->current_camera;
    *flyby_construct->current_camera = flyby;
    flyby_scene->camera_entity = flyby;
    flyby_construct->settled = 0;
    flyby_construct->error = 0;
    flyby->position = position;
    flyby->world = NULL;
    flyby->flyby.previous_position = position;
    flyby->flyby.older_position = position;
    flyby->flyby.waypoint_index = selected;
    flyby->basis[0] = (BHVec3){1, 0, 0};
    flyby->basis[1] = (BHVec3){0, 1, 0};
    flyby->basis[2] = (BHVec3){0, 0, 1};
    flyby->velocity = zero;
    flyby->angular_velocity = zero;
    flyby->acceleration = zero;
    flyby->angular_acceleration = zero;
    flyby->euler = (BHVec3){pi, 0, 0};
    state = flyby_scene->flyby_state;
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
    flyby->flyby.camera = state->camera;
    flyby->flyby.camera.position = position;
    flyby->flyby.camera.angles = state->angles;
    flyby->flyby.orbit_time = 0;
    flyby->flyby.route_phase = 0;
    state->route.current = bh_world_locate(camera->route->first, camera->route->scene_count, NULL, &position, 0);
    flyby->world = state->route.current;
    if (!bh_flyby_scene_bind(flyby_scene))
        goto failed;
    for (i = 0; i < 50; ++i)
    {
        flyby->step(flyby);
        if (flyby_scene->error != BH_FLYBY_SCENE_OK)
            goto failed;
        ++flyby_construct->settled;
        publish(flyby, state);
    }
    flyby_scene->player_camera->player = *flyby_scene->current_player;
    flyby_scene->player_camera->player_count = *flyby_scene->mode_count;
    flyby_scene->player_camera->local_player_index = *flyby_scene->player_slot;
    if (!bh_player_camera_reset(flyby_scene->player_camera, 23))
        goto failed;
    return flyby;
failed:
    flyby_construct->error = 3;
    bh_flyby_scene_unbind(flyby_scene);
    bh_entity_release(flyby_scene->entities, flyby);
    flyby_scene->camera_entity = NULL;
    *flyby_construct->current_camera = previous;
    return NULL;
}
