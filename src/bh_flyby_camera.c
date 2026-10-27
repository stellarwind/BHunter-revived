#include "bh_flyby_camera.h"
#include "bh_atlas_text.h"
#include "bh_rotation.h"
#include "bh_round.h"
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
static int finite_vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static BHVec3 orbit(const BHPlayerCameraStart *p, double t)
{
    BHVec3 out;
    float radius, height;
    double oscillation;
    memcpy(&radius, p->metadata, 4);
    memcpy(&height, p->metadata + 1, 4);
    oscillation = (cos(1 - t * (-.037)) * sin(t * .071)) * cos(t * .217);
    out.x = stored((double)p->position.x + (double)radius * oscillation);
    oscillation = (sin(1 - t * (-.087)) * cos(t * .041)) * cos(t * .173);
    out.y = stored((double)height * oscillation + p->position.y);
    oscillation = (sin(1 - t * (-.057)) * cos(t * .091)) * cos(t * .13);
    out.z = stored((double)p->position.z + (double)radius * oscillation);
    return out;
}
static int plan(BHFlybyCameraContext *c, BHFlybyCameraState *s)
{
    BHRoutePlan p;
    if (!bh_route_plan(c->route->navigation, c->route->first, c->route->scene_count, s->route.current,
                       &c->starts[c->route->selected_waypoint].position, &c->starts[s->route.waypoint_index].position,
                       &p))
        return 0;
    s->route.next = p.next;
    s->route.curve[0] = p.goal;
    s->route.curve[1] = p.portal_center;
    s->route.curve[2] = p.projected;
    s->route.curve[3] = p.anchor;
    s->route.parameter_rate = p.parameter_rate;
    return 1;
}
static void combine(BHVec3 *out, const BHVec3 *a, float a_weight, const BHVec3 *b, float b_weight)
{
    BHVec3 scaled;
    scaled.x = stored((double)b->x * b_weight);
    scaled.y = stored((double)b->y * b_weight);
    scaled.z = stored((double)b->z * b_weight);
    out->x = stored((double)a->x * a_weight + scaled.x);
    out->y = stored((double)a->y * a_weight + scaled.y);
    out->z = stored((double)a->z * a_weight + scaled.z);
}
static int publish(BHFlybyCameraContext *c, unsigned long index)
{
    const char *cursor = flyby_credits[index];
    char line[128];
    unsigned long n;
    short y = 1;
    long width, x;
    int result = 1, r;
    r = bh_text_queue_clear(c->text, c->credit_font, 0);
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
            width = bh_atlas_page_text_width(c->credit_font, (const unsigned char *)line);
            x = 64 - (short)(width / 2);
            r = bh_text_queue_text(c->text, c->credit_font, line, x, y);
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
static int credits(BHFlybyCameraContext *c, const BHFlybyCameraState *s)
{
    double yaw;
    int trigger, arm, r;
    if (s->route.waypoint_index == 13)
    {
        yaw = wrap_yaw(s->camera.angles.x);
        trigger = -1.4353981633974482 < yaw && yaw < -.13539816339744826;
        arm = 1.956194490192345 < yaw && yaw < 2.7561944901923447;
        if (arm)
            c->credits->gate = 1;
        if (trigger && c->credits->gate == 1)
        {
            c->credits->gate = 0;
            r = publish(c, c->credits->index);
            if (++c->credits->index > 14)
                c->credits->index = 0;
            return r;
        }
    }
    else if (c->credits->gate != -1)
    {
        c->credits->gate = -1;
        r = publish(c, 0);
        c->credits->index = 1;
        return r;
    }
    return 1;
}
int __cdecl bh_flyby_camera_step(BHFlybyCameraContext *c, BHFlybyCameraState *s)
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
    if (!c || !s || !c->route || !c->camera || !c->starts || !c->text || !c->credit_font || !c->credit_font->atlas ||
        !c->credits || !c->start_count || c->credits->index >= 15 || s->route.waypoint_index >= c->start_count ||
        c->route->selected_waypoint >= c->start_count || c->route->selected_waypoint >= c->route->waypoint_count ||
        !c->route->waypoints || !_finite(s->orbit_time) || !finite_vector(&s->position) ||
        !finite_vector(&s->previous_position) || !finite_vector(&s->older_position) || !_finite(s->route.phase) ||
        s->route.phase < 0 || s->route.phase > 3 ||
        (s->route.phase > 0 && (!_finite(s->route.parameter_rate) || s->route.parameter_rate <= 0)))
        return -1;
    selected = c->route->selected_waypoint;
    if (!finite_vector(&c->starts[s->route.waypoint_index].position) ||
        memcmp(&c->starts[selected].position, c->route->waypoints + selected, 12))
        return -1;
    memcpy(&radius, c->starts[s->route.waypoint_index].metadata, 4);
    memcpy(&height, c->starts[s->route.waypoint_index].metadata + 1, 4);
    if (!_finite(radius) || !_finite(height))
        return -1;
    s->orbit_time -= -0.03333333333333333;
    if (s->route.waypoint_index == 13)
        s->orbit_time -= -0.016666666666666666;
    t = s->orbit_time;
    desired = orbit(c->starts + s->route.waypoint_index, t);
    phase = s->route.phase;
    if (phase == 0)
    {
        if (s->route.waypoint_index != selected)
        {
            if (!plan(c, s))
                return -1;
            s->route.phase = stored((double)s->route.phase + s->route.parameter_rate);
            if (!bh_camera_state_set_mode(&s->camera, 10))
                return -1;
        }
    }
    else if (phase > 2)
    {
        u = stored((double)phase - 2);
        bh_camera_curve(u, &curve, s->route.curve, s->route.curve + 1, s->route.curve + 2, s->route.curve + 3);
        combine(&desired, &desired, u, &curve, stored(1.0 - u));
        advanced = (double)phase + s->route.parameter_rate;
        s->route.phase = stored(advanced);
        if (advanced > 3)
        {
            mode = (selected == 14 || selected == 15 || selected == 16) ? 8 : selected == 13 ? 11 : 10;
            if (!bh_camera_state_set_mode(&s->camera, mode))
                return -1;
            s->route.phase = 0;
        }
    }
    else if (phase > 1)
    {
        u = stored((double)phase - 1);
        bh_camera_curve(u, &desired, s->route.curve, s->route.curve + 1, s->route.curve + 2, s->route.curve + 3);
        advanced = (double)phase + s->route.parameter_rate;
        s->route.phase = stored(advanced);
        if (advanced > 2 && !bh_camera_route_advance(c->route, &s->route))
            return -1;
    }
    else
    {
        bh_camera_curve(phase, &curve, s->route.curve, s->route.curve + 1, s->route.curve + 2, s->route.curve + 3);
        amount = stored(1.0 - phase);
        combine(&desired, &desired, amount, &curve, phase);
        advanced = (double)phase + s->route.parameter_rate;
        s->route.phase = stored(advanced);
        if (advanced > 1 && !bh_camera_route_advance(c->route, &s->route))
            return -1;
    }
    s->older_position = s->previous_position;
    s->previous_position = s->position;
    s->position.x = stored((double)s->position.x * .6 - (double)desired.x * (-.4));
    s->position.y = stored((double)s->position.y * .6 - (double)desired.y * (-.4));
    s->position.z = stored((double)s->position.z * .6 - (double)desired.z * (-.4));
    dx = (double)s->position.x - s->previous_position.x;
    dz = (double)s->position.z - s->previous_position.z;
    rounded_dx = stored(dx);
    rounded_dz = stored(dz);
    length = sqrt(dx * rounded_dx + dz * rounded_dz);
    old_dx = (double)s->previous_position.x - s->older_position.x;
    old_dz = (double)s->previous_position.z - s->older_position.z;
    if (length != 0)
        turn = (old_dz * rounded_dx - old_dx * rounded_dz) / length;
    rounded_old_dx = stored(old_dx);
    rounded_old_dz = stored(old_dz);
    old_length = sqrt((double)rounded_old_dx * rounded_old_dx + (double)rounded_old_dz * rounded_old_dz);
    if (old_length != 0)
        turn /= old_length;
    else
        turn = 0;
    s->angles.x = stored(atan2(rounded_dx, rounded_dz));
    s->angles.z = stored(turn * 7.853981633974483);
    s->angles.y = stored(((double)s->previous_position.y - s->position.y) * .5);
    s->velocity.x = stored((double)stored(dx) * 30);
    s->velocity.y = stored((double)stored((double)s->position.y - s->previous_position.y) * 30);
    s->velocity.z = stored((double)stored(dz) * 30);
    bh_mat34_rotate_y(s->angles.x, &rotation);
    bh_mat34_rotate_x(s->angles.y, &rotation);
    bh_mat34_rotate_z(s->angles.z, &rotation);
    axis.x = 1;
    axis.y = axis.z = 0;
    bh_mat34_transform_direction(&rotation, &axis, &s->basis_x);
    axis.y = 1;
    axis.x = 0;
    bh_mat34_transform_direction(&rotation, &axis, &s->basis_y);
    axis.z = 1;
    axis.y = 0;
    bh_mat34_transform_direction(&rotation, &axis, &s->basis_z);
    input.target = s->position;
    input.angles = s->angles;
    input.basis_x = s->basis_x;
    input.basis_y = s->basis_y;
    input.basis_z = s->basis_z;
    input.actor_token = c->actor_token;
    if (!bh_camera_state_update(c->camera, &s->camera, &input, 0.03333333333333333, 0, 0))
        return -1;
    return credits(c, s);
}
