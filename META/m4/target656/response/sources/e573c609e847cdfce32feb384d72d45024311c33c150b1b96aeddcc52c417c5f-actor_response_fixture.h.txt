#include "../src/bh_callback_helpers.h"
#include "../src/bh_fp.h"
#include "../src/bh_round.h"
#include "../src/bh_target_construct.h"
#include "../src/bh_actor_damage.h"
#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned long checks, failed;
#define CHECK(x, s)                                                            \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x)) {                                                                \
      ++failed;                                                                \
      if (failed < 20)                                                         \
        printf("FAIL actor-damage %d: %s\n", __LINE__, s);                 \
    }                                                                          \
  } while (0)
static void identity(BHMat34 *m) {
  memset(m, 0, sizeof(*m));
  m->m[0][0] = m->m[1][1] = m->m[2][2] = 1;
}
typedef struct Fixture {
  BHEntityPool pool;
  BHEntity *e, *owner, *registry[4];
  BHDrawQueue queue;
  BHSoundQueue sound;
  BHDebrisContext debris;
  BHDebrisBody body;
  BHPhysicsState state;
  BHPhysicsStepContext physics;
  BHPhysicsStepBody pb;
  BHPhysicsEffects effects;
  BHContactEffects contact;
  BHGeometry *models[1];
  BHMat34 inverse;
  BHScriptDebrisContext fragments;
  BHFlashFlareContext flash;
  BHScriptExplosionContext explosions;
  BHScriptParticleContext particles;
  BHPickupContext pickups;
  BHPickupRenderBinding render;
  BHPickupDrawContext draw;
  BHEntityResourceDrawContext special;
  BHWorldGeometry world;
  BHPickupRoom room;
  unsigned long resources[20];
  BHTargetConstructContext construct;
  BHTargetConstructSlot slots[4];
  BHPhysicsStepBody roster[4];
  BHGeometry *catalog[4];
  BHTargetDrawContext target_draw;
  BHMainCamera camera;
  BHViewConfig view;
  long mode;
} Fixture;
static int setup(Fixture *f) {
  BHVec3 inertia = {1, 1, 1};
  BHSoundResource *r;
  unsigned long i;
  memset(f, 0, sizeof(*f));
  if (!bh_entities_init(&f->pool, 32, NULL) ||
      !bh_draw_queue_init(&f->queue, 64, NULL) ||
      !bh_sound_queue_init(&f->sound, NULL, NULL, NULL))
    return 0;
  r = bh_sound_resource_allocate(&f->sound);
  if (!r)
    return 0;
  r->streaming = 1;
  r->base_frequency = 100;
  f->e = bh_entity_acquire(&f->pool);
  f->owner = bh_entity_acquire(&f->pool);
  f->owner->position.x = 1000;
  bh_entity_set_query(&f->pool, f->owner, bh_actor_target_removed);
  f->registry[0] = f->e;
  f->registry[1] = f->owner;
  identity(&f->state.pose);
  f->state.last_clear = f->state.pose;
  identity(&f->inverse);
  if (!bh_debris_context_init(&f->debris, &f->pool, &f->sound, r->index,
                              f->registry, 4) ||
      !bh_debris_bind(&f->debris, &f->body, f->e, &f->state, 2, 0, &inertia))
    return 0;
  f->body.owner = f->owner;
  f->pb.entity = f->e;
  f->pb.generation = f->e->generation;
  f->pb.physics = &f->state;
  f->pb.target_world = &f->state.pose;
  f->pb.target_local = &f->inverse;
  f->pb.mass = 2;
  if (!bh_contact_effects_init(&f->contact, &f->pool, &f->queue, 1, 2, 3, 4))
    return 0;
  f->effects.effects = &f->contact;
  f->effects.sound = &f->sound;
  if (!bh_physics_step_init(&f->physics, &f->pool, &f->pb, 1, NULL, 0,
                            f->models, 1, &f->effects, 1, NULL))
    return 0;
  f->fragments.pool = &f->pool;
  f->fragments.queue = &f->queue;
  f->fragments.image = 9;
  f->flash.pool = &f->pool;
  f->flash.queue = &f->queue;
  f->flash.textures[0] = 10;
  f->explosions.pool = &f->pool;
  f->explosions.queue = &f->queue;
  f->explosions.sound = &f->sound;
  f->explosions.debris = &f->fragments;
  f->explosions.flash = &f->flash;
  f->explosions.sound_resource = r->index;
  f->explosions.image = 11;
  if (!bh_script_particles_init(&f->particles, &f->pool, &f->queue))
    return 0;
  f->world.navigation.next = &f->world.navigation;
  f->world.minimum.x = f->world.minimum.y = f->world.minimum.z = -100;
  f->world.maximum.x = f->world.maximum.y = f->world.maximum.z = 100;
  identity(&f->world.transform);
  f->room.world = &f->world;
  for (i = 0; i < 20; ++i)
    f->resources[i] = i + 1;
  f->pickups.pool = &f->pool;
  f->pickups.first_world = &f->world;
  f->pickups.world_count = 1;
  f->pickups.rooms = &f->room;
  f->pickups.room_count = 1;
  f->pickups.resources = f->resources;
  f->pickups.resource_count = 20;
  f->pickups.outside_actors = f->registry;
  f->pickups.sound = &f->sound;
  f->draw.queue = &f->queue;
  f->special.queue = &f->queue;
  f->render.default_draw = &f->draw;
  f->render.special_draw = &f->special;
  if (!bh_pickup_render_bind(&f->pickups, &f->render))
    return 0;
  f->target_draw.queue = &f->queue;
  f->target_draw.camera = &f->camera;
  f->target_draw.view = &f->view;
  f->target_draw.screen_width = 640;
  f->target_draw.player = f->owner;
  f->construct.debris = &f->debris;
  f->construct.physics = &f->physics;
  f->construct.effects = &f->effects;
  f->construct.explosions = &f->explosions;
  f->construct.particles = &f->particles;
  f->construct.pickups = &f->pickups;
  f->construct.draw = &f->target_draw;
  f->construct.mode = &f->mode;
  f->construct.slots = f->slots;
  f->construct.capacity = 4;
  f->construct.roster = f->roster;
  f->construct.models = f->catalog;
  f->construct.resources = f->resources;
  f->construct.resource_count = 20;
  f->construct.prefix = "actor-damage-fixture\\";
  f->construct.search = BH_BSP_PREFIX_ONLY;
  if (!bh_physics_step_dispose(&f->physics) || !bh_debris_unbind(&f->body) ||
      !bh_entity_release(&f->pool, f->e))
    return 0;
  f->registry[0] = NULL;
  f->e = NULL;
  return 1;
}
static int model_file(void) {
  BHCollisionModel model;
  BHVec3 extent = {2, 4, 6}, padding = {1, 2, 3};
  FILE *f;
  unsigned long i;
  const void *arrays[6];
  static const unsigned long sizes[6] = {32, 60, 16, 12, 56, 8};
  _mkdir("actor-damage-fixture");
  f = fopen("actor-damage-fixture/HUN_LOD1.BSP", "rb");
  if (f) {
    fclose(f);
    return 0;
  }
  if (!bh_collision_model_init(&model, NULL) ||
      !bh_collision_box_build(&model, &extent, &padding))
    return 0;
  arrays[0] = model.bsp.materials;
  arrays[1] = model.bsp.vertices;
  arrays[2] = model.bsp.edges;
  arrays[3] = model.bsp.faces;
  arrays[4] = model.bsp.nodes;
  arrays[5] = model.bsp.leaves;
  f = fopen("actor-damage-fixture/HUN_LOD1.BSP", "wb");
  if (!f)
    return 0;
  for (i = 0; i < 6; ++i) {
    fwrite(model.bsp.counts + i, 4, 1, f);
    fwrite(arrays[i], sizes[i], model.bsp.counts[i], f);
  }
  fclose(f);
  return bh_collision_model_dispose(&model);
}
static void finish(Fixture *f) {
  unsigned long i;
  for (i = 0; i < 4; ++i)
    if (f->slots[i].body.entity)
      bh_debris_remove(f->slots[i].body.entity);
  CHECK(bh_target_construct_dispose(&f->construct),
        "real collision and dynamic physics owners disposed");
  while (f->pickups.head)
    bh_pickup_cleanup(f->pickups.head);
  while (f->pool.active)
    bh_entity_release(&f->pool, f->pool.active);
  CHECK(bh_debris_context_dispose(&f->debris) &&
            bh_contact_effects_dispose(&f->contact) &&
            bh_sound_queue_dispose(&f->sound) &&
            bh_draw_queue_dispose(&f->queue) && bh_entities_dispose(&f->pool),
        "actual dependencies release all ownership");
}
