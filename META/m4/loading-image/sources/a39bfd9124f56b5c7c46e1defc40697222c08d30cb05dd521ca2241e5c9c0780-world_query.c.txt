#include "../src/bh_world_query.h"
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
        if (failures < 12)
            printf("FAIL world query line %d: %s\n", line, label);
    }
}
static void ring(BHWorldGeometry *w, unsigned long n) {
    unsigned long i;
    memset(w, 0, n * sizeof(*w));
    for (i = 0; i < n; ++i) {
        w[i].navigation.next = &w[(i + 1) % n].navigation;
        w[i].minimum.x = w[i].minimum.y = w[i].minimum.z = -2;
        w[i].maximum.x = w[i].maximum.y = w[i].maximum.z = 2;
    }
}
static void port(BHWorldGeometry *a, unsigned long slot, BHWorldGeometry *b,
                 unsigned long direction, long plane) {
    BHPortal *p = a->navigation.portals + slot;
    p->neighbor = &b->navigation;
    p->direction = direction;
    p->plane = plane;
    p->aperture[0] = p->aperture[2] = -2;
    p->aperture[1] = p->aperture[3] = 2;
    p->overlap[0] = p->scratch[0] = p->overlap[2] = p->scratch[2] = -0.1f;
    p->overlap[1] = p->scratch[1] = p->overlap[3] = p->scratch[3] = 0.1f;
    if (a->navigation.count <= slot)
        a->navigation.count = slot + 1;
}
typedef struct Memory {
    unsigned long calls, fail_at, live;
} Memory;
static void *__cdecl allocate(unsigned long bytes, void *user) {
    Memory *m = (Memory *)user;
    void *p;
    ++m->calls;
    if (m->calls == m->fail_at)
        return NULL;
    p = malloc(bytes);
    if (p)
        ++m->live;
    return p;
}
static void __cdecl release(void *p, void *user) {
    Memory *m = (Memory *)user;
    CHECK(m->live > 0, "balanced broad-phase workspace ownership");
    --m->live;
    free(p);
}
static void cases(void) {
    BHWorldGeometry w[6], *owner;
    BHWorldQuery q, bad, saved;
    BHEntityPool pool;
    BHEntity *a[4], *self, *inside, *touch;
    BHVec3 point = {0, 0, 0};
    Memory memory = {0, 0, 0};
    BHSceneAllocator allocator;
    unsigned long i, j, calls, word;
    BHWorldHit *old_hits;
    unsigned long old_count;
    allocator.allocate = allocate;
    allocator.release = release;
    allocator.user = &memory;
    ring(w, 6);
    w[0].minimum.x = -10;
    w[0].maximum.x = 0;
    w[3].minimum.x = 0;
    w[3].maximum.x = 10;
    w[5].minimum.x = 10;
    w[5].maximum.x = 20;
    w[1].owner = w;
    w[1].query_flags = 2;
    w[2].owner = w;
    w[2].query_flags = 0;
    w[4].owner = w + 3;
    w[4].query_flags = 2;
    w[4].minimum.x = 1;
    w[4].maximum.x = 2;
    port(w, 0, w + 3, 0, 0);
    port(w + 3, 0, w, 4, 0);
    port(w + 3, 1, w + 5, 0, 10);
    port(w + 5, 0, w + 3, 4, 10);
    CHECK(bh_entities_init(&pool, 3, NULL), "real source entity-pool dependency");
    self = bh_entity_acquire(&pool);
    inside = bh_entity_acquire(&pool);
    touch = bh_entity_acquire(&pool);
    CHECK(self && inside && touch, "real actor slots");
    if (!self || !inside || !touch) {
        bh_entities_dispose(&pool);
        return;
    }
    self->collision_radius = 1;
    inside->collision_radius = 0.5f;
    touch->collision_radius = 1;
    touch->position.x = 2;
    a[0] = touch;
    a[1] = inside;
    a[2] = self;
    a[3] = inside;
    for (i = 1; i <= 5; ++i) {
        memset(&bad, 0x5a, sizeof(bad));
        saved = bad;
        memory.fail_at = memory.calls + i;
        CHECK(!bh_world_query_init(&bad, w, 6, a, 4, &allocator) &&
                  !memcmp(&bad, &saved, sizeof(bad)) && !memory.live,
              "each workspace allocation failure leaves fresh output untouched");
    }
    memory.fail_at = 0;
    CHECK(bh_world_query_init(&q, w, 6, a, 4, &allocator) && memory.live == 5,
          "own source broad-phase workspace");
    calls = memory.calls;
    owner = NULL;
    CHECK(bh_world_query_run(&q, self, w, &point, 1, &owner) && owner == w && q.hit_count == 6,
          "room/child DFS then strict actor candidates");
    CHECK(q.hits[0].object.geometry == w && q.hits[1].object.geometry == w + 1 &&
              q.hits[2].object.geometry == w + 3 && q.hits[3].object.geometry == w + 4,
          "original room/eligible-child/portal depth-first order");
    CHECK(q.hits[4].kind == BH_WORLD_HIT_ACTOR && q.hits[4].object.actor == inside &&
              q.hits[5].object.actor == inside && q.hits[6].kind == BH_WORLD_HIT_END,
          "self/touch exclusion, caller duplicates and terminator retained");
    for (i = 0; i < 6; ++i)
        CHECK(!((unsigned char *)q.visited)[i], "visitation cleared after collection");
    for (i = 0; i < 4; ++i)
        CHECK(bh_world_query_run(&q, self, w, &point, 1, &owner) && q.hit_count == 6 &&
                  memory.calls == calls,
              "repeat query allocates nothing and resets visitation");
    point.y = 1.5f;
    CHECK(bh_world_query_run(&q, self, w, &point, 0.1f, &owner) &&
              q.hits[2].object.geometry == w + 3,
          "raw aperture drives traversal despite narrower clipped/scratch extents");
    point.x = 50;
    point.y = 0;
    CHECK(bh_world_query_run(&q, self, w, &point, 1, &owner) && owner == w && q.hit_count == 1,
          "outside scene retains previous owner and its room candidate");
    point.x = 20.5f;
    CHECK(bh_world_query_run(&q, self, NULL, &point, 1, &owner) && owner == w + 5,
          "expanded lookup fallback finds owner beyond point bounds");
    point.x = 0;
    CHECK(bh_world_query_run(&q, self, w, &point, -1, &owner) && q.hit_count == 2 &&
              q.hits[1].object.geometry == w + 1,
          "negative margin keeps enclosing child but rejects portal and actors");
    old_hits = q.hits;
    old_count = q.hit_count;
    w[0].navigation.next = &w[3].navigation;
    owner = w + 5;
    CHECK(!bh_world_query_run(&q, self, w, &point, 1, &owner) && q.hits == old_hits &&
              q.hit_count == old_count && owner == w + 5,
          "topology mutation preserves prior result and owner output");
    w[0].navigation.next = &w[1].navigation;
    word = 0x7fc00000UL;
    memcpy(&inside->collision_radius, &word, 4);
    CHECK(!bh_world_query_run(&q, self, w, &point, 1, &owner) && q.hits == old_hits,
          "nonfinite actor extent rejected without result replacement");
    inside->collision_radius = 0.5f;
    CHECK(bh_world_query_dispose(&q) && !memory.live,
          "workspace disposed before borrowed entities");
    inside->position.x = 50;
    a[0] = inside;
    CHECK(bh_world_query_init(&q, NULL, 0, a, 1, NULL),
          "actor-only query has real empty-scene ownership");
    point.x = 50;
    CHECK(bh_world_query_run(&q, NULL, NULL, &point, 1, &owner) && !owner && q.hit_count == 1 &&
              q.hits[0].object.actor == inside,
          "no static owner still appends dynamic actors");
    CHECK(bh_world_query_dispose(&q), "actor-only workspace disposal");
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 2; ++j) {
            ring(w, 2);
            port(w, 0, w + 1, i | (j ? 4 : 0), 0);
            port(w + 1, 0, w, (i | (j ? 4 : 0)) ^ 4, 0);
            CHECK(bh_world_query_init(&q, w, 2, NULL, 0, NULL), "axis portal query setup");
            point.x = point.y = point.z = 0;
            CHECK(bh_world_query_run(&q, NULL, w, &point, 0, &owner) && q.hit_count == 2,
                  "all six axis codes include exact portal plane");
            ((float *)&point)[i] = 0.5f;
            CHECK(bh_world_query_run(&q, NULL, w, &point, 0.25f, &owner) && q.hit_count == 1,
                  "portal plane rejection in every axis");
            CHECK(bh_world_query_dispose(&q), "axis query cleanup");
        }
    CHECK(bh_entities_dispose(&pool), "actor pool lifetime closes after queries");
}
int bh_world_query_checkpoint(unsigned long *count) {
    checks = 0;
    failures = 0;
    cases();
    *count = checks;
    printf("{\"scenario\":\"world-query\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false}\n",
           checks, failures);
    return failures ? 1 : 0;
}
