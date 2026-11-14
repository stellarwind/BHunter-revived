#include "bh_rail_wagon.h"
#include "Routes/bh_route_query.h"
#include "../Math/bh_rotation.h"
#include "../Math/bh_round.h"
#include "../Math/bh_fp.h"
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <float.h>
#define PI 3.14159265358979323846
typedef struct Private
{
    BHRailWagonContext *context;
    BHWorldGeometry *world;
    unsigned long leader_generation, rider_generation;
} Private;
static_assert(sizeof(BHRailWagonPose) == 48 && offsetof(BHEntity, opaque0) == 0x18 &&
                  offsetof(BHEntity, payload) + 4 == 0xe8 && offsetof(BHRailWagonState, actor_next) == 12 &&
                  offsetof(BHRailWagonState, kind) == 20 && offsetof(BHRailWagonState, segment) == 64 &&
                  offsetof(BHRailWagonState, checkpoint) == 80 && sizeof(Private) + 580 <= 636,
              "wagon layout");
static BHRailWagonState *state(BHEntity *entity)
{
    return (BHRailWagonState *)(entity->payload + 4);
}
static Private *private_data(BHEntity *entity)
{
    return (Private *)(entity->payload + 580);
}
BHRailWagonPose *bh_rail_wagon_pose(BHEntity *entity)
{
    return entity ? (BHRailWagonPose *)entity->opaque0 : NULL;
}
static void failure(BHRailWagonContext *context, BHRailWagonError error)
{
    if (context)
    {
        ++context->failures;
        context->error = error;
    }
}
static int live(BHRailWagonContext *context, BHEntity *entity, unsigned long generation)
{
    return bh_entity_is_active(context->pool, entity) && entity->generation == generation;
}
static int member(BHRailWagonContext *context, BHEntity *e)
{
    return bh_entity_is_active(context->pool, e);
}
static int wagon(BHRailWagonContext *c, BHEntity *entity)
{
    return entity && member(c, entity) && (entity->step == bh_rail_wagon_tick || entity->step == bh_rail_wagon_follow) &&
           state(entity)->kind == 9 && private_data(entity)->context == c;
}
BHRailWagonState *bh_rail_wagon_state(BHEntity *entity)
{
    return entity && (entity->step == bh_rail_wagon_tick || entity->step == bh_rail_wagon_follow) ? state(entity) : NULL;
}
static BHRailWagonContext *owner(BHEntity *e)
{
    BHRailWagonContext *context;
    if (!bh_rail_wagon_state(e))
        return NULL;
    context = private_data(e)->context;
    if (!context || !context->pool || !wagon(context, e))
        return NULL;
    return context;
}
static int segment(BHRailWagonContext *context, BHRailSegment *s)
{
    unsigned long base = (unsigned long)context->rails->segments, at = (unsigned long)s;
    return s && at >= base && at - base < (unsigned long)context->rails->count * sizeof(*s) && (at - base) % sizeof(*s) == 0;
}
static BHRailSegment *walk(BHRailWagonContext *c, BHRailSegment *rail_segment, int next)
{
    BHRailSegment *current_rail_segment;
    if (!segment(c, rail_segment))
    {
        failure(c, BH_WAGON_TOPOLOGY);
        return NULL;
    }
    current_rail_segment = next ? rail_segment->next : rail_segment->previous;
    if (!segment(c, current_rail_segment) || !_finite(current_rail_segment->length) || current_rail_segment->length <= 0)
    {
        failure(c, BH_WAGON_TOPOLOGY);
        return NULL;
    }
    return current_rail_segment;
}
static int ready(BHRailWagonContext *c, BHRailWagonState *rail_wagon_state)
{
    if (!segment(c, rail_wagon_state->segment) || !_finite(rail_wagon_state->duration) || rail_wagon_state->duration <= 0 || !_finite(rail_wagon_state->speed) || rail_wagon_state->speed <= 0 ||
        !_finite(rail_wagon_state->elapsed) || !bh_math_environment_supported())
    {
        failure(c, BH_WAGON_NUMERICAL);
        return 0;
    }
    return 1;
}
static BHPickupRoom *room(BHRailWagonContext *context, BHWorldGeometry *w)
{
    unsigned long i;
    for (i = 0; i < context->room_count; ++i)
        if (context->rooms[i].world == w)
            return context->rooms + i;
    return NULL;
}
static BHEntity *actor_next(BHEntity *actor)
{
    BHEntity *p;
    memcpy(&p, &actor->actor.room_next, 4);
    return p;
}
static BHEntity *actor_previous(BHEntity *actor)
{
    BHEntity *p;
    memcpy(&p, &actor->actor.room_previous, 4);
    return p;
}
static void actor_next_set(BHEntity *actor, BHEntity *v)
{
    memcpy(&actor->actor.room_next, &v, 4);
}
static void actor_previous_set(BHEntity *actor, BHEntity *v)
{
    memcpy(&actor->actor.room_previous, &v, 4);
}
static int unlink_room(BHRailWagonContext *c, BHEntity *e)
{
    BHRailWagonState *rail_wagon_state = state(e);
    BHPickupRoom *pickup_room = room(c, private_data(e)->world);
    if (!pickup_room || (rail_wagon_state->actor_previous && (!member(c, rail_wagon_state->actor_previous) || actor_next(rail_wagon_state->actor_previous) != e)) ||
        (!rail_wagon_state->actor_previous && pickup_room->actors != e) ||
        (rail_wagon_state->actor_next && (!member(c, rail_wagon_state->actor_next) || actor_previous(rail_wagon_state->actor_next) != e)))
    {
        failure(c, BH_WAGON_WORLD);
        return 0;
    }
    if (rail_wagon_state->actor_previous)
        actor_next_set(rail_wagon_state->actor_previous, rail_wagon_state->actor_next);
    else
        pickup_room->actors = rail_wagon_state->actor_next;
    if (rail_wagon_state->actor_next)
        actor_previous_set(rail_wagon_state->actor_next, rail_wagon_state->actor_previous);
    return 1;
}
static void prepend(BHRailWagonContext *c, BHEntity *entity, BHWorldGeometry *w)
{
    BHRailWagonState *rail_wagon_state = state(entity);
    BHPickupRoom *pickup_room = room(c, w);
    private_data(entity)->world = w;
    memcpy(entity->opaque2b, &w, 4);
    rail_wagon_state->actor_next = pickup_room->actors;
    rail_wagon_state->actor_previous = NULL;
    pickup_room->actors = entity;
    if (rail_wagon_state->actor_next)
        actor_previous_set(rail_wagon_state->actor_next, entity);
}
static int migrate(BHRailWagonContext *context, BHEntity *e, const BHVec3 *point)
{
    BHWorldGeometry *w = bh_world_locate(context->first_world, context->world_count, private_data(e)->world, point, 0);
    if (w && w != private_data(e)->world)
    {
        BHPickupRoom *pickup_room = room(context, w);
        if (!pickup_room || (pickup_room->actors && !member(context, pickup_room->actors)))
        {
            failure(context, BH_WAGON_WORLD);
            return 0;
        }
        if (!unlink_room(context, e))
            return 0;
        prepend(context, e, w);
        ++context->migrations;
    }
    return 1;
}
static void difference(const BHVec3 *first, const BHVec3 *second, BHVec3 *delta)
{
    bh_round_float(&delta->x, (double)first->x - second->x);
    bh_round_float(&delta->y, (double)first->y - second->y);
    bh_round_float(&delta->z, (double)first->z - second->z);
}
static void direction(BHRailWagonState *rail_wagon_state, BHVec3 *d)
{
    difference(rail_wagon_state->direction ? &rail_wagon_state->segment->end : &rail_wagon_state->segment->start,
               rail_wagon_state->direction ? &rail_wagon_state->segment->start : &rail_wagon_state->segment->end, d);
}
static void interpolate(BHRailSegment *rail_segment, int forward, double ratio, BHVec3 *out, BHVec3 *delta)
{
    const BHVec3 *from = forward ? &rail_segment->start : &rail_segment->end, *to = forward ? &rail_segment->end : &rail_segment->start;
    float dx, dy, dz;
    bh_round_float(&dx, (double)to->x - from->x);
    bh_round_float(&dy, (double)to->y - from->y);
    bh_round_float(&dz, (double)to->z - from->z);
    bh_round_float(&out->x, ((double)to->x - from->x) * ratio + from->x);
    bh_round_float(&out->y, ((double)to->y - from->y) * ratio + from->y);
    bh_round_float(&out->z, ((double)to->z - from->z) * ratio + from->z);
    if (delta)
    {
        bh_round_float(&delta->x, (double)dx / rail_segment->length);
        bh_round_float(&delta->y, (double)dy / rail_segment->length);
        bh_round_float(&delta->z, (double)dz / rail_segment->length);
    }
}
static void draw_point(BHRailWagonState *rail_wagon_state, BHVec3 *point, BHVec3 *delta)
{
    const BHVec3 *from = rail_wagon_state->direction ? &rail_wagon_state->segment->start : &rail_wagon_state->segment->end;
    double ratio = (double)rail_wagon_state->elapsed / rail_wagon_state->duration;
    direction(rail_wagon_state, delta);
    bh_round_float(&point->x, (double)delta->x * ratio + from->x);
    bh_round_float(&point->y, (double)delta->y * ratio + from->y);
    bh_round_float(&point->z, (double)delta->z * ratio + from->z);
}
static void pose_matrix(BHEntity *entity, BHMat34 *matrix)
{
    BHRailWagonPose *rail_wagon_pose = bh_rail_wagon_pose(entity);
    matrix->m[0][0] = rail_wagon_pose->right.x;
    matrix->m[0][1] = rail_wagon_pose->up.x;
    matrix->m[0][2] = rail_wagon_pose->forward.x;
    matrix->m[0][3] = entity->position.x;
    matrix->m[1][0] = rail_wagon_pose->right.y;
    matrix->m[1][1] = rail_wagon_pose->up.y;
    matrix->m[1][2] = rail_wagon_pose->forward.y;
    matrix->m[1][3] = entity->position.y;
    matrix->m[2][0] = rail_wagon_pose->right.z;
    matrix->m[2][1] = rail_wagon_pose->up.z;
    matrix->m[2][2] = rail_wagon_pose->forward.z;
    matrix->m[2][3] = entity->position.z;
}
static void align(BHEntity *e, const BHVec3 *delta)
{
    BHMat34 m;
    BHVec3 v, unit;
    BHRailWagonPose *rail_wagon_pose = bh_rail_wagon_pose(e);
    double angle;
    float negative;
    bh_vec3_normalize_to(delta, &rail_wagon_pose->forward);
    bh_vec3_cross(&rail_wagon_pose->up, &rail_wagon_pose->forward, &rail_wagon_pose->right);
    rail_wagon_pose->right.y = 0;
    bh_vec3_normalize(&rail_wagon_pose->right);
    bh_vec3_cross(&rail_wagon_pose->forward, &rail_wagon_pose->right, &rail_wagon_pose->up);
    pose_matrix(e, &m);
    unit.x = 0;
    unit.y = 0;
    unit.z = 1;
    bh_mat34_transform_direction(&m, &unit, &v);
    angle = atan2(v.x, v.z);
    bh_round_float(&rail_wagon_pose->angles.x, angle);
    bh_round_float(&negative, -angle);
    bh_mat34_rotate_world_y(negative, &m);
    bh_mat34_transform_direction(&m, &unit, &v);
    angle = atan2(-(double)v.y, v.z);
    bh_round_float(&rail_wagon_pose->angles.y, angle);
    bh_round_float(&negative, -angle);
    bh_mat34_rotate_world_x(negative, &m);
    unit.x = 1;
    unit.z = 0;
    bh_mat34_transform_direction(&m, &unit, &v);
    bh_round_float(&rail_wagon_pose->angles.z, atan2(v.y, v.x));
}
static void cooldown(BHRailWagonContext *context, BHRailWagonState *rail_wagon_state)
{
    if (rail_wagon_state->sound_cooldown)
    {
        rail_wagon_state->sound_cooldown = (long)((unsigned long)rail_wagon_state->sound_cooldown - 1);
        if (!rail_wagon_state->sound_cooldown && rail_wagon_state->sound_instance != BH_SOUND_INVALID)
        {
            bh_sound_stop(context->sound, rail_wagon_state->sound_instance);
            rail_wagon_state->sound_instance = BH_SOUND_INVALID;
        }
    }
}
static double wrapped(double difference)
{
    float stored;
    bh_round_float(&stored, difference);
    if (_isnan(difference) || difference >= PI)
        return fmod((double)stored + PI, 2 * PI) - PI;
    if (stored < -PI)
        return fmod((double)stored - PI, 2 * PI) + PI;
    return stored;
}
static int rider_update(BHRailWagonContext *context, BHEntity *e)
{
    BHRailWagonState *rail_wagon_state = state(e);
    BHRailWagonPose *rail_wagon_pose = bh_rail_wagon_pose(e), *result_rail_wagon_pose;
    BHEntity *rider = rail_wagon_state->rider;
    BHRailSegment *rail = rail_wagon_state->segment;
    BHVec3 velocity, d;
    double distance, ratio, dx, dy, dz, px, py, pz, complement;
    float inv, yaw, pitch, roll, stored_y, stored_z;
    unsigned long clear = 0;
    int crossed = 0;
    if (!rider)
        return 1;
    if (!live(context, rider, private_data(e)->rider_generation))
    {
        failure(context, BH_WAGON_LIFETIME);
        return 0;
    }
    result_rail_wagon_pose = bh_rail_wagon_pose(rider);
    direction(rail_wagon_state, &d);
    bh_round_float(&inv, 1.0 / rail_wagon_state->duration);
    bh_round_float(&velocity.x, (double)d.x * inv);
    bh_round_float(&velocity.y, (double)d.y * inv);
    bh_round_float(&velocity.z, (double)d.z * inv);
    cooldown(context, rail_wagon_state);
    distance = (double)rail_wagon_state->elapsed * rail_wagon_state->speed - (-5.0);
    if (distance > rail->length)
    {
        distance -= rail->length;
        rail = walk(context, rail, rail_wagon_state->direction);
        if (!rail)
            return 0;
        crossed = 1;
    }
    ratio = distance / rail->length;
    {
        const BHVec3 *from = rail_wagon_state->direction ? &rail->end : &rail->start, *to = rail_wagon_state->direction ? &rail->start : &rail->end;
        float f;
        bh_round_float(&f, ratio);
        complement = 1.0 - (crossed ? ratio : (double)f);
        px = (double)from->x * f + (double)to->x * complement;
        py = (double)from->y * f + (double)to->y * complement;
        pz = (double)from->z * f + (double)to->z * complement;
    }
    bh_round_float(&stored_y, py);
    bh_round_float(&stored_z, pz);
    bh_round_float(&rider->position.x, px - (double)rail_wagon_pose->up.x * 5.0);
    bh_round_float(&rider->position.y, (double)stored_y - (double)rail_wagon_pose->up.y * 5.0);
    bh_round_float(&rider->position.z, (double)stored_z - (double)rail_wagon_pose->up.z * 5.0);
    rider->origin = velocity;
    result_rail_wagon_pose->right = rail_wagon_pose->right;
    result_rail_wagon_pose->up = rail_wagon_pose->up;
    result_rail_wagon_pose->forward = rail_wagon_pose->forward;
    dx = wrapped((double)rail_wagon_pose->angles.x - result_rail_wagon_pose->angles.x);
    dy = wrapped((double)rail_wagon_pose->angles.y - result_rail_wagon_pose->angles.y);
    dz = wrapped((double)rail_wagon_pose->angles.z - result_rail_wagon_pose->angles.z);
    bh_round_float(&yaw, dx);
    bh_round_float(&pitch, dy);
    bh_round_float(&roll, dz);
    if (_isnan(yaw) || yaw < -3 * PI / 4 || yaw > 3 * PI / 4)
        result_rail_wagon_pose->angles = rail_wagon_pose->angles;
    else
    {
        bh_round_float(&result_rail_wagon_pose->angles.x, (double)result_rail_wagon_pose->angles.x - yaw * (-0.2));
        bh_round_float(&result_rail_wagon_pose->angles.y, (double)result_rail_wagon_pose->angles.y - pitch * (-0.2));
        bh_round_float(&result_rail_wagon_pose->angles.z, (double)result_rail_wagon_pose->angles.z + roll);
    }
    memcpy(rider->payload + (0x23c - 0xe4), &clear, 4);
    memcpy(rider->opaque1, &context->pool->origin, 12);
    memcpy(rider->opaque1 + 12, &context->pool->origin, 12);
    memcpy(rider->opaque1 + 24, &context->pool->origin, 12);
    return 1;
}
int bh_rail_wagon_set_rider(BHEntity *e, BHEntity *rider, BHRailSegment *checkpoint)
{
    BHRailWagonContext *c = owner(e);
    if (!c || (rider && !member(c, rider)) || (checkpoint && !segment(c, checkpoint)))
        return 0;
    state(e)->rider = rider;
    state(e)->checkpoint = checkpoint;
    private_data(e)->rider_generation = rider ? rider->generation : 0;
    return 1;
}
void bh_rail_wagon_draw(BHEntity *e)
{
    BHRailWagonContext *context = owner(e);
    BHRailWagonState *rail_wagon_state;
    BHVec3 point, delta, velocity;
    float pitch;
    double inv;
    BHRailWagonPose *rail_wagon_pose;
    if (!context)
        return;
    rail_wagon_state = state(e);
    if (!ready(context, rail_wagon_state))
        return;
    ++context->draws;
    rail_wagon_pose = bh_rail_wagon_pose(e);
    draw_point(rail_wagon_state, &point, &delta);
    if (bh_draw_submit_resource(context->queue, 0, context->model, &point, &rail_wagon_pose->angles, 1) < 0)
        failure(context, BH_WAGON_DRAW);
    inv = 1.0 / rail_wagon_state->duration;
    bh_round_float(&velocity.x, (double)delta.x * inv);
    bh_round_float(&velocity.y, (double)delta.y * inv);
    bh_round_float(&velocity.z, (double)delta.z * inv);
    bh_round_float(&pitch, (double)rail_wagon_state->speed * 0.02);
    rail_wagon_state->sound_cooldown = 2;
    if (rail_wagon_state->sound_instance == BH_SOUND_INVALID)
        rail_wagon_state->sound_instance = bh_sound_request(context->sound, rail_wagon_state->sound_resource, 1, pitch, &point, &velocity);
    else if (bh_sound_update(context->sound, rail_wagon_state->sound_instance, 1, pitch, &point, &velocity) < 0)
        failure(context, BH_WAGON_SOUND);
}
void bh_rail_wagon_tick(BHEntity *entity)
{
    BHRailWagonContext *context = owner(entity);
    BHRailWagonState *rail_wagon_state;
    BHVec3 delta;
    double elapsed, speed;
    unsigned long steps = 0;
    if (!context)
        return;
    rail_wagon_state = state(entity);
    if (!ready(context, rail_wagon_state))
        return;
    ++context->steps;
    elapsed = (double)rail_wagon_state->elapsed - (-1.0 / 30.0);
    bh_round_float(&rail_wagon_state->elapsed, elapsed);
    if (!_isnan(elapsed) && elapsed >= rail_wagon_state->duration)
        do
        {
            cooldown(context, rail_wagon_state);
            bh_round_float(&rail_wagon_state->elapsed, (double)rail_wagon_state->elapsed - rail_wagon_state->duration);
            rail_wagon_state->segment = walk(context, rail_wagon_state->segment, rail_wagon_state->direction);
            if (!rail_wagon_state->segment)
                return;
            direction(rail_wagon_state, &delta);
            entity->position = rail_wagon_state->direction ? rail_wagon_state->segment->start : rail_wagon_state->segment->end;
            speed = (double)rail_wagon_state->speed - (double)rail_wagon_state->segment->rise * (rail_wagon_state->direction ? -0.8 : 0.8);
            bh_round_float(&rail_wagon_state->speed, speed);
            if (_isnan(speed) || speed < 30)
                rail_wagon_state->speed = 30;
            bh_round_float(&rail_wagon_state->duration, (double)rail_wagon_state->segment->length / rail_wagon_state->speed);
            if (rail_wagon_state->segment == rail_wagon_state->checkpoint)
            {
                if (rail_wagon_state->rider)
                    rail_wagon_state->rider = NULL;
                else
                    rail_wagon_state->checkpoint = NULL;
            }
            if (!migrate(context, entity, &entity->position))
                return;
            align(entity, &delta);
            ++context->transitions;
            if (++steps > 20000)
            {
                failure(context, BH_WAGON_TOPOLOGY);
                return;
            }
        } while (rail_wagon_state->elapsed >= rail_wagon_state->duration);
    rider_update(context, entity);
}
void bh_rail_wagon_follow(BHEntity *e)
{
    BHRailWagonContext *context = owner(e);
    BHRailWagonState *rail_wagon_state, *leader;
    BHEntity *parent;
    BHRailSegment *rail;
    BHVec3 delta;
    double elapsed, distance;
    unsigned long n = 0;
    if (!context)
        return;
    rail_wagon_state = state(e);
    if (!ready(context, rail_wagon_state))
        return;
    ++context->follows;
    parent = rail_wagon_state->leader;
    if (!wagon(context, parent) || !live(context, parent, private_data(e)->leader_generation))
    {
        failure(context, BH_WAGON_LIFETIME);
        return;
    }
    leader = state(parent);
    if (!ready(context, leader))
        return;
    elapsed = (double)rail_wagon_state->elapsed - (-1.0 / 30.0);
    bh_round_float(&rail_wagon_state->elapsed, elapsed);
    if (!_isnan(elapsed) && elapsed < rail_wagon_state->duration && rail_wagon_state->speed == leader->speed)
        return;
    rail = leader->segment;
    distance = ((double)leader->elapsed * rail->length) / leader->duration - rail_wagon_state->spacing;
    while (_isnan(distance) || distance < 0)
    {
        rail = walk(context, rail, !rail_wagon_state->direction);
        if (!rail)
            return;
        distance += rail->length;
        if (++n > 20000)
        {
            failure(context, BH_WAGON_TOPOLOGY);
            return;
        }
    }
    rail_wagon_state->speed = leader->speed;
    bh_round_float(&rail_wagon_state->duration, (double)rail->length / rail_wagon_state->speed);
    bh_round_float(&rail_wagon_state->elapsed, distance / rail_wagon_state->speed);
    if (rail_wagon_state->segment != rail)
    {
        if (!migrate(context, e, &rail->start))
            return;
        rail_wagon_state->segment = rail;
        cooldown(context, rail_wagon_state);
        direction(rail_wagon_state, &delta);
        align(e, &delta);
    }
}
void bh_rail_wagon_query(BHEntity *entity, void *packet)
{
    BHRailWagonContext *context = owner(entity);
    BHRailWagonState *rail_wagon_state, *other;
    long event;
    BHEntity *target;
    BHRailSegment *rail;
    double distance;
    unsigned long i;
    BHVec3 point;
    float ratio;
    if (!context || !packet)
        return;
    rail_wagon_state = state(entity);
    ++context->queries;
    memcpy(&event, packet, 4);
    if (event == 4 || event == 20)
    {
        if (!unlink_room(context, entity))
            return;
        bh_sound_stop(context->sound, rail_wagon_state->sound_instance);
        ++context->removed;
        bh_entity_release(context->pool, entity);
        return;
    }
    if (!ready(context, rail_wagon_state))
        return;
    if (event == 3)
    {
        bh_round_float(&ratio, (double)rail_wagon_state->elapsed / rail_wagon_state->duration);
        interpolate(rail_wagon_state->segment, rail_wagon_state->direction, ratio, &point, NULL);
        bh_round_float(&point.x, (double)point.x - (double)bh_rail_wagon_pose(entity)->up.x * 5.0);
        bh_round_float(&point.y, (double)point.y - (double)bh_rail_wagon_pose(entity)->up.y * 5.0);
        bh_round_float(&point.z, (double)point.z - (double)bh_rail_wagon_pose(entity)->up.z * 5.0);
        memcpy((unsigned char *)packet + 4, &point, 12);
        return;
    }
    if (event != 19)
        return;
    memcpy(&target, (unsigned char *)packet + 4, 4);
    if (!wagon(context, target))
    {
        failure(context, BH_WAGON_LIFETIME);
        return;
    }
    other = state(target);
    if (!ready(context, other) || other->direction != rail_wagon_state->direction)
        return;
    rail = rail_wagon_state->segment;
    distance = ((double)other->elapsed * other->segment->length) / other->duration -
               ((double)rail_wagon_state->elapsed * rail->length) / rail_wagon_state->duration;
    for (i = 0; i < 10; ++i)
    {
        rail = walk(context, rail, !rail_wagon_state->direction);
        if (!rail)
            return;
        distance -= rail->length;
    }
    for (i = 0; rail != other->segment; ++i)
    {
        distance += rail->length;
        rail = walk(context, rail, rail_wagon_state->direction);
        if (!rail)
            return;
        if (i >= 20)
            return;
    }
    if (_isnan(distance) || distance < 0)
    {
        distance = -distance;
        if (rail_wagon_state->leader != entity && (_isnan(distance) || distance < rail_wagon_state->spacing))
            return;
        bh_round_float(&rail_wagon_state->spacing, distance);
        rail_wagon_state->leader = target;
        private_data(entity)->leader_generation = target->generation;
        entity->step = bh_rail_wagon_follow;
    }
    else
    {
        if (other->leader != target && distance < other->spacing)
            return;
        bh_round_float(&other->spacing, distance);
        other->leader = entity;
        private_data(target)->leader_generation = entity->generation;
        target->step = bh_rail_wagon_follow;
    }
    ++context->couples;
}
BHEntity *bh_rail_wagon_spawn(BHRailWagonContext *context, const BHVec3 *position, const BHVec3 *angles)
{
    BHWorldGeometry *w;
    BHEntity *entity;
    BHRailWagonState *rail_wagon_state;
    BHRailWagonPose *rail_wagon_pose;
    BHMat34 m;
    BHVec3 delta, offset, unit;
    long i, best = -1;
    float chosen = 0, closest = 9e37f, len, stored_projection;
    double dx, dy, projection, ratio, distance;
    unsigned long generation;
    struct
    {
        long event;
        BHEntity *entity;
    } packet;
    if (!context || !context->rails || !context->rails->segments || context->rails->count < 1 || context->rails->count > 200 || !context->pool ||
        !context->first_world || !context->world_count || !context->rooms || !context->room_count || !context->queue || !context->model || !context->sound ||
        !position || !angles || !bh_math_environment_supported())
    {
        failure(context, BH_WAGON_ARGUMENT);
        return NULL;
    }
    w = bh_world_locate(context->first_world, context->world_count, NULL, position, 0);
    if (!w)
        return NULL;
    if (!room(context, w) || (room(context, w)->actors && !member(context, room(context, w)->actors)))
    {
        failure(context, BH_WAGON_WORLD);
        return NULL;
    }
    for (i = 0; i < context->rails->count; ++i)
    {
        BHRailSegment *rail_segment = context->rails->segments + i;
        dx = (double)rail_segment->end.x - rail_segment->start.x;
        dy = (double)rail_segment->end.y - rail_segment->start.y;
        difference(&rail_segment->end, &rail_segment->start, &delta);
        difference(position, &rail_segment->start, &offset);
        bh_round_float(&len, (dx * delta.x + dy * delta.y) + (double)delta.z * delta.z);
        if (len <= 0 || !_finite(len))
            continue;
        projection = ((double)delta.x * offset.x + (double)delta.y * offset.y + (double)delta.z * offset.z) / len;
        bh_round_float(&stored_projection, projection);
        if (projection < 0 || stored_projection > 1 || !_finite(projection))
            continue;
        ratio =
            sqrt(((double)offset.x * offset.x + (double)offset.y * offset.y) + (double)offset.z * offset.z) / sqrt(len);
        distance = sqrt(ratio * ratio - (double)stored_projection * stored_projection);
        if (distance < closest)
        {
            bh_round_float(&closest, distance);
            chosen = stored_projection;
            best = i;
        }
    }
    if (best < 0)
        return NULL;
    entity = bh_entity_acquire(context->pool);
    if (!entity)
    {
        failure(context, BH_WAGON_POOL);
        return NULL;
    }
    rail_wagon_state = state(entity);
    rail_wagon_pose = bh_rail_wagon_pose(entity);
    private_data(entity)->context = context;
    private_data(entity)->leader_generation = entity->generation;
    private_data(entity)->rider_generation = 0;
    entity->position = *position;
    rail_wagon_pose->angles = *angles;
    entity->flags |= 0x100;
    entity->step = bh_rail_wagon_tick;
    rail_wagon_state->elapsed = chosen;
    rail_wagon_state->duration = 1;
    rail_wagon_state->speed = 30;
    rail_wagon_state->draw = bh_rail_wagon_draw;
    rail_wagon_state->sound_instance = BH_SOUND_INVALID;
    rail_wagon_state->sound_resource = context->sound_resource;
    rail_wagon_state->kind = 9;
    rail_wagon_state->subtype = 0;
    rail_wagon_state->sound_cooldown = 0;
    rail_wagon_state->secondary_instance = BH_SOUND_INVALID;
    rail_wagon_state->leader = entity;
    rail_wagon_state->rider = NULL;
    rail_wagon_state->checkpoint = NULL;
    prepend(context, entity, w);
    memset(&m, 0, sizeof(m));
    m.m[0][0] = m.m[1][1] = m.m[2][2] = 1;
    bh_mat34_rotate_y(angles->x, &m);
    bh_mat34_rotate_x(angles->y, &m);
    bh_mat34_rotate_z(angles->z, &m);
    unit.x = 1;
    unit.y = unit.z = 0;
    bh_mat34_transform_direction(&m, &unit, &rail_wagon_pose->right);
    unit.x = 0;
    unit.y = 1;
    bh_mat34_transform_direction(&m, &unit, &rail_wagon_pose->up);
    unit.y = 0;
    unit.z = 1;
    bh_mat34_transform_direction(&m, &unit, &rail_wagon_pose->forward);
    rail_wagon_state->segment = context->rails->segments + best;
    projection = (((double)rail_wagon_state->segment->end.y - rail_wagon_state->segment->start.y) * rail_wagon_pose->forward.y +
                  ((double)rail_wagon_state->segment->end.z - rail_wagon_state->segment->start.z) * rail_wagon_pose->forward.z) +
                 ((double)rail_wagon_state->segment->end.x - rail_wagon_state->segment->start.x) * rail_wagon_pose->forward.x;
    rail_wagon_state->direction = !_isnan(projection) && projection >= 0;
    if (!rail_wagon_state->direction)
        bh_round_float(&rail_wagon_state->elapsed, 1.0 - rail_wagon_state->elapsed);
    generation = entity->generation;
    packet.event = 19;
    packet.entity = entity;
    bh_entities_query(context->pool, 0, 0, NULL, 0, &packet);
    if (!live(context, entity, generation))
    {
        failure(context, BH_WAGON_LIFETIME);
        return NULL;
    }
    bh_entity_set_query(context->pool, entity, bh_rail_wagon_query);
    ++context->spawned;
    return entity;
}
