#include "bh_flyby_scene.h"
#include "../../Camera/bh_camera_queue.h"
#include "../../Graphics/bh_horizon_queue.h"
#include "../../Graphics/bh_draw_submit.h"
#include "../Items/bh_pickup_draw.h"
#include "../../Core/bh_queue_reserve.h"
#include "../../Math/bh_round.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

static_assert(sizeof(void *) == 4, "flyby scene pointer size");
static_assert(offsetof(BHEntity, payload) == 0xe4, "BHEntity.payload offset");
static_assert(offsetof(BHPlayerInputList, state) == 0x2010, "BHPlayerInputList.state offset");
static_assert(RAND_MAX == 32767, "flyby scene crt random");

static BHFlybyScene *flyby_bindings;

static float stored(double x)
{
    float v;
    bh_round_float(&v, x);
    return v;
}
static int fail(BHFlybyScene *flyby_scene, BHFlybySceneError e)
{
    if (flyby_scene)
        flyby_scene->error = e;
    return 0;
}
static void queue_result(BHFlybyScene *flyby_scene, int result)
{
    ++flyby_scene->attempted;
    if (result > 0)
        ++flyby_scene->queued;
    else if (result == 0)
        ++flyby_scene->dropped;
    else if (flyby_scene->error == BH_FLYBY_SCENE_OK)
        flyby_scene->error = BH_FLYBY_SCENE_QUEUE;
}
static int active_entity(const BHEntityPool *pool, const BHEntity *target)
{
    return bh_entity_is_active(pool, target);
}
static BHFlybyScene *binding(const BHEntity *entity)
{
    BHFlybyScene *flyby_scene;
    for (flyby_scene = flyby_bindings; flyby_scene; flyby_scene = flyby_scene->registry_next)
        if (flyby_scene->bound_entity == entity && flyby_scene->bound_generation == entity->generation &&
            active_entity(flyby_scene->entities, entity))
            return flyby_scene;
    return 0;
}
int bh_flyby_scene_bind(BHFlybyScene *flyby_scene)
{
    BHFlybyScene *current_flyby_scene;
    if (!flyby_scene || !flyby_scene->camera_entity || !flyby_scene->entities || !flyby_scene->flyby_camera || !flyby_scene->flyby_camera->route || !flyby_scene->flyby_state ||
        !flyby_scene->mode_count || !flyby_scene->selected_waypoint || !flyby_scene->starts || !flyby_scene->start_count ||
        flyby_scene->flyby_camera->starts != flyby_scene->starts || flyby_scene->flyby_camera->start_count != flyby_scene->start_count ||
        !active_entity(flyby_scene->entities, flyby_scene->camera_entity))
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    for (current_flyby_scene = flyby_bindings; current_flyby_scene; current_flyby_scene = current_flyby_scene->registry_next)
        if (current_flyby_scene == flyby_scene || current_flyby_scene->bound_entity == flyby_scene->camera_entity)
            return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    flyby_scene->bound_entity = flyby_scene->camera_entity;
    flyby_scene->bound_generation = flyby_scene->camera_entity->generation;
    flyby_scene->registry_next = flyby_bindings;
    flyby_bindings = flyby_scene;
    flyby_scene->camera_entity->refresh = bh_flyby_scene_refresh_entity;
    flyby_scene->camera_entity->step = bh_flyby_scene_camera_step_entity;
    flyby_scene->error = BH_FLYBY_SCENE_OK;
    return 1;
}
void bh_flyby_scene_unbind(BHFlybyScene *flyby_scene)
{
    BHFlybyScene **p;
    if (!flyby_scene)
        return;
    for (p = &flyby_bindings; *p; p = &(*p)->registry_next)
        if (*p == flyby_scene)
        {
            *p = flyby_scene->registry_next;
            if (flyby_scene->bound_entity && flyby_scene->bound_generation == flyby_scene->bound_entity->generation &&
                active_entity(flyby_scene->entities, flyby_scene->bound_entity))
            {
                if (flyby_scene->bound_entity->refresh == bh_flyby_scene_refresh_entity)
                    flyby_scene->bound_entity->refresh = 0;
                if (flyby_scene->bound_entity->step == bh_flyby_scene_camera_step_entity ||
                    flyby_scene->bound_entity->step == bh_flyby_scene_rearm_entity)
                    flyby_scene->bound_entity->step = 0;
            }
            flyby_scene->registry_next = 0;
            flyby_scene->bound_entity = 0;
            flyby_scene->bound_generation = 0;
            return;
        }
}
void bh_flyby_scene_refresh_entity(BHEntity *entity, void *packet)
{
    BHFlybyScene *flyby_scene = binding(entity);
    (void)packet;
    if (flyby_scene)
    {
        if (flyby_scene->camera_entity != entity)
            flyby_scene->error = BH_FLYBY_SCENE_BINDING;
        else
            bh_flyby_scene_draw(flyby_scene);
    }
}
void bh_flyby_scene_camera_step_entity(BHEntity *entity)
{
    BHFlybyScene *flyby_scene = binding(entity);
    int result;
    if (!flyby_scene)
        return;
    flyby_scene->flyby_camera->route->selected_waypoint = (unsigned long)*flyby_scene->selected_waypoint;
    result = bh_flyby_camera_step(flyby_scene->flyby_camera, flyby_scene->flyby_state);
    if (result < 0)
    {
        flyby_scene->error = BH_FLYBY_SCENE_OWNER;
        return;
    }
    /* +160/+16c/+178 are outputs of the camera at +e8, not the actor's
       orbit position/angles/velocity at +0c/+3c/+48. Original 00429fb0
       calls 0047aac0 on that camera; drawing and menu placement share it. */
    entity->flyby.camera.position = flyby_scene->flyby_state->camera.position;
    entity->flyby.camera.angles = flyby_scene->flyby_state->camera.angles;
    entity->flyby.camera.velocity = flyby_scene->flyby_state->camera.velocity;
    entity->flyby.waypoint_index = (long)flyby_scene->flyby_state->route.waypoint_index;
}
void bh_flyby_scene_rearm_entity(BHEntity *entity)
{
    BHFlybyScene *flyby_scene = binding(entity);
    if (!flyby_scene)
        return;
    if (*flyby_scene->mode_count == 0)
    {
        entity->refresh = bh_flyby_scene_refresh_entity;
        entity->step = bh_flyby_scene_camera_step_entity;
    }
}
static BHPlayerInputList *player_input(BHFlybyScene *flyby_scene, BHEntity *player)
{
    BHPlayerInputList *player_input_list, *target;
    unsigned long n = 0;
    if (!flyby_scene->input_owner || !player)
        return 0;
    target = player->player.input_list;
    for (player_input_list = flyby_scene->input_owner->first; player_input_list && n < 65536; player_input_list = player_input_list->next, ++n)
        if (player_input_list == target)
            return player_input_list;
    return 0;
}
static int reset_player(BHFlybyScene *flyby_scene, unsigned long index)
{
    if (!flyby_scene->current_player || !*flyby_scene->current_player || !flyby_scene->player_camera || !flyby_scene->player_slot || !flyby_scene->mode_count)
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    flyby_scene->player_camera->player = *flyby_scene->current_player;
    flyby_scene->player_camera->player_count = *flyby_scene->mode_count;
    flyby_scene->player_camera->local_player_index = *flyby_scene->player_slot;
    if (!bh_player_camera_reset(flyby_scene->player_camera, index))
        return fail(flyby_scene, BH_FLYBY_SCENE_OWNER);
    return 1;
}
static int handoff(BHFlybyScene *flyby_scene, BHEntity *camera)
{
    static const BHFlybyForceControl scenario_controls[] = {
        bh_player_force_control_b, bh_player_force_control_c, bh_player_force_control_d};
    BHEntity *player;
    BHPlayerInputList *list;
    BHPlayerInputState decoded;
    BHFlybyForceControl control;
    long scenario, count, slot;
    unsigned long index, model;
    int solo_vanos;
    struct
    {
        long event, value, message;
    } message;
    if (!flyby_scene->current_player || !(player = *flyby_scene->current_player) || !flyby_scene->input_profile || !flyby_scene->input_owner ||
        !flyby_scene->player_camera || !flyby_scene->player_slot || !flyby_scene->scenario || !flyby_scene->event_value || !flyby_scene->mission_identity ||
        !flyby_scene->force_control || !flyby_scene->entities || flyby_scene->player_camera->starts != flyby_scene->starts ||
        flyby_scene->player_camera->start_count != flyby_scene->start_count || !active_entity(flyby_scene->entities, player))
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    list = player_input(flyby_scene, player);
    if (!list)
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    bh_player_input_setup(list, flyby_scene->input_profile);
    player = *flyby_scene->current_player;
    list = player_input(flyby_scene, player);
    if (!list || !bh_input_set_enabled(list, 0, 1))
        return fail(flyby_scene, BH_FLYBY_SCENE_OWNER);
    player = *flyby_scene->current_player;
    list = player_input(flyby_scene, player);
    if (!list)
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    /* Original 00475870 takes (packet, destination). The source API takes
       (destination, packet). +2010 is serialized input; +18c is the player's
       decoded 36-word window, never a packet or a full 1024-byte state. */
    memset(&decoded, 0, sizeof(decoded));
    memcpy(decoded.values, player->player.input.words, sizeof(player->player.input.words));
    if (!bh_player_input_apply_packet_checked(&decoded, list->state, sizeof(list->state)))
        return fail(flyby_scene, BH_FLYBY_SCENE_PACKET);
    for (index = BH_PLAYER_INPUT_WORDS; index < 256; ++index)
        if (decoded.values[index])
            return fail(flyby_scene, BH_FLYBY_SCENE_PACKET);
    memcpy(player->player.input.words, decoded.values, sizeof(player->player.input.words));
    player = *flyby_scene->current_player;
    if (!player)
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    player->player.input.words[BH_PLAYER_INPUT_ESCAPE] = 2;
    scenario = *flyby_scene->scenario;
    count = *flyby_scene->mode_count;
    solo_vanos = count == 1 && scenario == BH_GARAGE_VANOS_ROW;
    if (solo_vanos)
        scenario = 0;
    if (scenario >= 0 && (unsigned long)scenario < sizeof(scenario_controls) / sizeof(scenario_controls[0]))
    {
        model = flyby_scene->scenario_model_override ? flyby_scene->scenario_model_override : flyby_scene->scenario_resources[scenario];
        control = flyby_scene->scenario_force_override ? flyby_scene->scenario_force_override : scenario_controls[scenario];
        if (solo_vanos)
        {
            model = flyby_scene->vanos_resource;
            control = bh_player_force_control_vanos;
        }
        if (!reset_player(flyby_scene, (unsigned long)(0x14 + scenario)))
            return 0;
        player = *flyby_scene->current_player;
        if (!player)
            return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
        player->player.model_resource = model;
        *flyby_scene->mission_identity = scenario == 0 ? 0x00415530UL : scenario == 1 ? 0x00416120UL : 0x00416eb0UL;
        *flyby_scene->force_control = control;
        if (flyby_scene->vanos_damage_resistance)
            *flyby_scene->vanos_damage_resistance = control == bh_player_force_control_vanos;
    }
    if (count > 1)
    {
        long value = rand();
        ++flyby_scene->random_calls;
        index = (unsigned long)((value + ((value >> 31) & 0xfff)) >> 12);
        index = (index + (unsigned long)*flyby_scene->player_slot) & 7;
        if (!reset_player(flyby_scene, index))
            return 0;
    }
    message.event = 14;
    message.value = (long)*flyby_scene->event_value;
    message.message = 0x2707;
    if (!bh_entities_query(flyby_scene->entities, 0, 0, 0, 0, &message))
        return fail(flyby_scene, BH_FLYBY_SCENE_OWNER);
    message.event = 14;
    count = *flyby_scene->mode_count;
    if (count != 1)
        message.message = 0xd03;
    else
    {
        slot = *flyby_scene->player_slot;
        if (slot == 0)
            message.message = 0xd04;
        else if (slot >= 1 && slot <= 8)
        {
            message.message = 0xd47 + slot;
            if (!reset_player(flyby_scene, (unsigned long)(slot - 1)))
                return 0;
        }
        /* Other selector values leave the first message word untouched. */
    }
    message.value = (long)*flyby_scene->event_value;
    if (!bh_entities_query(flyby_scene->entities, 0, 0, 0, 0, &message))
        return fail(flyby_scene, BH_FLYBY_SCENE_OWNER);
    if (!active_entity(flyby_scene->entities, camera) || (flyby_scene->bound_generation && camera->generation != flyby_scene->bound_generation))
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    camera->step = bh_flyby_scene_rearm_entity;
    camera->refresh = 0; /* verified 0047d680 shared RET */
    return 1;
}

int bh_flyby_scene_draw(BHFlybyScene *flyby_scene)
{
    BHEntity *camera;
    BHVec3 eye, angles, velocity;
    long waypoint;
    float left, side, depth;
    unsigned long i;
    int full = 0;
    if (!flyby_scene || !(camera = flyby_scene->camera_entity) || !flyby_scene->queue || !flyby_scene->sound || !flyby_scene->background || !flyby_scene->scene || !flyby_scene->starts ||
        !flyby_scene->mode_count || !flyby_scene->selected_waypoint || !flyby_scene->display_width || !flyby_scene->display_height || !flyby_scene->near_z ||
        *flyby_scene->display_width <= 0 || *flyby_scene->display_height <= 0 || !active_entity(flyby_scene->entities, camera))
        return fail(flyby_scene, BH_FLYBY_SCENE_ARGUMENT);
    if (flyby_scene->background->queue != flyby_scene->queue || !flyby_scene->scene->bindings ||
        (flyby_scene->scene->bindings->schedule_context && flyby_scene->scene->bindings->schedule_context->queue != flyby_scene->queue))
        return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
    flyby_scene->error = BH_FLYBY_SCENE_OK;
    flyby_scene->attempted = flyby_scene->queued = flyby_scene->dropped = flyby_scene->random_calls = 0;
    eye = camera->flyby.camera.position;
    angles = camera->flyby.camera.angles;
    velocity = camera->flyby.camera.velocity;
    if (!bh_sound_listener(flyby_scene->sound, 0, 1, &eye, &angles, &velocity))
        flyby_scene->error = BH_FLYBY_SCENE_OWNER;
    eye = camera->flyby.camera.position;
    angles = camera->flyby.camera.angles;
    queue_result(flyby_scene, bh_camera_queue_pose(flyby_scene->queue, &eye, &angles));
    queue_result(flyby_scene, bh_horizon_queue(flyby_scene->queue, flyby_scene->horizon_image, 5000, 5000, 25000, 12500, 0x804000));
    waypoint = camera->flyby.waypoint_index;
    if (waypoint == 13 || *flyby_scene->selected_waypoint == 17)
    {
        BHVec3 target;
        double dx;
        float dy, dz;
        if (waypoint < 0 || (unsigned long)waypoint >= flyby_scene->start_count)
            return fail(flyby_scene, BH_FLYBY_SCENE_BINDING);
        target = flyby_scene->starts[waypoint].position;
        eye = camera->flyby.camera.position;
        dx = (double)target.x - eye.x;
        dy = stored((double)target.y - eye.y);
        dz = stored((double)target.z - eye.z);
        if (!(sqrt((dz * dz + dy * dy) + dx * dx) >= 100.0) || *flyby_scene->selected_waypoint == 17)
        {
            queue_result(flyby_scene, bh_pickup_rect_queue(flyby_scene->queue, 0, flyby_scene->full_image, (float)(*flyby_scene->display_width / 2),
                                                 (float)(*flyby_scene->display_height / 2), (float)*flyby_scene->display_width,
                                                 (float)*flyby_scene->display_height, 0, 0, 1, .00390625f, .9921875f, .00390625f,
                                                 .9921875f, 2000));
            full = 1;
        }
    }
    if (!full)
        for (i = 0; i < 8; ++i)
        {
            eye = camera->flyby.camera.position;
            if (bh_scene_background_step(flyby_scene->background, i, &eye) < 0 && flyby_scene->error == BH_FLYBY_SCENE_OK)
                flyby_scene->error = BH_FLYBY_SCENE_OWNER;
        }
    eye = camera->flyby.camera.position;
    angles = camera->flyby.camera.angles;
    if (!bh_scene_owner_step(flyby_scene->scene, &eye, &angles, flyby_scene->scene->view_fov_degrees, .01f, 25000, *flyby_scene->display_width,
                             *flyby_scene->display_height) &&
        flyby_scene->scene->error != BH_SCENE_OWNER_INACTIVE &&
        !(flyby_scene->scene->error == BH_SCENE_OWNER_QUEUE && flyby_scene->queue->error == BH_DRAW_FULL) && flyby_scene->error == BH_FLYBY_SCENE_OK)
        flyby_scene->error = BH_FLYBY_SCENE_OWNER;
    if (!full)
    {
        const BHPlayerCameraStart *display = flyby_scene->vanos_display;
        if (!*flyby_scene->mode_count && display)
            queue_result(flyby_scene, bh_draw_submit_resource(flyby_scene->queue, 0, flyby_scene->vanos_resource,
                                                             &display->position, &display->angles, 1));
        queue_result(flyby_scene, bh_queue_mode(flyby_scene->queue, 8, 0));
        left = stored((double)*flyby_scene->display_width * .105);
        side = stored((double)*flyby_scene->display_width * .2);
        depth = stored((double)*flyby_scene->near_z - (-.01));
        queue_result(flyby_scene, bh_pickup_rect_queue(flyby_scene->queue, 3, flyby_scene->corner_left_image, left,
                                             stored((double)*flyby_scene->display_height - left), side, side, 0, 0, 1, .00390625f,
                                             1, .00390625f, 1, depth));
        queue_result(flyby_scene, bh_pickup_rect_queue(flyby_scene->queue, 8, flyby_scene->corner_right_image, stored((double)*flyby_scene->display_width * .9),
                                             left, side, side, 0, 0, 1, .00390625f, 1, .00390625f, 1, depth));
    }
    if (*flyby_scene->mode_count && !handoff(flyby_scene, camera))
        return 0;
    return flyby_scene->error == BH_FLYBY_SCENE_OK;
}
