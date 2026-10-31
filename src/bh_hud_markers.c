#include "bh_hud_markers.h"
#include "bh_round.h"
#ifdef BH_SDL_BACKEND
#include "bh_render_interpolation.h"
#endif
#include <stddef.h>
#include <float.h>
#include <string.h>

typedef char hud_marker_query_offset[offsetof(BHEntity, query) == 0xd0 ? 1 : -1];
typedef char hud_marker_payload_offset[offsetof(BHEntity, payload) == 0xe4 ? 1 : -1];
typedef char hud_marker_pointer_size[sizeof(BHEntity *) == 4 ? 1 : -1];
typedef union BHHudMarkerPacket {
    BHEntityPositionPacket position;
    unsigned long words[10];
} BHHudMarkerPacket;

static int fail(BHHudMarkers *c, BHHudMarkersError error)
{
    if (c)
        c->error = error;
    return 0;
}
static long multiply(long a, unsigned long b)
{
    return (long)((unsigned long)a * b);
}
static int rectangle(BHHudMarkers *c, unsigned long image, double x, double y, float width, float height, float angle,
                     float alpha, float u, float du, float v, float dv, float depth)
{
    float sx, sy;
    int r;
    bh_round_float(&sx, x);
    bh_round_float(&sy, y);
    r = bh_pickup_rect_queue(c->projection->queue, 3, image, sx, sy, width, height, angle, 0, alpha, u, du, v, dv,
                             depth);
    ++c->attempted;
    if (r > 0)
        ++c->queued;
    else if (!r)
        ++c->dropped;
    else
        return fail(c, BH_HUD_MARKERS_QUEUE);
    return 1;
}
int __cdecl bh_hud_markers_draw(BHHudMarkers *c, BHEntity *head, const BHVec3 *player,
                                const unsigned char *player_flags)
{
    BHHudMarkerPacket packet;
    BHVec3 point;
    long kind, unit;
    short status;
    unsigned long radar_kind, extended, skip;
#ifdef BH_SDL_BACKEND
    unsigned long marker_start, marker_index;
#endif
    float corner_u, label_v, x, y, z, scale, alpha, size, label_width;
    double raw_alpha, raw_depth, minimum;
    if (!c || !c->projection || !c->projection->queue || !c->projection->camera || !c->projection->view || !c->radar ||
        !player || !player_flags || c->radar->queue != c->projection->queue || c->display_width <= 0)
        return fail(c, BH_HUD_MARKERS_ARGUMENT);
    c->error = BH_HUD_MARKERS_OK;
    c->visited = c->attempted = c->queued = c->dropped = c->projected = 0;
    unit = c->display_width / 160;
    packet.position.event = 3;
    while (head)
    {
        if (c->visited >= c->limit)
            return fail(c, BH_HUD_MARKERS_LIMIT);
        ++c->visited;
        if (!head->query)
            return fail(c, BH_HUD_MARKERS_CALLBACK);
        head->query(head, &packet);
        memcpy(&kind, head->payload + 24, 4);
        extended = skip = 0;
        switch (kind)
        {
        case 1:
            radar_kind = 3;
            corner_u = 0;
            label_v = .25f;
            break;
        case 2:
            radar_kind = 0;
            corner_u = .25f;
            label_v = .375f;
            break;
        case 6:
            radar_kind = 1;
            corner_u = .5f;
            label_v = .5f;
            extended = 1;
            skip = *player_flags & 2;
            break;
        case 8:
            radar_kind = 1;
            corner_u = 0;
            label_v = .25f;
            extended = 1;
            skip = *player_flags & 2;
            break;
        default:
            return fail(c, BH_HUD_MARKERS_KIND);
        }
        point = packet.position.position;
        if (!bh_radar_marker(c->radar, player, &point, radar_kind, extended, skip))
            return fail(c, BH_HUD_MARKERS_QUEUE);
        c->attempted += c->radar->attempted;
        c->queued += c->radar->queued;
        c->dropped += c->radar->dropped;
        x = point.x;
        y = point.y;
        z = point.z;
        if (bh_pickup_project(c->projection, &x, &y, &z, &scale))
        {
            if (!_finite(x) || !_finite(y) || !_finite(z) || !_finite(scale))
                return fail(c, BH_HUD_MARKERS_NUMERICAL);
            ++c->projected;
            raw_alpha = ((double)scale * 100.0) / (double)c->display_width;
            bh_round_float(&alpha, raw_alpha);
            if (raw_alpha > 1.0)
                alpha = 1;
            if ((double)alpha > .1)
            {
                raw_depth = (double)z - 8.0;
                minimum = (double)c->projection->view->near_z - (-.01);
                bh_round_float(&z, raw_depth);
                if (raw_depth < minimum)
                    bh_round_float(&z, minimum);
                size = (float)multiply(unit, 4);
#ifdef BH_SDL_BACKEND
                marker_start = c->projection->queue->count;
#endif
                if (!rectangle(c, c->brackets_image, (double)x - scale * 2.0, (double)y - scale * 2.0, size, size, 0,
                               alpha, corner_u, .25f, 0, .25f, z) ||
                    !rectangle(c, c->brackets_image, (double)x - (double)scale * (-2.0), (double)y - scale * 2.0, size,
                               size, 1.5707963705062866211f, alpha, corner_u, .25f, 0, .25f, z) ||
                    !rectangle(c, c->brackets_image, (double)x - scale * 2.0, (double)y - (double)scale * (-2.0), size,
                               size, -1.5707963705062866211f, alpha, corner_u, .25f, 0, .25f, z) ||
                    !rectangle(c, c->brackets_image, (double)x - (double)scale * (-2.0),
                               (double)y - (double)scale * (-2.0), size, size, -3.1415927410125732422f, alpha, corner_u,
                               .25f, 0, .25f, z))
                    return 0;
                /* Original reloads kind after four synchronous producers. */
                memcpy(&kind, head->payload + 24, 4);
                if (kind == 8)
                {
                    memcpy(&status, head->payload + 48, 2);
                    label_width = (float)multiply(unit, 24);
                    bh_round_float(&label_v, (double)status * .125);
                    if (!rectangle(c, c->status_image, ((double)x - scale * 2.0) - (double)unit * (-9.5),
                                   ((double)y - (double)scale * (-2.0)) - (double)unit * (-4.5), label_width, size, 0,
                                   alpha, 0, .75f, label_v, .125f, z))
                        return 0;
                }
                else
                {
                    label_width = (float)multiply(unit, 16);
                    if (!rectangle(c, c->brackets_image, ((double)x - scale * 2.0) + (double)multiply(unit, 6),
                                   ((double)multiply(unit, 5) - (double)scale * (-2.0)) + y, label_width, size, 0,
                                   alpha, 0, .5f, label_v, .125f, z))
                        return 0;
                }
#ifdef BH_SDL_BACKEND
                /* Preserve pixel-sized corners/labels, but project their world
                   anchor and perspective offsets with the sampled camera. */
                for (marker_index = marker_start; marker_index < c->projection->queue->count; ++marker_index)
                {
                    BHDrawCommand *command = c->projection->queue->commands + marker_index;
                    unsigned long corner = marker_index - marker_start;
                    float sx, sy, fx = corner < 4 && (corner & 1) ? 2.0f : -2.0f;
                    float fy = corner < 2 ? -2.0f : 2.0f;
                    memcpy(&sx, command->words, 4);
                    memcpy(&sy, command->words + 1, 4);
                    sx -= x + fx * scale;
                    sy -= y + fy * scale;
                    memcpy(command->words, &sx, 4);
                    memcpy(command->words + 1, &sy, 4);
                    command->words[12] = BH_RECT_WORLD_MARKER;
                    memcpy(command->words + 13, &point, 12);
                    memcpy(command->words + 16, &fx, 4);
                    memcpy(command->words + 17, &fy, 4);
                }
                bh_interpolation_stamp(marker_start, (unsigned long)head, head->generation);
#endif
            }
        }
        memcpy(&head, head->payload + 12, 4);
    }
    return 1;
}
