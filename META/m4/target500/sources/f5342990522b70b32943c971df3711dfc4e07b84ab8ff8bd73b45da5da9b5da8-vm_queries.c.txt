#include "../src/bh_fp.h"
#include "../src/bh_vm_queries.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks, failures;
#define CHECK(x, s)                                                            \
  do {                                                                         \
    ++checks;                                                                  \
    if (!(x)) {                                                                \
      ++failures;                                                              \
      if (failures < 15)                                                       \
        printf("FAIL vm-queries %d %s\n", __LINE__, s);                        \
    }                                                                          \
  } while (0)
static void damage_cases(void) {
  BHEntityPool pool;
  BHEntity *block, *player, *other;
  BHWorldGeometry world;
  BHBlockDamageContext context;
  BHBlockDamageState state;
  BHBlockDirectDamagePacket direct;
  BHPhysicsContactPacket contact;
  BHDebrisBlastPacket blast;
  BHDebrisOwnerPacket detach;
  long ignored;
  unsigned long i;
  float nan;
  unsigned long nanbits = 0x7fc01234;
  CHECK(bh_entities_init(&pool, 8, NULL), "actual pool");
  block = bh_entity_acquire(&pool);
  player = bh_entity_acquire(&pool);
  other = bh_entity_acquire(&pool);
  memset(&world, 0, sizeof(world));
  world.minimum.x = -2;
  world.minimum.y = -4;
  world.minimum.z = -6;
  world.maximum.x = 2;
  world.maximum.y = 4;
  world.maximum.z = 6;
  context.world = &world;
  context.player = player;
  state.health = 100;
  state.owner = NULL;
  CHECK(bh_block_damage_bind(&pool, block, &context, &state, 0) &&
            block->query == bh_block_damage_query,
        "whole block damage query installed against actual world");
  direct.event = 0;
  direct.amount = .25f;
  direct.source = other;
  block->query(block, &direct);
  bh_block_damage_read(block, &state);
  CHECK(state.health == 102.5f && state.owner == other,
        "exact original negative10 multiplier and direct source attribution");
  memset(&contact, 0, sizeof(contact));
  contact.event = 2;
  contact.impulse = 999;
  contact.severity = .125f;
  block->query(block, &contact);
  bh_block_damage_read(block, &state);
  CHECK(state.health == 103.75f && state.owner == other,
        "contact reads severity+20 and preserves last attributed owner");
  detach.event = 17;
  detach.owner = player;
  block->query(block, &detach);
  bh_block_damage_read(block, &state);
  CHECK(state.owner == other, "foreign detach leaves owner");
  detach.owner = other;
  block->query(block, &detach);
  bh_block_damage_read(block, &state);
  CHECK(!state.owner, "matching detach clears actual owner state");
  state.health = 0;
  state.owner = other;
  bh_block_damage_bind(&pool, block, &context, &state, 0);
  blast.event = 1;
  blast.center.x = 3;
  blast.center.y = 0;
  blast.center.z = 0;
  blast.strength = 100;
  blast.radius = 10;
  block->query(block, &blast);
  bh_block_damage_read(block, &state);
  CHECK(fabs(state.health - .056) < .0000001 && state.owner == other,
        "radial midpoint distance and exact negative.0008 scale, no source "
        "attribution change");
  for (i = 0; i < 4; ++i) {
    float before = state.health;
    memcpy(&nan, &nanbits, 4);
    blast.center.x = i == 0 ? 0 : i == 1 ? 10 : i == 2 ? 11 : nan;
    block->query(block, &blast);
    bh_block_damage_read(block, &state);
    CHECK(state.health == before, "zero, exact-radius, outside and unordered "
                                  "distance skip radial branch");
  }
  for (i = 3; i < 25; ++i)
    if (i != 17) {
      float before = state.health;
      ignored = i;
      block->query(block, &ignored);
      bh_block_damage_read(block, &state);
      CHECK(state.health == before && state.owner == other,
            "all remaining positive events are unchanged");
    }
  state.health = 20;
  state.owner = NULL;
  CHECK(bh_block_damage_bind(&pool, block, &context, &state, 1) &&
            block->query == bh_block_player_damage_query,
        "whole player-only block query installed");
  direct.source = other;
  direct.amount = 1;
  block->query(block, &direct);
  bh_block_damage_read(block, &state);
  CHECK(state.health == 20 && !state.owner, "nonplayer direct damage excluded");
  direct.source = player;
  block->query(block, &direct);
  bh_block_damage_read(block, &state);
  CHECK(state.health == 30 && state.owner == player,
        "actual current-player identity selects full amount and owner");
  contact.event = 2;
  block->query(block, &contact);
  bh_block_damage_read(block, &state);
  CHECK(state.health == 30, "player-only query ignores contact");
  detach.owner = player;
  block->query(block, &detach);
  bh_block_damage_read(block, &state);
  CHECK(!state.owner, "player-only query detaches same actual owner");
  CHECK(bh_entities_dispose(&pool),
        "actual block/query/pool lifetime disposed");
}
static BHEntity *previous(BHEntity *e) {
  BHEntity *p;
  memcpy(&p, (char *)e + 0xec, 4);
  return p;
}
static BHEntity *next(BHEntity *e) {
  BHEntity *p;
  memcpy(&p, (char *)e + 0xf0, 4);
  return p;
}
typedef struct Packet {
  long event;
  unsigned long words[12];
} Packet;
static void radar_cases(void) {
  BHEntityPool pool;
  BHSoundQueue sound;
  BHSoundResource *resource;
  BHScriptLifetimeContext lifetime;
  BHRadarQueryContext context;
  BHEntity *a, *b, *c, *owner, *scheduled;
  Packet p, before;
  BHScriptLifetimeState state;
  BHVec3 zero = {0, 0, 0};
  unsigned long i, count;
  BHDebrisOwnerPacket detach;
  CHECK(bh_entities_init(&pool, 8, NULL) &&
            bh_sound_queue_init(&sound, NULL, NULL, NULL),
        "real radar pool and sound queue");
  resource = bh_sound_resource_allocate(&sound);
  CHECK(resource != NULL, "real delayed sound resource");
  if (!resource)
    return;
  resource->streaming = 1;
  resource->base_frequency = 8000;
  memset(&lifetime, 0, sizeof(lifetime));
  lifetime.timers.pool = &pool;
  lifetime.sounds = &sound;
  memset(&context, 0, sizeof(context));
  context.pool = &pool;
  context.lifetime = &lifetime;
  context.retired_sound = resource->index;
  a = bh_entity_acquire(&pool);
  b = bh_entity_acquire(&pool);
  c = bh_entity_acquire(&pool);
  owner = bh_entity_acquire(&pool);
  a->position.x = 1;
  a->position.y = 2;
  a->position.z = 3;
  CHECK(bh_radar_query_bind(&context, a, owner, 7) &&
            bh_radar_query_bind(&context, b, owner, 8) &&
            bh_radar_query_bind(&context, c, NULL, 0),
        "three real radar list members bound to whole query");
  CHECK(context.head == c && context.count == 3 && next(c) == b &&
            previous(b) == c && next(b) == a && previous(a) == b,
        "original reciprocal intrusive link fields");
  memset(&p, 0xa5, sizeof(p));
  p.event = 3;
  before = p;
  a->query(a, &p);
  CHECK(!memcmp(p.words, &a->position, 12) &&
            !memcmp(p.words + 3, before.words + 3, 36),
        "position event modifies only original three output words");
  memset(&p, 0xa5, sizeof(p));
  p.event = 16;
  a->query(a, &p);
  CHECK(!memcmp(p.words, &a->position, 12) && !memcmp(p.words + 3, &zero, 12) &&
            !memcmp(p.words + 6, &zero, 12) && p.words[9] == 0xa5a5a5a5,
        "full event16 emits position then both zero vectors without "
        "overrunning packet");
  p.event = 14;
  p.words[1] = 7;
  count = pool.used;
  b->query(b, &p);
  CHECK(context.count == 3 && pool.used == count,
        "nonmatching removalID ignored");
  p.words[1] = 8;
  b->query(b, &p);
  CHECK(context.count == 2 && context.head == c && next(c) == a &&
            previous(a) == c && pool.used == count - 1,
        "matching middle entry unlinks both lists and releases actual entity");
  p.words[1] = 0;
  c->query(c, &p);
  CHECK(context.count == 2,
        "zero removalID is disabled even when packet matches");
  detach.event = 17;
  detach.owner = a;
  a->query(a, &detach);
  CHECK(context.count == 2, "foreign watched owner ignored");
  detach.owner = owner;
  a->query(a, &detach);
  CHECK(context.count == 1 && context.head == c && !next(c) && !previous(c),
        "watched owner retirement removes tail through real release");
  scheduled = pool.active;
  CHECK(scheduled->step == bh_mission_sound_tick &&
            bh_script_lifetime_read(scheduled, &state) &&
            state.mission_countdown == 1.5f &&
            state.mission_resource == resource->index,
        "owner retirement acquires actual1.5-second mission sound with real "
        "context");
  count = pool.used;
  scheduled->step(scheduled);
  CHECK(pool.used == count && !resource->stream_head,
        "actual positive sound countdown queues nothing");
  for (i = 0; i < 50 && pool.used == count; ++i)
    scheduled->step(scheduled);
  CHECK(pool.used == count - 1 && resource->stream_head &&
            resource->stream_head->resource == resource,
        "actual restored sound tick queues real silent resource after delay "
        "and releases entity");
  p.event = 20;
  c->query(c, &p);
  CHECK(!context.head && !context.count && !context.error,
        "event20 removes final list head and reaches zero count");
  CHECK(bh_sound_queue_dispose(&sound) && bh_entities_dispose(&pool),
        "actual source owners disposed without Play");
}
int __cdecl bh_vm_queries_checkpoint(unsigned long *count) {
  BHMathEnvironment fp;
  checks = failures = 0;
  if (!bh_math_environment_enter(&fp))
    return 2;
  damage_cases();
  radar_cases();
  bh_math_environment_leave(&fp);
  if (count)
    *count = checks;
  printf("{\"scenario\":\"vm-queries\",\"checks\":%lu,\"failures\":%lu,"
         "\"original_code_required\":false,\"play_calls\":0}\n",
         checks, failures);
  return failures != 0;
}
