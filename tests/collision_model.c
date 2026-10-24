#include "../src/bh_blocks.h"
#include "../src/bh_collision_model.h"
#include "../src/bh_segment_query.h"
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
int bh_collision_model_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    *count = checks;
    printf("{\"scenario\":\"collision-model\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
