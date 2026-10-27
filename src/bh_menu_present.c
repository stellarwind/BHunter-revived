#include "bh_menu_present.h"
#include "bh_rotation.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
int __cdecl bh_menu_light_queue(BHDrawQueue *q, unsigned long bucket, long enabled, float x, float y, float z,
                                float direct, float ambient, unsigned long dc, unsigned long ac)
{
    static const unsigned long map[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long h, t, i;
    BHDrawCommand *c;
    float values[5];
    if (!q || !q->commands || q->count < 9 || q->count > q->capacity)
    {
        if (q)
            q->error = BH_DRAW_ARGUMENT;
        return -1;
    }
    if (q->count == q->capacity)
    {
        q->error = BH_DRAW_FULL;
        return 0;
    }
    h = bucket < 9 ? map[bucket] : 0;
    t = q->commands[h].resource;
    if (t >= q->count || (q->commands[t].next != BH_DRAW_END && q->commands[t].next >= q->count))
    {
        q->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    i = q->count++;
    c = q->commands + i;
    c->next = q->commands[t].next;
    q->commands[t].next = i;
    q->commands[h].resource = i;
    c->kind = BH_DRAW_LIGHT;
    c->resource = (unsigned long)enabled;
    c->words[0] = dc;
    c->words[1] = ac;
    values[0] = x;
    values[1] = y;
    values[2] = z;
    values[3] = direct;
    values[4] = ambient;
    memcpy(c->words + 2, values, 20);
    q->error = BH_DRAW_OK;
    return 1;
}
int __cdecl bh_menu_light_execute(BHLightConfig *light, const BHDrawCommand *c, unsigned long *next)
{
    float f[5];
    int ok;
    if (!light || !c || !next || c->kind != BH_DRAW_LIGHT)
        return 0;
    memcpy(f, c->words + 2, 20);
    ok = bh_light_configure(light, (long)c->resource, f[0], f[1], f[2], f[3], f[4], c->words[0], c->words[1]);
    *next = c->next;
    return ok;
}
static void record(BHMenuPresent *p, int status)
{
    if (status > 0)
        ++p->queued;
    else
    {
        ++p->dropped;
        if (status < 0)
            p->error = 1;
    }
}
static float rf(double x)
{
    float out;
    bh_round_float(&out, x);
    return out;
}
static void light(BHMenuPresent *p, const BHVec3 *v, int selected)
{
    record(p, bh_menu_light_queue(p->queue, 0, 1, v->x, v->y, v->z, 1, .4f, selected ? 0xff8080 : 0x80ffa0,
                                  selected ? 0xff8080 : 0xff80a0));
}
int __cdecl bh_menu_present(BHMenuPresent *p)
{
    BHMat34 basis, m;
    BHVec3 x = {1, 0, 0}, y = {0, 1, 0}, z = {0, 0, 1}, right, up, forward, ld, base, axis, point, normal, cross, pose;
    double a, s, c, scale, v;
    float theta, brightness;
    unsigned long i;
    if (!p)
        return 0;
    p->queued = p->dropped = 0;
    p->error = 0;
    if (!p->eye || p->application_mode >= 1)
        return 1;
    if (!p->angles || !p->queue || (p->row_count && !p->rows) || p->row_count > 4096)
        return 0;
    memset(&basis, 0, sizeof(basis));
    basis.m[0][0] = basis.m[1][1] = basis.m[2][2] = 1;
    a = rf((p->phase & 127UL) * 0.04908738521234052);
    bh_mat34_rotate_y(p->angles->x, &basis);
    bh_mat34_rotate_x(p->angles->y, &basis);
    bh_mat34_rotate_z(p->angles->z, &basis);
    bh_mat34_transform_direction(&basis, &x, &right);
    bh_mat34_transform_direction(&basis, &y, &up);
    bh_mat34_transform_direction(&basis, &z, &forward);
    s = sin(a);
    c = cos(a);
    ld.x = rf(up.x * s + right.x * c + forward.x);
    ld.y = rf(up.y * s + right.y * c + forward.y);
    ld.z = rf(up.z * s + forward.z + right.z * c);
    base.x = rf(p->distance * forward.x + p->eye->x);
    base.y = rf(p->eye->y + p->distance * forward.y);
    base.z = rf(p->distance * forward.z + p->eye->z);
    light(p, &ld, 0);
    theta = rf(p->display_selection * 0.39269908169872414);
    bh_mat34_rotate_x(rf(-p->display_selection * 0.39269908169872414), &basis);
    brightness = rf(.9 - (sin(a * 13.0) + cos(a * 17.0)) * (-.04));
    for (i = 0; i < p->row_count; ++i)
    {
        if (!p->rows[i].row)
        {
            p->error = 1;
            break;
        }
        m = basis;
        bh_mat34_transform_direction(&m, &z, &axis);
        v = atan2(axis.x, axis.z);
        pose.x = rf(v);
        bh_mat34_rotate_world_y(rf(-v), &m);
        bh_mat34_transform_direction(&m, &z, &axis);
        v = atan2(-axis.y, axis.z);
        pose.y = rf(v);
        bh_mat34_rotate_world_x(rf(-v), &m);
        bh_mat34_transform_direction(&m, &x, &axis);
        pose.z = rf(atan2(axis.y, axis.x));
        c = cos(theta);
        scale = c * (-1.5);
        point.x = rf(base.x + forward.x * scale);
        point.y = rf(forward.y * scale + base.y);
        point.z = rf(base.z + forward.z * scale);
        if ((float)i == p->display_selection)
        {
            point.x = rf(forward.x * (-p->push) + point.x);
            point.y = rf(forward.y * (-p->push) + point.y);
            point.z = rf(forward.z * (-p->push) + point.z);
        }
        s = sin(theta);
        scale = s * (-1.5);
        point.x = rf(up.x * scale + point.x);
        point.y = rf(up.y * scale + point.y);
        point.z = rf(up.z * scale + point.z);
        normal.x = rf(forward.x * (-c) + up.x * (-s));
        normal.y = rf(forward.y * (-c) + up.y * (-s));
        normal.z = rf(forward.z * (-c) + up.z * (-s));
        bh_vec3_cross(&right, &normal, &cross);
        cross.x = -cross.x;
        cross.y = -cross.y;
        cross.z = -cross.z;
        record(p, bh_quad_queue(p->queue, 7, p->rows[i].image_handle, &point, &cross, &normal, 1, .4f, 0, 0,
                                i == (unsigned long)p->selection ? brightness : rf(brightness * .4), 0, 1, 0, .5f));
        if (p->capture && i == (unsigned long)p->selection)
        {
            light(p, &ld, 1);
            record(p, bh_draw_submit_resource(p->queue, 0, p->rows[i].row->identity, &point, &pose, 1));
            light(p, &ld, 0);
        }
        else
            record(p, bh_draw_submit_resource(p->queue, 0, p->rows[i].row->identity, &point, &pose, 1));
        record(p, bh_draw_submit_resource(p->queue, 3, p->frame_resource, &point, &pose, 1));
        bh_mat34_rotate_x(.3926991f, &basis);
        theta = rf(theta - 0.39269908169872414);
    }
    record(p, bh_menu_light_queue(p->queue, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    return !p->error;
}
