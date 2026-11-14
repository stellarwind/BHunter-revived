#include "bh_menu_present.h"
#include "../../Math/bh_rotation.h"
#include "../../Math/bh_round.h"
#include <math.h>
#include <string.h>
int bh_menu_light_queue(BHDrawQueue *queue, unsigned long bucket, long enabled, float x, float y, float z, float direct,
                        float ambient, unsigned long dc, unsigned long ac)
{
    static const unsigned long map[9] = {1, 2, 3, 4, 5, 0, 6, 7, 8};
    unsigned long h, t, i;
    BHDrawCommand *command;
    float values[5];
    if (!queue || !queue->commands || queue->count < 9 || queue->count > queue->capacity)
    {
        if (queue)
            queue->error = BH_DRAW_ARGUMENT;
        return -1;
    }
    if (queue->count == queue->capacity)
    {
        queue->error = BH_DRAW_FULL;
        return 0;
    }
    h = bucket < 9 ? map[bucket] : 0;
    t = queue->commands[h].resource;
    if (t >= queue->count || (queue->commands[t].next != BH_DRAW_END && queue->commands[t].next >= queue->count))
    {
        queue->error = BH_DRAW_TOPOLOGY;
        return -1;
    }
    i = queue->count++;
    command = queue->commands + i;
    command->next = queue->commands[t].next;
    queue->commands[t].next = i;
    queue->commands[h].resource = i;
    command->kind = BH_DRAW_LIGHT;
    command->resource = (unsigned long)enabled;
    command->words[0] = dc;
    command->words[1] = ac;
    values[0] = x;
    values[1] = y;
    values[2] = z;
    values[3] = direct;
    values[4] = ambient;
    memcpy(command->words + 2, values, 20);
    queue->error = BH_DRAW_OK;
    return 1;
}
int bh_menu_light_execute(BHLightConfig *light, const BHDrawCommand *command, unsigned long *next)
{
    float f[5];
    int ok;
    if (!light || !command || !next || command->kind != BH_DRAW_LIGHT)
        return 0;
    memcpy(f, command->words + 2, 20);
    ok = bh_light_configure(light, (long)command->resource, f[0], f[1], f[2], f[3], f[4], command->words[0], command->words[1]);
    *next = command->next;
    return ok;
}
static void record(BHMenuPresent *menu_present, int status)
{
    if (status > 0)
        ++menu_present->queued;
    else
    {
        ++menu_present->dropped;
        if (status < 0)
            menu_present->error = 1;
    }
}
static float rf(double x)
{
    float out;
    bh_round_float(&out, x);
    return out;
}
static void light(BHMenuPresent *menu_present, const BHVec3 *vector, int selected)
{
    record(menu_present, bh_menu_light_queue(menu_present->queue, 0, 1, vector->x, vector->y, vector->z, 1, .4f, selected ? 0xff8080 : 0x80ffa0,
                                  selected ? 0xff8080 : 0xff80a0));
}
int bh_menu_present(BHMenuPresent *menu_present)
{
    BHMat34 basis, m;
    BHVec3 x = {1, 0, 0}, y = {0, 1, 0}, z = {0, 0, 1}, right, up, forward, ld, base, axis, point, normal, cross, pose;
    double a, s, c, scale, v;
    float theta, brightness;
    unsigned long i;
    if (!menu_present)
        return 0;
    menu_present->queued = menu_present->dropped = 0;
    menu_present->error = 0;
    if (!menu_present->eye || menu_present->application_mode >= 1)
        return 1;
    if (!menu_present->angles || !menu_present->queue || (menu_present->row_count && !menu_present->rows) || menu_present->row_count > 4096)
        return 0;
    memset(&basis, 0, sizeof(basis));
    basis.m[0][0] = basis.m[1][1] = basis.m[2][2] = 1;
    a = rf((menu_present->phase & 127UL) * 0.04908738521234052);
    bh_mat34_rotate_y(menu_present->angles->x, &basis);
    bh_mat34_rotate_x(menu_present->angles->y, &basis);
    bh_mat34_rotate_z(menu_present->angles->z, &basis);
    bh_mat34_transform_direction(&basis, &x, &right);
    bh_mat34_transform_direction(&basis, &y, &up);
    bh_mat34_transform_direction(&basis, &z, &forward);
    s = sin(a);
    c = cos(a);
    ld.x = rf(up.x * s + right.x * c + forward.x);
    ld.y = rf(up.y * s + right.y * c + forward.y);
    ld.z = rf(up.z * s + forward.z + right.z * c);
    base.x = rf(menu_present->distance * forward.x + menu_present->eye->x);
    base.y = rf(menu_present->eye->y + menu_present->distance * forward.y);
    base.z = rf(menu_present->distance * forward.z + menu_present->eye->z);
    light(menu_present, &ld, 0);
    theta = rf(menu_present->display_selection * 0.39269908169872414);
    bh_mat34_rotate_x(rf(-menu_present->display_selection * 0.39269908169872414), &basis);
    brightness = rf(.9 - (sin(a * 13.0) + cos(a * 17.0)) * (-.04));
    for (i = 0; i < menu_present->row_count; ++i)
    {
        if (!menu_present->rows[i].row)
        {
            menu_present->error = 1;
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
        if ((float)i == menu_present->display_selection)
        {
            point.x = rf(forward.x * (-menu_present->push) + point.x);
            point.y = rf(forward.y * (-menu_present->push) + point.y);
            point.z = rf(forward.z * (-menu_present->push) + point.z);
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
        record(menu_present, bh_quad_queue(menu_present->queue, 7, menu_present->rows[i].image_handle, &point, &cross, &normal, 1, .4f, 0, 0,
                                i == (unsigned long)menu_present->selection ? brightness : rf(brightness * .4), 0, 1, 0, .5f));
        if (menu_present->capture && i == (unsigned long)menu_present->selection)
        {
            light(menu_present, &ld, 1);
            record(menu_present, bh_draw_submit_resource(menu_present->queue, 0, menu_present->rows[i].row->identity, &point, &pose, 1));
            light(menu_present, &ld, 0);
        }
        else
            record(menu_present, bh_draw_submit_resource(menu_present->queue, 0, menu_present->rows[i].row->identity, &point, &pose, 1));
        record(menu_present, bh_draw_submit_resource(menu_present->queue, 3, menu_present->frame_resource, &point, &pose, 1));
        bh_mat34_rotate_x(.3926991f, &basis);
        theta = rf(theta - 0.39269908169872414);
    }
    record(menu_present, bh_menu_light_queue(menu_present->queue, 0, 0, 0, 0, 0, 0, 0, 0, 0));
    return !menu_present->error;
}
