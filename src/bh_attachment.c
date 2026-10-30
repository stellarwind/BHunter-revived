#include "bh_attachment.h"
#include "bh_round.h"
#include <float.h>
#include <math.h>
#include <stddef.h>

typedef char attachment_navigation_first[offsetof(BHWorldGeometry, navigation) == 0 ? 1 : -1];
typedef char attachment_portal_stride[sizeof(BHPortal) == 60 ? 1 : -1];
typedef char attachment_neighbor_offset[offsetof(BHPortal, neighbor) == 56 ? 1 : -1];
typedef struct RadarEndpoint
{
    float x, z, radius;
} RadarEndpoint;

static BHAttachmentNode *neighbor(const BHPortal *p)
{
    /* navigation is the first member, including in every borrowed neighbor. */
    return (BHAttachmentNode *)p->neighbor;
}
void __cdecl bh_attachment_reset(BHAttachmentNode *node)
{
    unsigned long i;
    if (!node || !node->radar_visited)
        return;
    node->radar_visited = 0;
    for (i = 0; i < 6; ++i)
        if (node->navigation.portals[i].neighbor)
            bh_attachment_reset(neighbor(node->navigation.portals + i));
}
static int fail(BHAttachmentContext *c, BHAttachmentError error)
{
    if (c)
        c->error = error;
    return 0;
}
static int vector(const BHVec3 *v)
{
    return _finite(v->x) && _finite(v->y) && _finite(v->z);
}
static int endpoint(BHAttachmentContext *c, RadarEndpoint *out, double x, double z)
{
    double dx = x - c->player_position.x, dz = z - c->player_position.z, squared;
    bh_round_float(&out->x, dx);
    bh_round_float(&out->z, dz);
    squared = dx * out->x + dz * out->z;
    if (!_finite(squared) || squared < 0)
        return fail(c, BH_ATTACHMENT_NUMERICAL);
    bh_round_float(&out->radius, sqrt(squared));
    return _finite(out->x) && _finite(out->z) && _finite(out->radius) ? 1 : fail(c, BH_ATTACHMENT_NUMERICAL);
}
static int portal_endpoint(BHAttachmentContext *c, const BHPortal *p, RadarEndpoint *out)
{
    float u, v, plane;
    if (p->direction > 6 || p->direction == 3)
        return fail(c, BH_ATTACHMENT_PORTAL);
    if (!_finite(p->overlap[0]) || !_finite(p->overlap[1]) || !_finite(p->overlap[2]) || !_finite(p->overlap[3]))
        return fail(c, BH_ATTACHMENT_NUMERICAL);
    bh_round_float(&u, ((double)p->overlap[0] + p->overlap[1]) * .5);
    bh_round_float(&v, ((double)p->overlap[2] + p->overlap[3]) * .5);
    bh_round_float(&plane, (double)p->plane);
    switch (p->direction & 3UL)
    {
    case 0:
        return endpoint(c, out, plane, v);
    case 1:
        return endpoint(c, out, u, v);
    default:
        return endpoint(c, out, u, plane);
    }
}
/* The original clips by interpolating endpoint radial distances, rather than
   solving a segment/circle intersection. Keep this distinction and the stored
   binary32 weights. Updating a shared center here intentionally affects the
   following portal slots. */
static int radial_clip(RadarEndpoint *a, RadarEndpoint *b)
{
    float difference, wa, wb;
    double raw_wa, raw_wb;
    if (a->radius < 400)
    {
        if (b->radius > 400)
        {
            bh_round_float(&difference, (double)b->radius - a->radius);
            raw_wa = ((double)b->radius - 400) / difference;
            raw_wb = (400.0 - a->radius) / difference;
            bh_round_float(&wa, raw_wa);
            bh_round_float(&wb, raw_wb);
            b->radius = 400;
            bh_round_float(&b->x, raw_wb * b->x + raw_wa * a->x);
            bh_round_float(&b->z, (double)wb * b->z + (double)wa * a->z);
        }
        return 1;
    }
    if (b->radius >= 400)
        return 0;
    bh_round_float(&difference, (double)a->radius - b->radius);
    raw_wb = ((double)a->radius - 400) / difference;
    raw_wa = (400.0 - b->radius) / difference;
    bh_round_float(&wb, raw_wb);
    bh_round_float(&wa, raw_wa);
    a->radius = 400;
    bh_round_float(&a->x, raw_wa * a->x + raw_wb * b->x);
    bh_round_float(&a->z, (double)wa * a->z + (double)wb * b->z);
    return 1;
}
static int submit(BHAttachmentContext *c, int result)
{
    ++c->attempted;
    if (result > 0)
        ++c->queued;
    else if (!result)
        ++c->dropped;
    else
        return fail(c, BH_ATTACHMENT_QUEUE);
    return 1;
}
static int segment(BHAttachmentContext *c, unsigned long image, RadarEndpoint *a, RadarEndpoint *b, float height)
{
    const BHRadarFrame *f = c->frame;
    double signed_height = (double)height * .02, absolute_height, mx, mz, dx, dz, raw_radius, xx, zz;
    float fade, angle, mid_angle, delta_x, delta_z, length, radius, width, span, alpha, x, y, scale;
    BHVec3 center;
    int visible = radial_clip(a, b);
    absolute_height = signed_height < 0 ? -signed_height : signed_height;
    if (absolute_height > 1)
        absolute_height = 1;
    bh_round_float(&fade, 1.0 - absolute_height);
    if (fade == 0)
    {
        visible = 0;
        b->radius = 400;
    }
    if (!visible)
    {
        ++c->culled;
        return 1;
    }
    mx = ((double)b->x + a->x) * .5;
    mz = ((double)b->z + a->z) * .5;
    dx = (double)b->x - a->x;
    dz = (double)b->z - a->z;
    bh_round_float(&delta_x, dx);
    bh_round_float(&delta_z, dz);
    bh_round_float(&angle, atan2(dx, dz) - c->yaw);
    bh_round_float(&mid_angle, atan2(mx, mz) - c->yaw);
    bh_round_float(&length, sqrt(dx * delta_x + dz * delta_z));
    /* Original midpoint x is stored and reloaded for only one factor. */
    bh_round_float(&radius, mx);
    bh_round_float(&radius, sqrt(mx * radius + mz * mz));
    bh_round_float(&alpha, (double)fade * (1.0 - ((double)b->radius + a->radius) * .00125));
    if (!f->flat)
    {
        raw_radius = (double)radius * .00025;
        xx = raw_radius * sin((double)mid_angle);
        zz = raw_radius * cos((double)mid_angle);
        bh_round_float(&center.x, ((double)f->axis_x.x * xx + f->origin.x) + (double)f->axis_y.x * zz);
        bh_round_float(&center.y, ((double)f->axis_x.y * xx + f->origin.y) + (double)f->axis_y.y * zz);
        bh_round_float(&center.z, ((double)f->axis_x.z * xx + f->origin.z) + (double)f->axis_y.z * zz);
        bh_round_float(&width, (1.0 - signed_height) * .01);
        bh_round_float(&span, (double)length * .00025);
        if (!vector(&center) || !_finite(width) || !_finite(span) || !_finite(alpha) || !_finite(angle))
            return fail(c, BH_ATTACHMENT_NUMERICAL);
        return submit(
            c, bh_draw_oriented(c->queue, 3, image, &center, &f->axis_y, &f->normal, width, span, angle, 0, alpha));
    }
    bh_round_float(&scale, (double)f->screen_radius);
    raw_radius = (double)radius * scale * .0025;
    bh_round_float(&x, raw_radius);
    bh_round_float(&x, (double)f->screen_x + (double)x * sin((double)mid_angle));
    bh_round_float(&y, (double)f->screen_y - raw_radius * cos((double)mid_angle));
    bh_round_float(&width, (double)c->viewport_width * .0078125);
    bh_round_float(&width, (1.0 - signed_height) * width);
    bh_round_float(&span, (double)length * scale * .0025);
    if (!_finite(x) || !_finite(y) || !_finite(width) || !_finite(span) || !_finite(alpha) || !_finite(angle))
        return fail(c, BH_ATTACHMENT_NUMERICAL);
    return submit(c, bh_screen_quad_queue(c->queue, 8, image, x, y, width, span, angle, BH_HUD_ANCHOR_RIGHT, alpha));
}
static int render_node(BHAttachmentContext *c, BHAttachmentNode *node)
{
    unsigned long i, count, first, second, image;
    float height;
    RadarEndpoint a, b;
    BHPortal *p;
    while (!node->radar_visited)
    {
        node->radar_visited = 1;
        ++c->visited;
        if (!vector(&node->minimum) || !vector(&node->maximum))
            return fail(c, BH_ATTACHMENT_NUMERICAL);
        image = c->default_image;
        if (node->query_flags & 0xe00UL)
            image = (node->query_flags & 0x200UL)
                        ? c->flag200_image
                        : ((node->query_flags & 0x400UL) ? c->flag400_image : c->flag800_image);
        if (node->minimum.y > c->player_position.y)
            bh_round_float(&height, (double)c->player_position.y - node->minimum.y);
        else if (node->maximum.y < c->player_position.y)
            bh_round_float(&height, (double)c->player_position.y - node->maximum.y);
        else
            height = 0;
        count = 0;
        first = second = 0;
        for (i = 0; i < 6; ++i)
            if (node->navigation.portals[i].neighbor)
            {
                if (!count)
                    first = i;
                else
                    second = i;
                ++count;
            }
        if (count == 2)
        {
            if (!portal_endpoint(c, node->navigation.portals + first, &a) ||
                !portal_endpoint(c, node->navigation.portals + second, &b) || !segment(c, image, &a, &b, height))
                return 0;
            if (a.radius < 400 && !render_node(c, neighbor(node->navigation.portals + first)))
                return 0;
            if (b.radius >= 400)
                return 1;
            node = neighbor(node->navigation.portals + second); /* original tail visit */
        }
        else
        {
            if (!endpoint(c, &a, ((double)node->minimum.x + node->maximum.x) * .5,
                          ((double)node->minimum.z + node->maximum.z) * .5))
                return 0;
            for (i = 0; i < 6; ++i)
            {
                p = node->navigation.portals + i;
                if (!p->neighbor)
                    continue;
                if (!portal_endpoint(c, p, &b) || !segment(c, image, &a, &b, height))
                    return 0;
                if (b.radius < 400 && !render_node(c, neighbor(p)))
                    return 0;
            }
            return 1;
        }
    }
    return 1;
}
int __cdecl bh_attachment_render(BHAttachmentContext *c, BHAttachmentNode *root)
{
    if (!c || !root || !c->queue || !c->frame)
        return fail(c, BH_ATTACHMENT_ARGUMENT);
    c->visited = c->attempted = c->queued = c->dropped = c->culled = 0;
    c->error = BH_ATTACHMENT_OK;
    if (!vector(&c->player_position) || !_finite(c->yaw) || !vector(&c->frame->origin) || !vector(&c->frame->axis_x) ||
        !vector(&c->frame->axis_y) || !vector(&c->frame->normal))
        return fail(c, BH_ATTACHMENT_ARGUMENT);
    return render_node(c, root);
}
