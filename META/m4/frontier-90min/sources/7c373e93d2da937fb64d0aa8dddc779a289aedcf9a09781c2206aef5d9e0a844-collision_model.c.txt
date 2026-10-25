#include "../src/bh_blocks.h"
#include "../src/bh_collision_model.h"
#include "../src/bh_segment_query.h"
#include <math.h>
#include "saved_contact.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks;
static int failures;
#define CHECK(x, label) check((x) != 0, label, __LINE__)
static void check(int ok, const char *label, int line) {
    ++checks;
    if (!ok) {
        ++failures;
        if (failures < 8)
            printf("FAIL collision model %d: %s\n", line, label);
    }
}
static void section(unsigned char *bytes, unsigned long *at, unsigned long count, const void *data,
                    unsigned long size) {
    memcpy(bytes + *at, &count, 4);
    *at += 4;
    memcpy(bytes + *at, data, size);
    *at += size;
}
static unsigned long fixture(unsigned char *bytes) {
    BHBspMaterial material;
    BHBspVertex vertices[3];
    BHBspEdge edges[3];
    BHBspFace face;
    BHBspNode node;
    BHBspLeaf leaf;
    unsigned long at = 0, i;
    memset(&material, 0, sizeof(material));
    strcpy(material.texture, "STONE.PCX");
    memset(vertices, 0, sizeof(vertices));
    memset(edges, 0, sizeof(edges));
    for (i = 0; i < 3; ++i) {
        edges[i].vertex = i;
        edges[i].next = i < 2 ? i + 1 : -1;
    }
    vertices[0].y = vertices[0].z = -1;
    vertices[1].y = 1;
    vertices[1].z = -1;
    vertices[2].z = 1;
    face.material = face.first_edge = 0;
    face.runtime = 0x12345678;
    memset(&node, 0, sizeof(node));
    for (i = 0; i < 3; ++i) {
        node.minimum[i] = -1;
        node.maximum[i] = 1;
    }
    node.normal[0] = 1;
    node.front_node = node.back_node = node.back_leaf = -1;
    leaf.face = 0;
    leaf.next = -1;
    section(bytes, &at, 1, &material, sizeof(material));
    section(bytes, &at, 3, vertices, sizeof(vertices));
    section(bytes, &at, 3, edges, sizeof(edges));
    section(bytes, &at, 1, &face, sizeof(face));
    section(bytes, &at, 1, &node, sizeof(node));
    section(bytes, &at, 1, &leaf, sizeof(leaf));
    return at;
}
typedef struct Memory {
    unsigned long calls, fail_at, live;
} Memory;
static void *__cdecl allocate(unsigned long bytes, void *user) {
    Memory *m = (Memory *)user;
    void *p;
    if (++m->calls == m->fail_at)
        return NULL;
    p = malloc(bytes);
    if (p)
        ++m->live;
    return p;
}
static void __cdecl release(void *p, void *user) {
    Memory *m = (Memory *)user;
    CHECK(m->live > 0, "balanced owning model allocations");
    --m->live;
    free(p);
}
int bh_collision_models_real_check(const BHBlockLibrary *library) {
    BHCollisionModel model;
    unsigned long i, j;
    const BHBsp *expected, *actual;
    if (!bh_collision_model_init(&model, NULL))
        return 0;
    for (i = 0; i < library->count; ++i) {
        expected = &library->blocks[i].bsp;
        if (!bh_collision_model_load(&model, library->blocks[i].name, "BSP\\", BH_BSP_PREFIX_ONLY))
            goto failed;
        actual = &model.bsp;
        if (memcmp(actual->counts, expected->counts, sizeof(actual->counts)) ||
            memcmp(actual->materials, expected->materials, actual->counts[0] * 32) ||
            memcmp(actual->vertices, expected->vertices, actual->counts[1] * 60) ||
            memcmp(actual->edges, expected->edges, actual->counts[2] * 16) ||
            memcmp(actual->nodes, expected->nodes, actual->counts[4] * 56) ||
            memcmp(actual->leaves, expected->leaves, actual->counts[5] * 8) ||
            memcmp(actual->bounds, expected->bounds, actual->counts[4] * sizeof(BHBspBound)) ||
            model.geometry.bsp != actual)
            goto failed;
        for (j = 0; j < actual->counts[3]; ++j)
            if (actual->faces[j].material != expected->faces[j].material ||
                actual->faces[j].first_edge != expected->faces[j].first_edge ||
                actual->faces[j].runtime || model.geometry.face_flags[j])
                goto failed;
    }
    return bh_collision_model_dispose(&model);
failed:
    bh_collision_model_dispose(&model);
    return 0;
}
static void cases(void) {
    unsigned char bytes[1024], saved[1024], *old;
    unsigned long size, i, baseline;
    BHCollisionModel model;
    Memory memory = {0, 0, 0};
    BHSceneAllocator allocator = {allocate, release, &memory};
    BHSegmentQuery query;
    BHSegmentHit hit;
    BHMat34 identity;
    BHVec3 start = {2, 0, 0}, end = {-2, 0, 0};
    FILE *file;
    memset(&identity, 0, sizeof(identity));
    identity.m[0][0] = identity.m[1][1] = identity.m[2][2] = 1;
    size = fixture(bytes);
    memcpy(saved, bytes, size);
    CHECK(bh_collision_model_init(&model, &allocator), "initialize complete collision owner");
    CHECK(bh_collision_model_decode(&model, bytes, size),
          "decode all six sections and construct runtime metadata");
    CHECK(!memcmp(bytes, saved, size) && !model.bsp.faces[0].runtime &&
              !model.geometry.face_flags[0] && model.geometry.bsp == &model.bsp,
          "owned runtime reset leaves source input immutable and fixes owner pointers");
    CHECK(model.bsp.bounds[0].center.x == 0 && model.bsp.bounds[0].radius_squared == 3,
          "real collision bounds retain staged numerical contract");
    old = model.bsp.storage;
    baseline = memory.live;
    for (i = 1; i <= 4; ++i) {
        memory.fail_at = memory.calls + i;
        CHECK(!bh_collision_model_decode(&model, bytes, size) && model.bsp.storage == old &&
                  memory.live == baseline,
              "every partial ownership allocation rolls back to prior model");
    }
    memory.fail_at = 0;
    CHECK(bh_segment_query_init(&query, &model.geometry) &&
              bh_segment_query_run(&query, &start, &end, &identity, &identity, &hit) == 1 &&
              hit.face == 0 && hit.fraction == .5f,
          "new owning loader supplies real segment contact definitions");
    CHECK(bh_segment_query_dispose(&query), "release borrower before model replacement");
    file = fopen("bhcollision_search.tmp", "wb");
    CHECK(file != NULL, "create private filename-search fixture");
    if (file) {
        CHECK(fwrite(bytes, 1, size, file) == size && !fclose(file),
              "write complete private BSP fixture");
        CHECK(bh_collision_model_load(&model, "bhcollision_search.tmp", "missing\\",
                                      BH_BSP_PREFIX_FIRST),
              "prefix-first failure falls back to bare real VFILE");
        CHECK(bh_collision_model_load(&model, "bhcollision_search.tmp", "missing\\",
                                      BH_BSP_BARE_FIRST),
              "bare-first real VFILE load");
        old = model.bsp.storage;
        CHECK(!bh_collision_model_load(&model, "bhcollision_search.tmp", "missing\\",
                                       BH_BSP_PREFIX_ONLY) &&
                  model.bsp.storage == old,
              "prefix-only miss preserves complete prior owner");
        remove("bhcollision_search.tmp");
    }
    CHECK(bh_collision_model_dispose(&model) && !memory.live,
          "complete model lifetime frees all real owners");
}
static void face_contacts(void) {
    BHBsp a,b;
    BHBspVertex av[4],bv[4];
    BHBspEdge ae[4],be[4];
    BHBspFace af,bf;
    BHBspNode plane;
    BHMat34 identity, world, trial, inverse_world;
    BHVec3 normal = {1,0,0};
    BHFaceContact hit,saved;
    unsigned long i;
    int center, width, winding, expected, actual;
    float lo,hi;
    memset(&a,0,sizeof(a)); memset(&b,0,sizeof(b));
    memset(av,0,sizeof(av)); memset(bv,0,sizeof(bv));
    memset(&plane,0,sizeof(plane)); memset(&identity,0,sizeof(identity));
    identity.m[0][0]=identity.m[1][1]=identity.m[2][2]=1;
    world=identity; world.m[0][3]=10; world.m[1][3]=20; world.m[2][3]=30;
    /* a spans x/y on z=0; b spans y/z on x=0. */
    av[0].x=-2; av[0].y=-2; av[1].x=2; av[1].y=-2;
    av[2].x=2; av[2].y=2; av[3].x=-2; av[3].y=2;
    bv[0].y=-1; bv[0].z=-1; bv[1].y=1; bv[1].z=-1;
    bv[2].y=1; bv[2].z=1; bv[3].y=-1; bv[3].z=1;
    for(i=0;i<4;++i) {
        ae[i].vertex=be[i].vertex=i;
        ae[i].next=be[i].next=i==3 ? -1 : i+1;
    }
    af.first_edge=bf.first_edge=0;
    a.vertices=av; a.edges=ae; a.faces=&af;
    b.vertices=bv; b.edges=be; b.faces=&bf; b.nodes=&plane;
    a.counts[1]=a.counts[2]=b.counts[1]=b.counts[2]=4;
    a.counts[3]=b.counts[3]=b.counts[4]=1;
    plane.normal[0]=1;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit)==1 &&
        hit.raw_point.x==10 && hit.raw_point.y==20 && hit.raw_point.z==30 &&
        fabs(hit.point.x-10.05)<.00001,"face intersection transforms midpoint and offsets normal");
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&identity,&saved_contact_normal,&hit)==1 &&
        fabs(hit.point.x-.05*saved_contact_normal.x)<1e-8 &&
        fabs(hit.point.y-.05*saved_contact_normal.y)<1e-12 &&
        fabs(hit.point.z-.05*saved_contact_normal.z)<1e-12,
        "saved M3 normal drives actual recovered contact offset (not detector replay)");
    trial=world;inverse_world=identity;
    inverse_world.m[0][3]=-10;inverse_world.m[1][3]=-20;inverse_world.m[2][3]=-30;
    CHECK(bh_collision_cached_contact(&a,0,&trial,&b,0,0,&world,&inverse_world,
        &saved_contact_normal,&hit)==1 && hit.raw_point.x==10 && hit.raw_point.y==20 &&
        hit.raw_point.z==30 && fabs(hit.point.x-(10+.05*saved_contact_normal.x))<1e-6,
        "cached trial composes local times pose and retains saved M3 normal offset");
    saved=hit;trial.m[0][3]+=8;
    CHECK(bh_collision_cached_contact(&a,0,&trial,&b,0,0,&world,&inverse_world,
        &normal,&hit)==0 && !memcmp(&saved,&hit,sizeof(hit)),
        "cached trial miss retains previous contact");
    CHECK(bh_collision_cached_contact(&a,0,NULL,&b,0,0,&world,&inverse_world,
        &normal,&hit)==-1 && !memcmp(&saved,&hit,sizeof(hit)),
        "invalid trial leaves contact unchanged");
    for(i=0;i<4;++i) bv[i].y+=2;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&identity,&normal,&hit)==1 &&
        hit.raw_point.y==1.5f,"partial interval clip uses midpoint of overlap");
    saved=hit;
    for(i=0;i<4;++i) bv[i].y+=1;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&identity,&normal,&hit)==0 &&
        !memcmp(&hit,&saved,sizeof(hit)),"endpoint-only overlap is a strict miss retaining output");
    for(i=0;i<4;++i) bv[i].y-=3;
    identity.m[1][3]=.5f;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit)==1 &&
        hit.raw_point.y==20,"a-to-b translation intersects in destination coordinates");
    identity.m[1][3]=0;
    for(i=0;i<4;++i) ae[i].vertex=3-i;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit)==1 &&
        hit.raw_point.y==20,"reversed source winding keeps interval contact");
    for(i=0;i<4;++i) ae[i].vertex=i;
    for(i=0;i<4;++i) av[i].x=0;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit)==0,
        "coplanar source polygon has no strict crossings");
    av[0].x=av[3].x=-2; av[1].x=av[2].x=2;
    saved=hit; ae[0].next=99;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit)==-1 &&
        !memcmp(&hit,&saved,sizeof(hit)),"invalid topology retains output");
    ae[0].next=0;
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit)==-1,
        "edge cycle rejects within owned edge count");
    ae[0].next=1;
    /* Independent interval oracle on perpendicular rectangles: exercises both
       endpoint order branches, containment, partial overlap and disjointness. */
    for(winding=0;winding<4;++winding) {
        for(i=0;i<4;++i) {
            ae[i].vertex=(winding&1) ? 3-i : i;
            be[i].vertex=(winding&2) ? 3-i : i;
        }
        for(center=-4;center<=4;++center) for(width=1;width<=5;++width) {
            bv[0].y=bv[3].y=center-width*.5f;
            bv[1].y=bv[2].y=center+width*.5f;
            lo=bv[0].y > -2 ? bv[0].y : -2;
            hi=bv[1].y < 2 ? bv[1].y : 2;
            expected=hi>lo;
            saved=hit;
            actual=bh_collision_face_contact(&a,0,&identity,&b,0,0,&world,&normal,&hit);
            CHECK(actual==expected && (expected ?
                hit.raw_point.x==10 && hit.raw_point.y==20+(lo+hi)*.5f && hit.raw_point.z==30 :
                !memcmp(&hit,&saved,sizeof(hit))),"independent interval/winding oracle");
        }
    }
    CHECK(bh_collision_face_contact(&a,-1,&identity,&b,0,0,&world,&normal,&hit)==-1,
        "invalid face rejects");
    CHECK(bh_collision_face_contact(&a,0,&identity,&b,0,1,&world,&normal,&hit)==-1,
        "invalid plane rejects");
}
int bh_collision_model_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    face_contacts();
    *count = checks;
    printf("{\"scenario\":\"collision-model\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
