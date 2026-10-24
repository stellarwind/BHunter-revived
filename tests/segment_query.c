#include "../src/bh_segment_query.h"
#include <math.h>
#include "saved_contact.h"
#include <stdio.h>
#include <string.h>

static unsigned long checks;
static int failures;
#define CHECK(x, label) check((x) != 0, label, __LINE__)
static void check(int ok, const char *label, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        if (failures < 8)
            printf("FAIL segment %d: %s\n", line, label);
    }
}
static void identity(BHMat34 *m) {
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}
int bh_segment_real_check(BHGeometry *geometry, unsigned long count) {
    BHSegmentQuery query;
    BHSegmentHit first, second;
    BHMat34 matrix;
    BHVec3 a, b;
    unsigned long i, axis;
    int x, y;
    identity(&matrix);
    for (i = 0; i < count; ++i) {
        const BHBsp *bsp = geometry[i].bsp;
        if (!bsp->counts[3] || !bsp->counts[4])
            continue;
        if (!bh_segment_query_init(&query, geometry + i))
            return 0;
        for (axis = 0; axis < 3; ++axis) {
            a = b = bsp->bounds[0].center;
            ((float *)&a)[axis] -= bsp->bounds[0].radius + 1;
            ((float *)&b)[axis] += bsp->bounds[0].radius + 1;
            x = bh_segment_query_run(&query, &a, &b, &matrix, &matrix, &first);
            y = bh_segment_query_run(&query, &a, &b, &matrix, &matrix, &second);
            if (x < 0 || x != y ||
                (x && (memcmp(&first, &second, sizeof(first)) || first.face < 0 ||
                       (unsigned long)first.face >= bsp->counts[3] ||
                       first.flags != geometry[i].face_flags[first.face]))) {
                bh_segment_query_dispose(&query);
                return 0;
            }
        }
        if (!bh_segment_query_dispose(&query))
            return 0;
    }
    return 1;
}
static void cases(void) {
    BHBsp bsp;
    BHBspVertex vertices[12];
    BHBspEdge edges[12];
    BHBspFace faces[3];
    BHBspNode nodes[2];
    BHBspLeaf leaves[3];
    BHBspMaterial material;
    unsigned char storage = 0;
    BHGeometry geometry;
    BHSegmentQuery query;
    BHSegmentHit hit, saved;
    BHMat34 world, local;
    BHVec3 a = {12, 20, 30}, b = {8, 20, 30};
    unsigned long i, j;
    memset(&bsp, 0, sizeof(bsp));
    memset(vertices, 0, sizeof(vertices));
    memset(edges, 0, sizeof(edges));
    memset(faces, 0, sizeof(faces));
    memset(nodes, 0, sizeof(nodes));
    bsp.vertices = vertices;
    bsp.edges = edges;
    bsp.faces = faces;
    bsp.nodes = nodes;
    bsp.leaves = leaves;
    bsp.materials = &material;
    bsp.storage = &storage;
    bsp.counts[0] = 1;
    bsp.counts[1] = bsp.counts[2] = 12;
    bsp.counts[3] = bsp.counts[5] = 3;
    bsp.counts[4] = 2;
    for (i = 0; i < 3; ++i) {
        faces[i].first_edge = i * 4;
        leaves[i].face = i;
        leaves[i].next = -1;
        for (j = 0; j < 4; ++j) {
            vertices[i * 4 + j].x = i == 2 ? 1.0f : 0.0f;
            vertices[i * 4 + j].y = (j == 0 || j == 3) ? -1.0f : 1.0f;
            vertices[i * 4 + j].z = j < 2 ? -1.0f : 1.0f;
            edges[i * 4 + j].vertex = i * 4 + j;
            edges[i * 4 + j].next = j < 3 ? i * 4 + j + 1 : -1;
        }
    }
    nodes[0].normal[0] = nodes[1].normal[0] = 1;
    nodes[0].front_node = 1;
    nodes[0].back_node = -1;
    nodes[0].front_leaf = 0;
    nodes[0].back_leaf = 1;
    nodes[1].front_node = nodes[1].back_node = -1;
    nodes[1].front_leaf = 2;
    nodes[1].back_leaf = -1;
    nodes[1].plane = -1;
    identity(&world);
    world.m[0][3] = 10;
    world.m[1][3] = 20;
    world.m[2][3] = 30;
    bh_mat34_inverse_rigid(&world, &local);
    CHECK(bh_geometry_init(&geometry, &bsp, NULL), "real geometry metadata owner");
    CHECK(bh_segment_query_init(&query, &geometry), "owned traversal workspace");
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == 1 && hit.face == 2 &&
              hit.fraction == .25f && hit.normal.x == 1 && fabs(hit.point.x - 11.05f) < 0.00001f &&
              hit.point.y == 20 && hit.point.z == 30,
          "near child precedes parent; real transforms and contact offset");
    CHECK(bh_geometry_set_flags(&geometry, 2, 1), "disable near polygon via real flags");
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == 1 && hit.face == 0 &&
              hit.fraction == .5f && hit.normal.x == 1,
          "flagged near face skipped before front-list contact");
    CHECK(bh_geometry_set_flags(&geometry, 0, 1), "disable front-list polygon");
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == 1 && hit.face == 1 &&
              hit.normal.x == -1 && fabs(hit.point.x - 9.95f) < .00001f,
          "back-list contact reverses normal and offset");
    saved = hit;
    a.y = b.y = 22;
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == 0 &&
              !memcmp(&hit, &saved, sizeof(hit)),
          "outside polygon misses without replacing output");
    a.y = b.y = 21;
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == 1,
          "zero edge signs join nonpositive winding at polygon boundary");
    a.y = b.y = 20;
    a.x = 14;
    b.x = 12;
    saved = hit;
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == 0 &&
              !memcmp(&hit, &saved, sizeof(hit)),
          "same-side segment follows child without false contact");
    a.x = 12;
    b.x = 8;
    /* Captured output contract on analytical geometry; original swept model
       inputs were not retained, so this is explicitly not a replay. */
    a.x=17; b.x=9; geometry.face_flags[0]=geometry.face_flags[1]=geometry.face_flags[2]=0;
    nodes[0].front_node=-1;
    CHECK(bh_segment_query_run(&query,&a,&b,&world,&local,&hit)==1 &&
        hit.fraction==SAVED_CONTACT_FRACTION &&
        fabs(hit.normal.x-saved_contact_normal.x)<.000001 &&
        fabs(hit.normal.y-saved_contact_normal.y)<.000001 &&
        fabs(hit.normal.z-saved_contact_normal.z)<.000001,
        "saved M3 fraction/normal contract on analytical segment (not full detector replay)");
    a.x=12; b.x=8; saved=hit;
    nodes[0].front_node = 999;
    CHECK(bh_segment_query_run(&query, &a, &b, &world, &local, &hit) == -1 &&
              !memcmp(&hit, &saved, sizeof(hit)) && !query.busy,
          "foreign node rejected with output and workspace retained");
    nodes[0].front_node = 1;
    CHECK(bh_segment_query_dispose(&query), "query workspace disposal");
    CHECK(bh_geometry_dispose(&geometry), "borrowed geometry outlives query");
}
int bh_segment_query_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    *count = checks;
    printf("{\"scenario\":\"segment-query\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
