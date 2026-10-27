#include "../src/bh_physics_step.h"
#include "../src/bh_debris_response.h"
#include "../src/bh_fp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks;
static int failed;
#define CHECK(x, label)                                                                            \
    do {                                                                                           \
        ++checks;                                                                                  \
        if (!(x)) {                                                                                \
            ++failed;                                                                              \
            printf("FAIL physics step %d: %s\n", __LINE__, label);                                 \
        }                                                                                          \
    } while (0)
static void section(unsigned char *data, unsigned long *at, unsigned long count, const void *p,
                    unsigned long size) {
    memcpy(data + *at, &count, 4);
    *at += 4;
    memcpy(data + *at, p, count * size);
    *at += count * size;
}
static unsigned long fixture(unsigned char *data, int destination) {
    BHBspMaterial material;
    BHBspVertex vertices[4];
    BHBspEdge edges[4];
    BHBspFace faces[3];
    BHBspNode nodes[3];
    BHBspLeaf leaves[3];
    unsigned long at = 0, i;
    memset(&material, 0, sizeof(material));
    strcpy(material.texture, "STONE.PCX");
    memset(vertices, 0, sizeof(vertices));
    memset(edges, 0, sizeof(edges));
    memset(faces, 0, sizeof(faces));
    memset(nodes, 0, sizeof(nodes));
    for (i = 0; i < 4; ++i) {
        vertices[i].y = (i == 0 || i == 3) ? -2 : 2;
        if (destination)
            vertices[i].z = i < 2 ? -1 : 1;
        else
            vertices[i].x = i < 2 ? -1 : 1;
        edges[i].vertex = i;
        edges[i].next = i == 3 ? -1 : i + 1;
    }
    for (i = 0; i < 3; ++i) {
        faces[i].first_edge = 0;
        nodes[i].minimum[0] = destination ? 0 : -1;
        nodes[i].maximum[0] = destination ? 0 : 1;
        nodes[i].minimum[1] = -2;
        nodes[i].maximum[1] = 2;
        nodes[i].minimum[2] = destination ? -1 : 0;
        nodes[i].maximum[2] = destination ? 1 : 0;
        nodes[i].normal[destination ? 0 : 2] = 1;
        nodes[i].front_leaf = i;
        nodes[i].back_leaf = -1;
        nodes[i].front_node = nodes[i].back_node = -1;
        leaves[i].face = i;
        leaves[i].next = -1;
    }
    section(data, &at, 1, &material, sizeof(material));
    section(data, &at, 4, vertices, sizeof(vertices[0]));
    section(data, &at, 4, edges, sizeof(edges[0]));
    section(data, &at, 3, faces, sizeof(faces[0]));
    section(data, &at, 3, nodes, sizeof(nodes[0]));
    section(data, &at, 3, leaves, sizeof(leaves[0]));
    return at;
}
static void identity(BHMat34 *m) {
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}

typedef struct Memory {
    unsigned long count, live, limit;
} Memory;
static void *__cdecl allocate(unsigned long n, void *u) {
    Memory *m = u;
    void *p;
    if (++m->count == m->limit)
        return NULL;
    p = malloc(n);
    if (p)
        ++m->live;
    return p;
}
static void __cdecl release(void *p, void *u) {
    Memory *m = u;
    if (p) {
        --m->live;
        free(p);
    }
}
typedef struct Fixture {
    BHEntityPool pool;
    BHEntity *entities[3];
    BHPhysicsState state[2];
    BHPhysicsStepBody bodies[2];
    BHPhysicsStepContext step;
    BHPhysicsStepWorld worlds[2];
    BHWorldGeometry world[2];
    BHGeometryModel model;
    BHCollisionModel source, target;
    BHGeometry *models[2];
    BHMat34 target_world[2], target_local[2];
    BHDrawQueue queue;
    BHContactEffects effects;
    BHSoundQueue sound;
    BHNetworkTransport network;
    BHPhysicsEffects feedback;
    BHDebrisContext debris;
    BHDebrisBody debris_body[2];
    Memory memory;
} Fixture;
static void reset(Fixture *f) {
    unsigned long i;
    for (i = 0; i < 2; ++i) {
        memset(f->state + i, 0, sizeof(f->state[i]));
        identity(&f->state[i].pose);
        f->state[i].pose.m[0][3] = i ? 0 : 3;
        f->state[i].last_clear = f->state[i].pose;
        f->state[i].velocity.x = i ? 0 : -2.125f;
        f->debris_body[i].sleep_time = 3;
    }
}
static int setup(Fixture *f, int actors, int worlds) {
    unsigned char bytes[2048];
    unsigned long i, size;
    BHSoundResource *r;
    BHVec3 inertia = {1, 1, 1};
    BHSceneAllocator mem;
    memset(f, 0, sizeof(*f));
    if (!bh_entities_init(&f->pool, 32, NULL) || !bh_draw_queue_init(&f->queue, 64, NULL) ||
        !bh_sound_queue_init(&f->sound, NULL, NULL, NULL) ||
        !bh_network_transport_init(&f->network, NULL, 0) ||
        !bh_contact_effects_init(&f->effects, &f->pool, &f->queue, 1, 2, 3, 4))
        return 0;
    for (i = 0; i < 3; ++i) {
        r = bh_sound_resource_allocate(&f->sound);
        if (!r)
            return 0;
        r->streaming = 1;
        r->base_frequency = 100;
    }
    f->feedback.effects = &f->effects;
    f->feedback.sound = &f->sound;
    f->feedback.transport = &f->network;
    f->feedback.impact_sound = 0;
    f->feedback.quad_sound = 1;
    f->feedback.rays_sound = 2;
    f->feedback.peer_count = 2;
    f->feedback.sender = 17;
    if (!bh_collision_model_init(&f->source, NULL) || !bh_collision_model_init(&f->target, NULL))
        return 0;
    size = fixture(bytes, 0);
    if (!bh_collision_model_decode(&f->source, bytes, size))
        return 0;
    size = fixture(bytes, 1);
    if (!bh_collision_model_decode(&f->target, bytes, size))
        return 0;
    if (!bh_geometry_model_init(&f->model, &f->target.geometry, 1, 0))
        return 0;
    f->models[0] = &f->target.geometry;
    f->models[1] = NULL;
    for (i = 0; i < 2; ++i) {
        f->world[i].model = &f->model;
        f->world[i].query_flags = 2;
        identity(&f->world[i].transform);
        f->world[i].transform.m[0][3] = i ? 1 : 0;
        f->world[i].minimum.x = f->world[i].minimum.y = f->world[i].minimum.z = -20;
        f->world[i].maximum.x = f->world[i].maximum.y = f->world[i].maximum.z = 20;
        f->world[i].navigation.next = &f->world[(i + 1) % ((worlds > 0) ? worlds : 1)].navigation;
        f->worlds[i].world = f->world + i;
        f->worlds[i].mass = -1;
        f->worlds[i].index = (unsigned short)(73 + i);
    }
    for (i = 0; i < (unsigned long)actors; ++i) {
        f->entities[i] = bh_entity_acquire(&f->pool);
        if (!f->entities[i])
            return 0;
        f->entities[i]->collision_radius = 4;
        identity(f->target_world + i);
        identity(f->target_local + i);
        f->bodies[i].entity = f->entities[i];
        f->bodies[i].generation = f->entities[i]->generation;
        f->bodies[i].physics = f->state + i;
        f->bodies[i].geometry = i ? &f->target.geometry : &f->source.geometry;
        f->bodies[i].model_index = 0;
        f->bodies[i].target_world = f->target_world + i;
        f->bodies[i].target_local = f->target_local + i;
        f->bodies[i].mass = 1;
        f->bodies[i].friction = .5f;
        f->bodies[i].flags = 0;
        f->bodies[i].world = worlds ? f->world : NULL;
    }
    reset(f);
    if (!bh_debris_context_init(&f->debris, &f->pool, &f->sound, 0, f->entities, 3))
        return 0;
    for (i = 0; i < (unsigned long)actors; ++i)
        if (!bh_debris_bind(&f->debris, f->debris_body + i, f->entities[i], f->state + i, 1, .5f,
                            &inertia))
            return 0;
    mem.allocate = allocate;
    mem.release = release;
    mem.user = &f->memory;
    return bh_physics_step_init(&f->step, &f->pool, f->bodies, actors, f->worlds, worlds, f->models,
                                2, &f->feedback, 1, &mem);
}
static void finish(Fixture *f) {
    unsigned long i;
    CHECK(bh_physics_step_dispose(&f->step) && !f->memory.live,
          "bridge releases all owning workspaces");
    for (i = 0; i < 2; ++i)
        if (f->debris_body[i].entity)
            CHECK(bh_debris_unbind(f->debris_body + i), "real query owner unbound");
    CHECK(bh_debris_context_dispose(&f->debris), "query context lifetime");
    while (f->pool.active)
        CHECK(bh_entity_release(&f->pool, f->pool.active), "real actor/effect release");
    CHECK(bh_contact_effects_dispose(&f->effects) && bh_entities_dispose(&f->pool),
          "effect context and pool disposed");
    CHECK(bh_sound_queue_dispose(&f->sound) && bh_network_transport_dispose(&f->network) &&
              bh_draw_queue_dispose(&f->queue),
          "sound/transport/queue source owners disposed");
    CHECK(bh_collision_model_dispose(&f->source) && bh_collision_model_dispose(&f->target),
          "borrowed decoded models disposed last");
}
static void empty_cases(void) {
    Fixture f;
    BHPhysicsState saved, view;
    unsigned long allocations;
    int next;
    CHECK(setup(&f, 1, 0), "real empty-world registry");
    allocations = f.memory.count;
    f.state[0].velocity.x = 2;
    f.state[0].acceleration.x = 4;
    f.state[0].angular_velocity.z = 1;
    f.state[0].angular_acceleration.z = 2;
    srand(1);
    next = rand();
    srand(1);
    CHECK(bh_physics_step(&f.step, f.entities[0], .5f) && f.step.error == BH_PHYSICS_STEP_OK,
          "complete no-candidate owner");
    CHECK(f.step.misses == 1 && f.step.steps == 1 && !f.step.contacts &&
              f.state[0].pose.m[0][3] == 5 && f.state[0].velocity.x == (float)3.98,
          "independent integrated position and differentiated .995 velocity");
    CHECK(f.state[0].angular_velocity.z == (float)1.96 &&
              f.state[0].acceleration.y == (float)9.82 && f.state[0].acceleration.x == 0 &&
              !memcmp(&f.state[0].last_clear, &f.state[0].pose, 48),
          "last clear pose, force clearing, gravity and spin damping");
    CHECK(rand() == next && f.memory.count == allocations && !f.feedback.packets &&
              !f.feedback.sound_requests,
          "miss performs no random/effect/packet or bridge allocation");
    CHECK(bh_physics_step_read_entity(&view, f.entities[0]) &&
              !memcmp(&view, f.state, sizeof(view)),
          "real common-field publish/read roundtrip");
    saved = f.state[0];
    CHECK(!bh_physics_step(&f.step, f.entities[0], 0) && !memcmp(&saved, f.state, sizeof(saved)),
          "invalid dt rejects before mutation");
    f.step.busy = 1;
    CHECK(!bh_physics_step(&f.step, f.entities[0], 1) && f.step.error == BH_PHYSICS_STEP_BUSY,
          "reentrant source owner guard");
    f.step.busy = 0;
    finish(&f);
}
static void contact_cases(void) {
    Fixture f;
    unsigned long flags, i, initial;
    float expected;
    int next;
    for (flags = 0; flags <= 8; flags += 2) {
        CHECK(setup(&f, 1, 1), "real static decoded collision bridge");
        for (i = 0; i < 3; ++i)
            CHECK(bh_geometry_set_flags(&f.target.geometry, i, flags),
                  "real per-face material flag");
        initial = f.memory.count;
        srand(1);
        CHECK(bh_physics_step(&f.step, f.entities[0], 1), "complete static contact owner");
        CHECK(f.step.contacts == 1 && f.step.first.surface_present &&
                  f.step.first.fraction == .875f && f.state[0].pose.m[0][3] == 1.140625f,
              "real first sweep and actual slide pose");
        CHECK(f.step.first_events == 1 && !f.step.second_events &&
                  f.debris_body[0].sleep_time == 0 && f.state[0].acceleration.x > 30,
              "actual event2 reaches real debris callback and wakes body");
        CHECK(f.step.fallback == 1 && f.step.selected.kind == BH_WORLD_HIT_GEOMETRY &&
                  f.step.selected.object.geometry == f.world && f.bodies[0].world == f.world,
              "fallback restored and actual world identity retained");
        CHECK(f.memory.count == initial && !f.network.native_calls,
              "step bridge never allocates or sends native network");
        expected = (flags & 6) ? 0 : (float)(2.125 * .08);
        if (flags & 6) {
            CHECK(f.feedback.sound_requests == 1 && !f.feedback.packets &&
                      !f.feedback.spark_attempts && f.pool.used == 2,
                  "special contact allocates real impact before sound");
            CHECK(f.feedback.last_voice != BH_SOUND_INVALID, "actual special source sound queued");
        } else {
            CHECK(f.feedback.packets == 1 && f.feedback.last_packet_size == 20 &&
                      f.feedback.last_packet[8] == 73 && f.feedback.last_packet[9] == 0 &&
                      !f.feedback.sound_requests && !f.feedback.sparks_created,
                  "quiet normal packet encodes actual world registry index, no playback");
            CHECK(f.feedback.last_packet[16] == 255 && f.feedback.last_packet[18] == 64,
                  "low-level normal packet sentinel");
        }
        srand(1);
        rand();
        rand();
        rand();
        next = rand();
        srand(1);
        reset(&f);
        CHECK(bh_physics_step(&f.step, f.entities[0], 1) && rand() == next,
              "normal and special low-level contacts consume exactly three response randoms");
        finish(&f);
    }
}
static void world_role_cases(void) {
    Fixture f;
    BHEntity *node;
    CHECK(setup(&f, 1, 1), "disabled world owner fixture");
    f.world[0].query_flags = 0;
    CHECK(bh_physics_step(&f.step, f.entities[0], 1) && f.step.misses == 1 &&
              !f.step.contacts && f.state[0].pose.m[0][3] == 3,
          "world-only enable gate retains original untouched single-candidate pose");
    finish(&f);
    CHECK(setup(&f, 1, 1), "moving world owner fixture");
    node = bh_entity_acquire(&f.pool);
    CHECK(node != NULL, "real source world response entity");
    node->kind = 1;
    node->origin.x = .125f;
    f.worlds[0].response = node;
    CHECK(bh_physics_step(&f.step, f.entities[0], 1) && f.step.contacts == 1 &&
              f.step.first.fraction == .875f && f.step.first.motion.x == -3.375f,
          "world response translation reaches relative sweep with original1.5 scaling");
    CHECK(f.step.candidates[0].movement.x == .125f && f.step.candidates[0].flags == 2,
          "world candidate binds live source translation and query flags");
    finish(&f);
}
static void actor_cases(void) {
    Fixture f;
    double dx, dy, dz, impulse, expected;
    CHECK(setup(&f, 2, 0), "actual two-actor registry");
    CHECK(f.step.pair_count == 4, "each pair of actual physical-body meshes owns one sweep");
    f.state[0].pose.m[1][3] = .5f;
    f.state[0].velocity.x = -4.25f;
    f.state[0].angular_velocity.x = 1;
    f.state[1].angular_velocity.x = 1;
    srand(1);
    CHECK(bh_physics_step(&f.step, f.entities[0], .5f) && f.step.contacts == 1,
          "actual target model catalog and actor sweep");
    CHECK(f.step.first_events == 1 && f.step.second_events == 1 &&
              f.debris_body[0].sleep_time == 0 && f.debris_body[1].sleep_time == 0,
          "both ordered event2 deliveries use actual installed debris owners");
    CHECK(f.state[0].acceleration.x > 30 && f.state[1].acceleration.x < 0,
          "real contact response changes both shared physics components");
    dx = (double)f.step.first.surface.contact.point.x - 3;
    dy = (double)f.step.first.surface.contact.point.y - .5;
    dz = f.step.first.surface.contact.point.z;
    impulse = (double)f.step.first.motion.x * .5;
    expected = (dy / sqrt(dx * dx + dy * dy + dz * dz)) * impulse * 270;
    CHECK(fabs(f.state[0].angular_acceleration.z - expected) < .0001,
          "callback lever arm uses OLD actor pose before local collision pose is committed");
    CHECK(fabs(f.state[0].angular_velocity.x - .85 * .98) < .000001 &&
              f.state[1].angular_velocity.x == (float).85,
          "real callback spin changes survive final first-body damping");
    CHECK(f.debris_body[0].last_sound != BH_SOUND_INVALID &&
              f.debris_body[1].last_sound != BH_SOUND_INVALID &&
              bh_sound_voice(&f.sound, f.debris_body[0].last_sound)->index <
                  bh_sound_voice(&f.sound, f.debris_body[1].last_sound)->index,
          "actual source sound voice sequence proves first callback precedes second");
    CHECK(f.step.selected.kind == BH_WORLD_HIT_ACTOR &&
              f.step.selected.object.actor == f.entities[1] && !f.feedback.packets,
          "actor identity retained; NULL current world suppresses only packet");
    finish(&f);
    CHECK(setup(&f, 2, 0), "overlap fixture");
    f.state[0].pose.m[0][3] = 0;
    f.state[0].last_clear.m[0][3] = 7;
    f.state[0].velocity.x = -2.125f;
    srand(1);
    CHECK(bh_physics_step(&f.step, f.entities[0], 1), "zero fraction actor overlap owner");
    CHECK(f.step.first.fraction == 0 && f.step.rollbacks == 1 && !f.step.contacts &&
              f.state[0].pose.m[0][3] == 7 && f.state[0].last_clear.m[0][3] == 7,
          "real original-position overlap rolls back and retains last-clear snapshot");
    CHECK(rand() == 41 && !f.step.first_events && !f.step.second_events &&
              !f.feedback.sound_requests,
          "overlap rollback precedes random effects and query delivery");
    finish(&f);
    CHECK(setup(&f, 2, 0), "translated rotated actor with stale borrowed inverse");
    f.state[0].pose.m[0][3] = 0;
    f.state[0].pose.m[1][3] = 3;
    f.state[0].velocity.x = 0;
    f.state[0].velocity.y = -2.125f;
    f.state[1].pose.m[0][0] = f.state[1].pose.m[1][1] = 0;
    f.state[1].pose.m[0][1] = -1;
    f.state[1].pose.m[1][0] = 1;
    f.state[1].pose.m[1][3] = .25f;
    CHECK(bh_physics_step(&f.step, f.entities[0], 1) && f.step.contacts == 1 &&
              f.step.first.surface.normal.y == 1 && f.state[0].pose.m[1][3] == 2.46875f,
          "physical actor sweep rebuilds pose/inverse from current authoritative body");
    CHECK(f.step.candidates[0].world.m[1][3] == .25f &&
              f.step.candidates[0].local.m[0][3] == -.25f &&
              f.target_local[1].m[0][3] == 0,
          "candidate owns fresh inverse rather than unchanged caller identity matrix");
    finish(&f);
    CHECK(setup(&f, 1, 0), "lifetime fixture");
    CHECK(bh_debris_unbind(f.debris_body) && bh_entity_release(&f.pool, f.entities[0]),
          "real bound owner released through proper lifetime");
    CHECK(!bh_physics_step(&f.step, f.entities[0], 1) && f.step.error == BH_PHYSICS_STEP_BINDING,
          "stale registry generation rejected");
    finish(&f);
}
static void reordered_case(void) {
    Fixture f;
    CHECK(setup(&f, 1, 2), "two real wall placement registry");
    f.world[1].owner = f.world;
    f.world[1].query_flags = 2;
    identity(&f.world[1].transform);
    f.world[1].transform.m[0][0] = f.world[1].transform.m[1][1] = 0;
    f.world[1].transform.m[0][1] = -1;
    f.world[1].transform.m[1][0] = 1;
    f.state[0].pose.m[1][3] = 4;
    f.state[0].velocity.y = -3;
    f.bodies[0].friction = 4;
    f.debris_body[0].friction = 4;
    CHECK(bh_physics_step(&f.step, f.entities[0], 1), "whole mixed-normal first/slide dispatch");
    CHECK(f.step.candidate_count == 2 && f.step.first_selected.object.geometry == f.world + 1 &&
              f.step.selected.object.geometry == f.world && f.step.first.surface.normal.y == 1 &&
              f.step.slide.surface.normal.x == 1,
          "second slide reorders current contact identity while preserving first surface");
    CHECK(f.step.fallback == 1 && f.step.contacts == 1 && f.step.first_events == 1 &&
              f.feedback.last_packet[8] == 73,
          "slide reorder preserves fallback and contact-world packet registry");
    finish(&f);
}
static void loud_case(void) {
    Fixture f;
    int next, i;
    BHSoundVoice *voice;
    CHECK(setup(&f, 1, 1), "loud whole-owner fixture");
    f.state[0].velocity.x = -42.5f;
    srand(1);
    for (i = 0; i < 30; ++i)
        rand();
    next = rand();
    srand(1);
    CHECK(bh_physics_step(&f.step, f.entities[0], .05f) && f.step.contacts == 1 &&
              f.feedback.sound_requests == 1 && f.feedback.sparks_created == 3 && rand() == next,
          "clamped full-owner loud response consumes3 response+27 particle CRT draws");
    voice = bh_sound_voice(&f.sound, f.feedback.last_voice);
    CHECK(voice && voice->frequency == 110 && f.feedback.last_packet[16] == 0 &&
              f.feedback.last_packet[17] == 128 && f.feedback.last_packet[18] == 70 &&
              f.feedback.last_packet[19] == 3,
          "full-owner actual sound and clamped loud packet quantization");
    CHECK(f.step.first_events == 1 &&
              bh_sound_voice(&f.sound, f.debris_body[0].last_sound)->index > voice->index,
          "whole-owner local audio precedes actual first event callback sound");
    finish(&f);
    CHECK(setup(&f, 1, 1), "real ignored sound-request failure fixture");
    f.state[0].velocity.x = -42.5f;
    f.feedback.impact_sound = BH_SOUND_INVALID;
    CHECK(bh_physics_step(&f.step, f.entities[0], .05f) && f.step.error == BH_PHYSICS_STEP_EFFECT &&
              f.feedback.sparks_created == 3 && f.step.first_events == 1 &&
              f.debris_body[0].sleep_time == 0,
          "actual failed sound request retains particles, event delivery and final pose as "
          "original ignored result");
    finish(&f);
}
static void allocation_cases(void) {
    Fixture f;
    BHPhysicsStepContext c, saved;
    BHEntity saved_entity;
    BHSceneAllocator a;
    unsigned long limit;
    CHECK(setup(&f, 1, 1), "allocation controls fixture");
    CHECK(bh_physics_step_dispose(&f.step), "release first binding");
    a.allocate = allocate;
    a.release = release;
    a.user = &f.memory;
    saved_entity = *f.entities[0];
    f.state[0].pose.m[0][3] = 8;
    for (limit = 1; limit <= 10; ++limit) {
        memset(&c, 0xa5, sizeof(c));
        saved = c;
        f.memory.count = 0;
        f.memory.limit = limit;
        CHECK(!bh_physics_step_init(&c, &f.pool, f.bodies, 1, f.worlds, 1, f.models, 2, &f.feedback,
                                    1, &a) &&
                  !f.memory.live && !memcmp(&c, &saved, sizeof(c)) &&
                  !memcmp(f.entities[0], &saved_entity, sizeof(saved_entity)),
              "each bridge allocation failure releases real partial owners and preserves output");
    }
    f.memory.limit = 0;
    CHECK(bh_physics_step_init(&f.step, &f.pool, f.bodies, 1, f.worlds, 1, f.models, 2, &f.feedback,
                               1, &a),
          "successful binding after allocation controls");
    finish(&f);
}
int __cdecl bh_physics_step_checkpoint(unsigned long *count) {
    BHMathEnvironment fp;
    checks = 0;
    failed = 0;
    if (!bh_math_environment_enter(&fp))
        return 2;
    empty_cases();
    contact_cases();
    world_role_cases();
    actor_cases();
    reordered_case();
    loud_case();
    allocation_cases();
    bh_math_environment_leave(&fp);
    *count = checks;
    printf("{\"scenario\":\"physics-step\",\"checks\":%lu,\"failures\":%d,\"original_code_"
           "required\":false,\"play_calls\":0,\"native_send_calls\":0}\n",
           checks, failed);
    return failed;
}
#ifdef BH_PHYSICS_STEP_STANDALONE
int main(void) {
    unsigned long count;
    return bh_physics_step_checkpoint(&count);
}
#endif
