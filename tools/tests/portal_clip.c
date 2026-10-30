#include "../../src/bh_portal_clip.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static BHPortalClipCache cache;
static void portal(BHDrawCommand *c, float half, float z, unsigned long parent)
{
    BHVec3 corners[4] = {{-half, -half, z}, {half, -half, z}, {half, half, z}, {-half, half, z}};
    float near_z = .1f, far_z = 1000;
    memset(c, 0, sizeof(*c));
    c->kind = BH_DRAW_CLIP_RECT;
    c->resource = BH_PORTAL_CLIP_WORLD;
    memcpy(c->words, corners, sizeof(corners));
    c->words[12] = parent;
    memcpy(c->words + 13, &near_z, 4);
    memcpy(c->words + 14, &far_z, 4);
}
static void camera_at(BHMainCamera *c, float x, float z)
{
    memset(c, 0, sizeof(*c));
    /* 90-degree 640x480 projection, camera looking along +Z. */
    c->projected.m[0][0] = 320;
    c->projected.m[0][2] = 320;
    c->projected.m[0][3] = -320 * x - 320 * z;
    c->projected.m[1][1] = 320;
    c->projected.m[1][2] = 240;
    c->projected.m[1][3] = -240 * z;
    c->projected.m[2][2] = 1;
    c->projected.m[2][3] = -z;
}
int main(void)
{
    BHDrawCommand commands[3], unchanged[3];
    BHDrawQueue q = {0};
    BHMainCamera camera;
    BHViewConfig view = {0};
    float b[4], parent[4];
    q.commands = commands;
    q.count = 3;
    q.capacity = 3;
    view.width = 640;
    view.height = 480;
    portal(commands, 2, 10, BH_DRAW_END);
    portal(commands + 1, 4, 20, 0);
    portal(commands + 2, 1, -10, 0);
    memcpy(unchanged, commands, sizeof(commands));
    camera_at(&camera, 0, 0);
    bh_portal_clip_reset(&cache);
    assert(bh_portal_clip_project(&q, 1, &camera, &view, &cache, b));
    assert(fabs(b[0] - 256) < .001);
    assert(fabs(b[2] - 384) < .001);
    assert(cache.projections == 2);
    assert(bh_portal_clip_project(&q, 1, &camera, &view, &cache, b));
    assert(cache.projections == 2);
    /* Render halfway through a 30 Hz update. The near and far portal edges
       move by different amounts: interpolating stale rectangles is incorrect. */
    camera_at(&camera, 1, 0);
    bh_portal_clip_reset(&cache);
    assert(bh_portal_clip_project(&q, 0, &camera, &view, &cache, parent));
    assert(fabs(parent[0] - 224) < .001);
    assert(bh_portal_clip_project(&q, 1, &camera, &view, &cache, b));
    assert(fabs(b[0] - 240) < .001);
    assert(fabs(b[2] - 352) < .001);
    assert(b[0] >= parent[0] && b[2] <= parent[2]);
    /* Dolly changes perspective as well as the portal's screen-space position. */
    camera_at(&camera, 1, 2);
    bh_portal_clip_reset(&cache);
    assert(bh_portal_clip_project(&q, 1, &camera, &view, &cache, b));
    assert(fabs(b[0] - (320 - 5 * 320.f / 18)) < .001);
    assert(fabs(b[2] - 360) < .001);
    assert(bh_portal_clip_project(&q, 2, &camera, &view, &cache, b));
    assert(b[0] == 0 && b[2] == 0);
    assert(!memcmp(commands, unchanged, sizeof(commands)));
    commands[0].words[12] = 1;
    bh_portal_clip_reset(&cache);
    assert(!bh_portal_clip_project(&q, 0, &camera, &view, &cache, b));
    puts("Portal clips: render-camera parallax/dolly, nested bounds, per-render cache, behind-camera culling and "
         "immutable snapshots passed");
    return 0;
}
