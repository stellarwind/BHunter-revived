#include "bh_flyby_camera.h"
#include "../../Graphics/Textures/bh_atlas_text.h"
#include "../../Math/bh_rotation.h"
#include "../../Math/bh_round.h"
#include "bh_flyby_credits.h"
#include <float.h>
#include <math.h>
#include <string.h>
static float stored(double v)
{
    float out;
    bh_round_float(&out, v);
    return out;
}
static int finite_vector(const BHVec3 *vector)
{
    return _finite(vector->x) && _finite(vector->y) && _finite(vector->z);
}
static BHVec3 orbit(const BHPlayerCameraStart *player_camera_start, double t)
{
    BHVec3 out;
    float radius, height;
    double oscillation;
    memcpy(&radius, player_camera_start->metadata, 4);
    memcpy(&height, player_camera_start->metadata + 1, 4);
    oscillation = (cos(1 - t * (-.037)) * sin(t * .071)) * cos(t * .217);
    out.x = stored((double)player_camera_start->position.x + (double)radius * oscillation);
    oscillation = (sin(1 - t * (-.087)) * cos(t * .041)) * cos(t * .173);
    out.y = stored((double)height * oscillation + player_camera_start->position.y);
    oscillation = (sin(1 - t * (-.057)) * cos(t * .091)) * cos(t * .13);
    out.z = stored((double)player_camera_start->position.z + (double)radius * oscillation);
    return out;
}
static int plan(BHFlybyCameraContext *context, BHFlybyCameraState *flyby_camera_state)
{
    BHRoutePlan p;
    if (!bh_route_plan(context->route->navigation, context->route->first, context->route->scene_count, flyby_camera_state->route.current,
                       &context->starts[context->route->selected_waypoint].position, &context->starts[flyby_camera_state->route.waypoint_index].position,
                       &p))
        return 0;
    flyby_camera_state->route.next = p.next;
    flyby_camera_state->route.curve[0] = p.goal;
    flyby_camera_state->route.curve[1] = p.portal_center;
    flyby_camera_state->route.curve[2] = p.projected;
    flyby_camera_state->route.curve[3] = p.anchor;
    flyby_camera_state->route.parameter_rate = p.parameter_rate;
    return 1;
}
static void combine(BHVec3 *out, const BHVec3 *first_vector, float a_weight, const BHVec3 *second_vector, float b_weight)
{
    BHVec3 scaled;
    scaled.x = stored((double)second_vector->x * b_weight);
    scaled.y = stored((double)second_vector->y * b_weight);
    scaled.z = stored((double)second_vector->z * b_weight);
    out->x = stored((double)first_vector->x * a_weight + scaled.x);
    out->y = stored((double)first_vector->y * a_weight + scaled.y);
    out->z = stored((double)first_vector->z * a_weight + scaled.z);
}
static int publish(BHFlybyCameraContext *context, unsigned long index)
{
    const char *cursor = flyby_credits[index];
    char line[128];
    unsigned long n;
    short y = 1;
    long width, x;
    int result = 1, r;
    r = bh_text_queue_clear(context->text, context->credit_font, 0);
    if (r < 0)
        return -1;
    if (!r)
        result = 0;
    while (*cursor)
    {
        n = 0;
        while (*cursor && *cursor != '\n')
        {
            if (n >= 127)
                return -1;
            line[n++] = *cursor++;
        }
        if (*cursor)
            ++cursor;
        line[n] = 0;
        if (!n)
            y = (short)(y + 7);
        else
        {
            width = bh_atlas_page_text_width(context->credit_font, (const unsigned char *)line);
            x = 64 - (short)(width / 2);
            r = bh_text_queue_text(context->text, context->credit_font, line, x, y);
            if (r < 0)
                return -1;
            if (!r)
                result = 0;
            y = (short)(y + 14);
        }
    }
    return result;
}
static double wrap_yaw(float value)
{
    const double pi = 3.141592653589793;
    if ((double)value >= pi)
        return fmod((double)value + pi, 2 * pi) - pi;
    if ((double)value < -pi)
        return fmod((double)value - pi, 2 * pi) + pi;
    return value;
}
static int credits(BHFlybyCameraContext *context, const BHFlybyCameraState *flyby_camera_state)
{
    double yaw;
    int trigger, arm, r;
    if (flyby_camera_state->route.waypoint_index == 13)
    {
        yaw = wrap_yaw(flyby_camera_state->camera.angles.x);
        trigger = -1.4353981633974482 < yaw && yaw < -.13539816339744826;
        arm = 1.956194490192345 < yaw && yaw < 2.7561944901923447;
        if (arm)
            context->credits->gate = 1;
        if (trigger && context->credits->gate == 1)
        {
            context->credits->gate = 0;
            r = publish(context, context->credits->index);
            if (++context->credits->index > 14)
                context->credits->index = 0;
            return r;
        }
    }
    else if (context->credits->gate != -1)
    {
        context->credits->gate = -1;
        r = publish(context, 0);
        context->credits->index = 1;
        return r;
    }
    return 1;
}
static int failure(BHFlybyCameraContext *context, const char *stage)
{
    if (context)
        context->last_failure = stage;
    return -1;
}
int bh_flyby_camera_step(BHFlybyCameraContext *context, BHFlybyCameraState *flyby_camera_state)
{
    BHVec3 desired, curve;
    BHCameraInput input;
    BHMat34 rotation = {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}}};
    BHVec3 axis;
    double dx, dz, old_dx, old_dz, length, old_length, turn = 0, t, advanced;
    float phase, u, amount, rounded_dx, rounded_dz, rounded_old_dx, rounded_old_dz;
    long mode;
    unsigned long selected;
    float radius, height;
    int result;
    if (!context || !flyby_camera_state || !context->route || !context->camera || !context->starts || !context->text || !context->credit_font || !context->credit_font->atlas ||
        !context->credits || !context->start_count || context->credits->index >= 15 || flyby_camera_state->route.waypoint_index >= context->start_count ||
        context->route->selected_waypoint >= context->start_count || context->route->selected_waypoint >= context->route->waypoint_count ||
        !context->route->waypoints || !_finite(flyby_camera_state->orbit_time) || !finite_vector(&flyby_camera_state->position) ||
        !finite_vector(&flyby_camera_state->previous_position) || !finite_vector(&flyby_camera_state->older_position) || !_finite(flyby_camera_state->route.phase) ||
        flyby_camera_state->route.phase < 0 || flyby_camera_state->route.phase > 3 ||
        (flyby_camera_state->route.phase > 0 && (!_finite(flyby_camera_state->route.parameter_rate) || flyby_camera_state->route.parameter_rate <= 0)))
        return failure(context, "validate camera and route state");
    selected = context->route->selected_waypoint;
    if (!finite_vector(&context->starts[flyby_camera_state->route.waypoint_index].position) ||
        memcmp(&context->starts[selected].position, context->route->waypoints + selected, 12))
        return failure(context, "validate waypoint positions");
    memcpy(&radius, context->starts[flyby_camera_state->route.waypoint_index].metadata, 4);
    memcpy(&height, context->starts[flyby_camera_state->route.waypoint_index].metadata + 1, 4);
    if (!_finite(radius) || !_finite(height))
        return failure(context, "validate orbit metadata");
    flyby_camera_state->orbit_time -= -0.03333333333333333;
    if (flyby_camera_state->route.waypoint_index == 13)
        flyby_camera_state->orbit_time -= -0.016666666666666666;
    t = flyby_camera_state->orbit_time;
    desired = orbit(context->starts + flyby_camera_state->route.waypoint_index, t);
    phase = flyby_camera_state->route.phase;
    if (phase == 0)
    {
        if (flyby_camera_state->route.waypoint_index != selected)
        {
            if (!plan(context, flyby_camera_state))
                return failure(context, "plan route");
            flyby_camera_state->route.phase = stored((double)flyby_camera_state->route.phase + flyby_camera_state->route.parameter_rate);
            if (!bh_camera_state_set_mode(&flyby_camera_state->camera, 10))
                return failure(context, "enter travel camera mode");
        }
    }
    else if (phase > 2)
    {
        u = stored((double)phase - 2);
        bh_camera_curve(u, &curve, flyby_camera_state->route.curve, flyby_camera_state->route.curve + 1, flyby_camera_state->route.curve + 2, flyby_camera_state->route.curve + 3);
        combine(&desired, &desired, u, &curve, stored(1.0 - u));
        advanced = (double)phase + flyby_camera_state->route.parameter_rate;
        flyby_camera_state->route.phase = stored(advanced);
        if (advanced > 3)
        {
            mode = (selected == 14 || selected == 15 || selected == 16) ? 8 : selected == 13 ? 11 : 10;
            if (!bh_camera_state_set_mode(&flyby_camera_state->camera, mode))
                return failure(context, "enter destination camera mode");
            flyby_camera_state->route.phase = 0;
        }
    }
    else if (phase > 1)
    {
        u = stored((double)phase - 1);
        bh_camera_curve(u, &desired, flyby_camera_state->route.curve, flyby_camera_state->route.curve + 1, flyby_camera_state->route.curve + 2, flyby_camera_state->route.curve + 3);
        advanced = (double)phase + flyby_camera_state->route.parameter_rate;
        flyby_camera_state->route.phase = stored(advanced);
        if (advanced > 2 && !bh_camera_route_advance(context->route, &flyby_camera_state->route))
            return failure(context, "advance portal route");
    }
    else
    {
        bh_camera_curve(phase, &curve, flyby_camera_state->route.curve, flyby_camera_state->route.curve + 1, flyby_camera_state->route.curve + 2, flyby_camera_state->route.curve + 3);
        amount = stored(1.0 - phase);
        combine(&desired, &desired, amount, &curve, phase);
        advanced = (double)phase + flyby_camera_state->route.parameter_rate;
        flyby_camera_state->route.phase = stored(advanced);
        if (advanced > 1 && !bh_camera_route_advance(context->route, &flyby_camera_state->route))
            return failure(context, "enter first portal");
    }
    flyby_camera_state->older_position = flyby_camera_state->previous_position;
    flyby_camera_state->previous_position = flyby_camera_state->position;
    flyby_camera_state->position.x = stored((double)flyby_camera_state->position.x * .6 - (double)desired.x * (-.4));
    flyby_camera_state->position.y = stored((double)flyby_camera_state->position.y * .6 - (double)desired.y * (-.4));
    flyby_camera_state->position.z = stored((double)flyby_camera_state->position.z * .6 - (double)desired.z * (-.4));
    dx = (double)flyby_camera_state->position.x - flyby_camera_state->previous_position.x;
    dz = (double)flyby_camera_state->position.z - flyby_camera_state->previous_position.z;
    rounded_dx = stored(dx);
    rounded_dz = stored(dz);
    length = sqrt(dx * rounded_dx + dz * rounded_dz);
    old_dx = (double)flyby_camera_state->previous_position.x - flyby_camera_state->older_position.x;
    old_dz = (double)flyby_camera_state->previous_position.z - flyby_camera_state->older_position.z;
    if (length != 0)
        turn = (old_dz * rounded_dx - old_dx * rounded_dz) / length;
    rounded_old_dx = stored(old_dx);
    rounded_old_dz = stored(old_dz);
    old_length = sqrt((double)rounded_old_dx * rounded_old_dx + (double)rounded_old_dz * rounded_old_dz);
    if (old_length != 0)
        turn /= old_length;
    else
        turn = 0;
    flyby_camera_state->angles.x = stored(atan2(rounded_dx, rounded_dz));
    flyby_camera_state->angles.z = stored(turn * 7.853981633974483);
    flyby_camera_state->angles.y = stored(((double)flyby_camera_state->previous_position.y - flyby_camera_state->position.y) * .5);
    flyby_camera_state->velocity.x = stored((double)stored(dx) * 30);
    flyby_camera_state->velocity.y = stored((double)stored((double)flyby_camera_state->position.y - flyby_camera_state->previous_position.y) * 30);
    flyby_camera_state->velocity.z = stored((double)stored(dz) * 30);
    bh_mat34_rotate_y(flyby_camera_state->angles.x, &rotation);
    bh_mat34_rotate_x(flyby_camera_state->angles.y, &rotation);
    bh_mat34_rotate_z(flyby_camera_state->angles.z, &rotation);
    axis.x = 1;
    axis.y = axis.z = 0;
    bh_mat34_transform_direction(&rotation, &axis, &flyby_camera_state->basis_x);
    axis.y = 1;
    axis.x = 0;
    bh_mat34_transform_direction(&rotation, &axis, &flyby_camera_state->basis_y);
    axis.z = 1;
    axis.y = 0;
    bh_mat34_transform_direction(&rotation, &axis, &flyby_camera_state->basis_z);
    input.target = flyby_camera_state->position;
    input.angles = flyby_camera_state->angles;
    input.basis_x = flyby_camera_state->basis_x;
    input.basis_y = flyby_camera_state->basis_y;
    input.basis_z = flyby_camera_state->basis_z;
    input.actor_token = context->actor_token;
    if (!bh_camera_state_update(context->camera, &flyby_camera_state->camera, &input, 0.03333333333333333, 0, 0))
        return failure(context, "update camera pose");
    result = credits(context, flyby_camera_state);
    return result < 0 ? failure(context, "queue credits text") : result;
}
