#include "bh_flyby_scene.h"
#include "bh_camera_queue.h"
#include "bh_horizon_queue.h"
#include "bh_pickup_draw.h"
#include "bh_queue_reserve.h"
#include "bh_round.h"
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

typedef char flyby_scene_pointer_size[sizeof(void *) == 4 ? 1 : -1];
typedef char flyby_scene_payload_offset[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char flyby_scene_input_state_offset[offsetof(BHPlayerInputList, state) == 0x2010 ? 1 : -1];
typedef char flyby_scene_crt_random[RAND_MAX == 32767 ? 1 : -1];

static BHFlybyScene *flyby_bindings;

static BHVec3 read_vector(const BHEntity *e, unsigned long offset)
{
    BHVec3 v;
    memcpy(&v, (const unsigned char *)e + offset, sizeof(v));
    return v;
}
static long read_long(const BHEntity *e, unsigned long offset)
{
    long v;
    memcpy(&v, (const unsigned char *)e + offset, sizeof(v));
    return v;
}
static void write_vector(BHEntity *e, unsigned long offset, const BHVec3 *v)
{
    memcpy((unsigned char *)e + offset, v, sizeof(*v));
}
static void write_long(BHEntity *e, unsigned long offset, long v)
{
    memcpy((unsigned char *)e + offset, &v, sizeof(v));
}
static float stored(double x)
{
    float v;
    bh_round_float(&v, x);
    return v;
}
static int fail(BHFlybyScene *s, BHFlybySceneError e)
{
    if (s)
        s->error = e;
    return 0;
}
static void queue_result(BHFlybyScene *s, int result)
{
    ++s->attempted;
    if (result > 0)
        ++s->queued;
    else if (result == 0)
        ++s->dropped;
    else if (s->error == BH_FLYBY_SCENE_OK)
        s->error = BH_FLYBY_SCENE_QUEUE;
}
static int active_entity(const BHEntityPool *pool, const BHEntity *target)
{
    const BHEntity *p;
    unsigned long n = 0;
    if (!pool || !target)
        return 0;
    for (p = pool->active; p && n < pool->used; p = p->next, ++n)
        if (p == target)
            return 1;
    return 0;
}
static BHFlybyScene *binding(const BHEntity *entity)
{
    BHFlybyScene *p;
    for (p = flyby_bindings; p; p = p->registry_next)
        if (p->bound_entity == entity && p->bound_generation == entity->generation &&
            active_entity(p->entities, entity))
            return p;
    return 0;
}
int __cdecl bh_flyby_scene_bind(BHFlybyScene *s)
{
    BHFlybyScene *p;
    if (!s || !s->camera_entity || !s->entities || !s->flyby_camera || !s->flyby_camera->route || !s->flyby_state ||
        !s->mode_count || !s->selected_waypoint || !s->starts || !s->start_count ||
        s->flyby_camera->starts != s->starts || s->flyby_camera->start_count != s->start_count ||
        !active_entity(s->entities, s->camera_entity))
        return fail(s, BH_FLYBY_SCENE_BINDING);
    for (p = flyby_bindings; p; p = p->registry_next)
        if (p == s || p->bound_entity == s->camera_entity)
            return fail(s, BH_FLYBY_SCENE_BINDING);
    s->bound_entity = s->camera_entity;
    s->bound_generation = s->camera_entity->generation;
    s->registry_next = flyby_bindings;
    flyby_bindings = s;
    s->camera_entity->refresh = bh_flyby_scene_refresh_entity;
    s->camera_entity->step = bh_flyby_scene_camera_step_entity;
    s->error = BH_FLYBY_SCENE_OK;
    return 1;
}
void __cdecl bh_flyby_scene_unbind(BHFlybyScene *s)
{
    BHFlybyScene **p;
    if (!s)
        return;
    for (p = &flyby_bindings; *p; p = &(*p)->registry_next)
        if (*p == s)
        {
            *p = s->registry_next;
            if (s->bound_entity && s->bound_generation == s->bound_entity->generation &&
                active_entity(s->entities, s->bound_entity))
            {
                if (s->bound_entity->refresh == bh_flyby_scene_refresh_entity)
                    s->bound_entity->refresh = 0;
                if (s->bound_entity->step == bh_flyby_scene_camera_step_entity ||
                    s->bound_entity->step == bh_flyby_scene_rearm_entity)
                    s->bound_entity->step = 0;
            }
            s->registry_next = 0;
            s->bound_entity = 0;
            s->bound_generation = 0;
            return;
        }
}
void __cdecl bh_flyby_scene_refresh_entity(BHEntity *entity, void *packet)
{
    BHFlybyScene *s = binding(entity);
    (void)packet;
    if (s)
    {
        if (s->camera_entity != entity)
            s->error = BH_FLYBY_SCENE_BINDING;
        else
            bh_flyby_scene_draw(s);
    }
}
void __cdecl bh_flyby_scene_camera_step_entity(BHEntity *entity)
{
    BHFlybyScene *s = binding(entity);
    int result;
    if (!s)
        return;
    s->flyby_camera->route->selected_waypoint = (unsigned long)*s->selected_waypoint;
    result = bh_flyby_camera_step(s->flyby_camera, s->flyby_state);
    if (result < 0)
    {
        s->error = BH_FLYBY_SCENE_OWNER;
        return;
    }
    /* +160/+16c/+178 are outputs of the camera at +e8, not the actor's
       orbit position/angles/velocity at +0c/+3c/+48. Original 00429fb0
       calls 0047aac0 on that camera; drawing and menu placement share it. */
    write_vector(entity, 0x160, &s->flyby_state->camera.position);
    write_vector(entity, 0x16c, &s->flyby_state->camera.angles);
    write_vector(entity, 0x178, &s->flyby_state->camera.velocity);
    write_long(entity, 0x284, (long)s->flyby_state->route.waypoint_index);
}
void __cdecl bh_flyby_scene_rearm_entity(BHEntity *entity)
{
    BHFlybyScene *s = binding(entity);
    if (!s)
        return;
    if (*s->mode_count == 0)
    {
        entity->refresh = bh_flyby_scene_refresh_entity;
        entity->step = bh_flyby_scene_camera_step_entity;
    }
}
static BHPlayerInputList *player_input(BHFlybyScene *s, BHEntity *player)
{
    BHPlayerInputList *p, *target;
    unsigned long n = 0;
    if (!s->input_owner || !player)
        return 0;
    target = (BHPlayerInputList *)(unsigned long)read_long(player, 0x188);
    for (p = s->input_owner->first; p && n < 65536; p = p->next, ++n)
        if (p == target)
            return p;
    return 0;
}
static int reset_player(BHFlybyScene *s, unsigned long index)
{
    if (!s->current_player || !*s->current_player || !s->player_camera || !s->player_slot || !s->mode_count)
        return fail(s, BH_FLYBY_SCENE_BINDING);
    s->player_camera->player = *s->current_player;
    s->player_camera->player_count = *s->mode_count;
    s->player_camera->local_player_index = *s->player_slot;
    if (!bh_player_camera_reset(s->player_camera, index))
        return fail(s, BH_FLYBY_SCENE_OWNER);
    return 1;
}
static int handoff(BHFlybyScene *s, BHEntity *camera)
{
    BHEntity *player;
    BHPlayerInputList *list;
    BHPlayerInputState decoded;
    long scenario, count, slot;
    unsigned long index;
    struct
    {
        long event, value, message;
    } message;
    if (!s->current_player || !(player = *s->current_player) || !s->input_profile || !s->input_owner ||
        !s->player_camera || !s->player_slot || !s->scenario || !s->event_value || !s->mission_identity ||
        !s->entities || s->player_camera->starts != s->starts || s->player_camera->start_count != s->start_count ||
        !active_entity(s->entities, player))
        return fail(s, BH_FLYBY_SCENE_BINDING);
    list = player_input(s, player);
    if (!list)
        return fail(s, BH_FLYBY_SCENE_BINDING);
    bh_player_input_setup(list, s->input_profile);
    player = *s->current_player;
    list = player_input(s, player);
    if (!list || !bh_input_set_enabled(list, 0, 1))
        return fail(s, BH_FLYBY_SCENE_OWNER);
    player = *s->current_player;
    list = player_input(s, player);
    if (!list)
        return fail(s, BH_FLYBY_SCENE_BINDING);
    /* Original 00475870 takes (packet, destination). The source API takes
       (destination, packet). +2010 is serialized input; +18c is the player's
       decoded 36-word window, never a packet or a full 1024-byte state. */
    memset(&decoded, 0, sizeof(decoded));
    memcpy(decoded.values, (const unsigned char *)player + 0x18c, 36 * 4);
    if (!bh_player_input_apply_packet_checked(&decoded, list->state, sizeof(list->state)))
        return fail(s, BH_FLYBY_SCENE_PACKET);
    for (index = 36; index < 256; ++index)
        if (decoded.values[index])
            return fail(s, BH_FLYBY_SCENE_PACKET);
    memcpy((unsigned char *)player + 0x18c, decoded.values, 36 * 4);
    player = *s->current_player;
    if (!player)
        return fail(s, BH_FLYBY_SCENE_BINDING);
    write_long(player, 0x210, 2);
    scenario = *s->scenario;
    if (scenario >= 0 && scenario < 3)
    {
        if (!reset_player(s, (unsigned long)(0x14 + scenario)))
            return 0;
        player = *s->current_player;
        if (!player)
            return fail(s, BH_FLYBY_SCENE_BINDING);
        write_long(player, 0x21c, (long)s->scenario_resources[scenario]);
        *s->mission_identity = scenario == 0 ? 0x00415530UL : scenario == 1 ? 0x00416120UL : 0x00416eb0UL;
    }
    count = *s->mode_count;
    if (count > 1)
    {
        long value = rand();
        ++s->random_calls;
        index = (unsigned long)((value + ((value >> 31) & 0xfff)) >> 12);
        if (!reset_player(s, index))
            return 0;
    }
    message.event = 14;
    message.value = (long)*s->event_value;
    message.message = 0x2707;
    if (!bh_entities_query(s->entities, 0, 0, 0, 0, &message))
        return fail(s, BH_FLYBY_SCENE_OWNER);
    message.event = 14;
    count = *s->mode_count;
    if (count != 1)
        message.message = 0xd03;
    else
    {
        slot = *s->player_slot;
        if (slot == 0)
            message.message = 0xd04;
        else if (slot >= 1 && slot <= 8)
        {
            message.message = 0xd47 + slot;
            if (!reset_player(s, (unsigned long)(slot - 1)))
                return 0;
        }
        /* Other selector values leave the first message word untouched. */
    }
    message.value = (long)*s->event_value;
    if (!bh_entities_query(s->entities, 0, 0, 0, 0, &message))
        return fail(s, BH_FLYBY_SCENE_OWNER);
    if (!active_entity(s->entities, camera) || (s->bound_generation && camera->generation != s->bound_generation))
        return fail(s, BH_FLYBY_SCENE_BINDING);
    camera->step = bh_flyby_scene_rearm_entity;
    camera->refresh = 0; /* verified 0047d680 shared RET */
    return 1;
}

int __cdecl bh_flyby_scene_draw(BHFlybyScene *s)
{
    BHEntity *camera;
    BHVec3 eye, angles, velocity;
    long waypoint;
    float left, side, depth;
    unsigned long i;
    int full = 0;
    if (!s || !(camera = s->camera_entity) || !s->queue || !s->sound || !s->background || !s->scene || !s->starts ||
        !s->mode_count || !s->selected_waypoint || !s->display_width || !s->display_height || !s->near_z ||
        *s->display_width <= 0 || *s->display_height <= 0 || !active_entity(s->entities, camera))
        return fail(s, BH_FLYBY_SCENE_ARGUMENT);
    if (s->background->queue != s->queue || !s->scene->bindings ||
        (s->scene->bindings->schedule_context && s->scene->bindings->schedule_context->queue != s->queue))
        return fail(s, BH_FLYBY_SCENE_BINDING);
    s->error = BH_FLYBY_SCENE_OK;
    s->attempted = s->queued = s->dropped = s->random_calls = 0;
    eye = read_vector(camera, 0x160);
    angles = read_vector(camera, 0x16c);
    velocity = read_vector(camera, 0x178);
    if (!bh_sound_listener(s->sound, 0, 1, &eye, &angles, &velocity))
        s->error = BH_FLYBY_SCENE_OWNER;
    eye = read_vector(camera, 0x160);
    angles = read_vector(camera, 0x16c);
    queue_result(s, bh_camera_queue_pose(s->queue, &eye, &angles));
    queue_result(s, bh_horizon_queue(s->queue, s->horizon_image, 5000, 5000, 25000, 12500, 0x804000));
    waypoint = read_long(camera, 0x284);
    if (waypoint == 13 || *s->selected_waypoint == 17)
    {
        BHVec3 target;
        double dx;
        float dy, dz;
        if (waypoint < 0 || (unsigned long)waypoint >= s->start_count)
            return fail(s, BH_FLYBY_SCENE_BINDING);
        target = s->starts[waypoint].position;
        eye = read_vector(camera, 0x160);
        dx = (double)target.x - eye.x;
        dy = stored((double)target.y - eye.y);
        dz = stored((double)target.z - eye.z);
        if (!(sqrt((dz * dz + dy * dy) + dx * dx) >= 100.0) || *s->selected_waypoint == 17)
        {
            queue_result(s, bh_pickup_rect_queue(s->queue, 0, s->full_image, (float)(*s->display_width / 2),
                                                 (float)(*s->display_height / 2), (float)*s->display_width,
                                                 (float)*s->display_height, 0, 0, 1, .00390625f, .9921875f, .00390625f,
                                                 .9921875f, 2000));
            full = 1;
        }
    }
    if (!full)
        for (i = 0; i < 8; ++i)
        {
            eye = read_vector(camera, 0x160);
            if (bh_scene_background_step(s->background, i, &eye) < 0 && s->error == BH_FLYBY_SCENE_OK)
                s->error = BH_FLYBY_SCENE_OWNER;
        }
    eye = read_vector(camera, 0x160);
    angles = read_vector(camera, 0x16c);
    if (!bh_scene_owner_step(s->scene, &eye, &angles, 60, .01f, 25000, *s->display_width, *s->display_height) &&
        s->scene->error != BH_SCENE_OWNER_INACTIVE &&
        !(s->scene->error == BH_SCENE_OWNER_QUEUE && s->queue->error == BH_DRAW_FULL) && s->error == BH_FLYBY_SCENE_OK)
        s->error = BH_FLYBY_SCENE_OWNER;
    if (!full)
    {
        queue_result(s, bh_queue_mode(s->queue, 8, 0));
        left = stored((double)*s->display_width * .105);
        side = stored((double)*s->display_width * .2);
        depth = stored((double)*s->near_z - (-.01));
        queue_result(s, bh_pickup_rect_queue(s->queue, 3, s->corner_left_image, left,
                                             stored((double)*s->display_height - left), side, side, 0, 0, 1, .00390625f,
                                             1, .00390625f, 1, depth));
        queue_result(s, bh_pickup_rect_queue(s->queue, 8, s->corner_right_image, stored((double)*s->display_width * .9),
                                             left, side, side, 0, 0, 1, .00390625f, 1, .00390625f, 1, depth));
    }
    if (*s->mode_count && !handoff(s, camera))
        return 0;
    return s->error == BH_FLYBY_SCENE_OK;
}
