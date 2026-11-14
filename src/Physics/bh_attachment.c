#include "bh_attachment.h"
#include "../Math/bh_round.h"
#include <float.h>
#include <math.h>
#include <stddef.h>

static_assert(offsetof(BHWorldGeometry, navigation) == 0, "BHWorldGeometry.navigation offset");
static_assert(sizeof(BHPortal) == 60, "BHPortal size");
static_assert(offsetof(BHPortal, neighbor) == 56, "BHPortal.neighbor offset");
typedef struct RadarEndpoint
{
    float x, z, radius;
} RadarEndpoint;

static BHAttachmentNode *neighbor(const BHPortal *portal)
{
    /* navigation is the first member, including in every borrowed neighbor. */
    return (BHAttachmentNode *)portal->neighbor;
}
void bh_attachment_reset(BHAttachmentNode *node)
{
    unsigned long i;
    if (!node || !node->radar_visited)
        return;
    node->radar_visited = 0;
    for (i = 0; i < 6; ++i)
        if (node->navigation.portals[i].neighbor)
            bh_attachment_reset(neighbor(node->navigation.portals + i));
}
static int fail(BHAttachmentContext *context, BHAttachmentError error)
{
    if (context)
        context->error = error;
    return 0;
}
static int vector(const BHVec3 *vec3)
{
    return _finite(vec3->x) && _finite(vec3->y) && _finite(vec3->z);
}
static int endpoint(BHAttachmentContext *context, RadarEndpoint *out, double x, double z)
{
    double dx = x - context->player_position.x, dz = z - context->player_position.z, squared;
    bh_round_float(&out->x, dx);
    bh_round_float(&out->z, dz);
    squared = dx * out->x + dz * out->z;
    if (!_finite(squared) || squared < 0)
        return fail(context, BH_ATTACHMENT_NUMERICAL);
    bh_round_float(&out->radius, sqrt(squared));
    return _finite(out->x) && _finite(out->z) && _finite(out->radius) ? 1 : fail(context, BH_ATTACHMENT_NUMERICAL);
}
static int portal_endpoint(BHAttachmentContext *c, const BHPortal *portal, RadarEndpoint *out)
{
    float u, v, plane;
    if (portal->direction > 6 || portal->direction == 3)
        return fail(c, BH_ATTACHMENT_PORTAL);
    if (!_finite(portal->overlap[0]) || !_finite(portal->overlap[1]) || !_finite(portal->overlap[2]) || !_finite(portal->overlap[3]))
        return fail(c, BH_ATTACHMENT_NUMERICAL);
    bh_round_float(&u, ((double)portal->overlap[0] + portal->overlap[1]) * .5);
    bh_round_float(&v, ((double)portal->overlap[2] + portal->overlap[3]) * .5);
    bh_round_float(&plane, (double)portal->plane);
    switch (portal->direction & 3UL)
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
static int radial_clip(RadarEndpoint *first_radar_endpoint, RadarEndpoint *second_radar_endpoint)
{
    float difference, wa, wb;
    double raw_wa, raw_wb;
    if (first_radar_endpoint->radius < 400)
    {
        if (second_radar_endpoint->radius > 400)
        {
            bh_round_float(&difference, (double)second_radar_endpoint->radius - first_radar_endpoint->radius);
            raw_wa = ((double)second_radar_endpoint->radius - 400) / difference;
            raw_wb = (400.0 - first_radar_endpoint->radius) / difference;
            bh_round_float(&wa, raw_wa);
            bh_round_float(&wb, raw_wb);
            second_radar_endpoint->radius = 400;
            bh_round_float(&second_radar_endpoint->x, raw_wb * second_radar_endpoint->x + raw_wa * first_radar_endpoint->x);
            bh_round_float(&second_radar_endpoint->z, (double)wb * second_radar_endpoint->z + (double)wa * first_radar_endpoint->z);
        }
        return 1;
    }
    if (second_radar_endpoint->radius >= 400)
        return 0;
    bh_round_float(&difference, (double)first_radar_endpoint->radius - second_radar_endpoint->radius);
    raw_wb = ((double)first_radar_endpoint->radius - 400) / difference;
    raw_wa = (400.0 - second_radar_endpoint->radius) / difference;
    bh_round_float(&wb, raw_wb);
    bh_round_float(&wa, raw_wa);
    first_radar_endpoint->radius = 400;
    bh_round_float(&first_radar_endpoint->x, raw_wa * first_radar_endpoint->x + raw_wb * second_radar_endpoint->x);
    bh_round_float(&first_radar_endpoint->z, (double)wa * first_radar_endpoint->z + (double)wb * second_radar_endpoint->z);
    return 1;
}
static int submit(BHAttachmentContext *context, int result)
{
    ++context->attempted;
    if (result > 0)
        ++context->queued;
    else if (!result)
        ++context->dropped;
    else
        return fail(context, BH_ATTACHMENT_QUEUE);
    return 1;
}
static int segment(BHAttachmentContext *context, unsigned long image, RadarEndpoint *first_radar_endpoint, RadarEndpoint *second_radar_endpoint, float height)
{
    const BHRadarFrame *radar_frame = context->frame;
    double signed_height = (double)height * .02, absolute_height, mx, mz, dx, dz, raw_radius, xx, zz;
    float fade, angle, mid_angle, delta_x, delta_z, length, radius, width, span, alpha, x, y, scale;
    BHVec3 center;
    int visible = radial_clip(first_radar_endpoint, second_radar_endpoint);
    absolute_height = signed_height < 0 ? -signed_height : signed_height;
    if (absolute_height > 1)
        absolute_height = 1;
    bh_round_float(&fade, 1.0 - absolute_height);
    if (fade == 0)
    {
        visible = 0;
        second_radar_endpoint->radius = 400;
    }
    if (!visible)
    {
        ++context->culled;
        return 1;
    }
    mx = ((double)second_radar_endpoint->x + first_radar_endpoint->x) * .5;
    mz = ((double)second_radar_endpoint->z + first_radar_endpoint->z) * .5;
    dx = (double)second_radar_endpoint->x - first_radar_endpoint->x;
    dz = (double)second_radar_endpoint->z - first_radar_endpoint->z;
    bh_round_float(&delta_x, dx);
    bh_round_float(&delta_z, dz);
    bh_round_float(&angle, atan2(dx, dz) - context->yaw);
    bh_round_float(&mid_angle, atan2(mx, mz) - context->yaw);
    bh_round_float(&length, sqrt(dx * delta_x + dz * delta_z));
    /* Original midpoint x is stored and reloaded for only one factor. */
    bh_round_float(&radius, mx);
    bh_round_float(&radius, sqrt(mx * radius + mz * mz));
    bh_round_float(&alpha, (double)fade * (1.0 - ((double)second_radar_endpoint->radius + first_radar_endpoint->radius) * .00125));
    if (!radar_frame->flat)
    {
        raw_radius = (double)radius * .00025;
        xx = raw_radius * sin((double)mid_angle);
        zz = raw_radius * cos((double)mid_angle);
        bh_round_float(&center.x, ((double)radar_frame->axis_x.x * xx + radar_frame->origin.x) + (double)radar_frame->axis_y.x * zz);
        bh_round_float(&center.y, ((double)radar_frame->axis_x.y * xx + radar_frame->origin.y) + (double)radar_frame->axis_y.y * zz);
        bh_round_float(&center.z, ((double)radar_frame->axis_x.z * xx + radar_frame->origin.z) + (double)radar_frame->axis_y.z * zz);
        bh_round_float(&width, (1.0 - signed_height) * .01);
        bh_round_float(&span, (double)length * .00025);
        if (!vector(&center) || !_finite(width) || !_finite(span) || !_finite(alpha) || !_finite(angle))
            return fail(context, BH_ATTACHMENT_NUMERICAL);
        return submit(
            context, bh_draw_oriented(context->queue, 3, image, &center, &radar_frame->axis_y, &radar_frame->normal, width, span, angle, 0, alpha));
    }
    bh_round_float(&scale, (double)radar_frame->screen_radius);
    raw_radius = (double)radius * scale * .0025;
    bh_round_float(&x, raw_radius);
    bh_round_float(&x, (double)radar_frame->screen_x + (double)x * sin((double)mid_angle));
    bh_round_float(&y, (double)radar_frame->screen_y - raw_radius * cos((double)mid_angle));
    bh_round_float(&width, (double)context->viewport_width * .0078125);
    bh_round_float(&width, (1.0 - signed_height) * width);
    bh_round_float(&span, (double)length * scale * .0025);
    if (!_finite(x) || !_finite(y) || !_finite(width) || !_finite(span) || !_finite(alpha) || !_finite(angle))
        return fail(context, BH_ATTACHMENT_NUMERICAL);
    return submit(context, bh_screen_quad_queue(context->queue, 8, image, x, y, width, span, angle, BH_HUD_ANCHOR_RIGHT, alpha));
}
static int render_node(BHAttachmentContext *context, BHAttachmentNode *node)
{
    unsigned long i, count, first, second, image;
    float height;
    RadarEndpoint a, b;
    BHPortal *portal;
    while (!node->radar_visited)
    {
        node->radar_visited = 1;
        ++context->visited;
        if (!vector(&node->minimum) || !vector(&node->maximum))
            return fail(context, BH_ATTACHMENT_NUMERICAL);
        image = context->default_image;
        if (node->query_flags & 0xe00UL)
            image = (node->query_flags & 0x200UL)
                        ? context->flag200_image
                        : ((node->query_flags & 0x400UL) ? context->flag400_image : context->flag800_image);
        if (node->minimum.y > context->player_position.y)
            bh_round_float(&height, (double)context->player_position.y - node->minimum.y);
        else if (node->maximum.y < context->player_position.y)
            bh_round_float(&height, (double)context->player_position.y - node->maximum.y);
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
            if (!portal_endpoint(context, node->navigation.portals + first, &a) ||
                !portal_endpoint(context, node->navigation.portals + second, &b) || !segment(context, image, &a, &b, height))
                return 0;
            if (a.radius < 400 && !render_node(context, neighbor(node->navigation.portals + first)))
                return 0;
            if (b.radius >= 400)
                return 1;
            node = neighbor(node->navigation.portals + second); /* original tail visit */
        }
        else
        {
            if (!endpoint(context, &a, ((double)node->minimum.x + node->maximum.x) * .5,
                          ((double)node->minimum.z + node->maximum.z) * .5))
                return 0;
            for (i = 0; i < 6; ++i)
            {
                portal = node->navigation.portals + i;
                if (!portal->neighbor)
                    continue;
                if (!portal_endpoint(context, portal, &b) || !segment(context, image, &a, &b, height))
                    return 0;
                if (b.radius < 400 && !render_node(context, neighbor(portal)))
                    return 0;
            }
            return 1;
        }
    }
    return 1;
}
int bh_attachment_render(BHAttachmentContext *context, BHAttachmentNode *root)
{
    if (!context || !root || !context->queue || !context->frame)
        return fail(context, BH_ATTACHMENT_ARGUMENT);
    context->visited = context->attempted = context->queued = context->dropped = context->culled = 0;
    context->error = BH_ATTACHMENT_OK;
    if (!vector(&context->player_position) || !_finite(context->yaw) || !vector(&context->frame->origin) || !vector(&context->frame->axis_x) ||
        !vector(&context->frame->axis_y) || !vector(&context->frame->normal))
        return fail(context, BH_ATTACHMENT_ARGUMENT);
    return render_node(context, root);
}
