#include "bh_vm_queries.h"
#include "bh_fp.h"
#include "bh_round.h"
#include <math.h>
#include <string.h>
static int live(BHEntityPool *p, const BHEntity *e) {
  BHEntity *at;
  unsigned long n = 0;
  if (!p)
    return 0;
  for (at = p->active; at && n++ < p->capacity; at = at->next)
    if (at == e)
      return 1;
  return 0;
}
static void put(BHEntity *e, unsigned long o, const void *v, unsigned long n) {
  memcpy((char *)e + o, v, n);
}
static void get(const BHEntity *e, unsigned long o, void *v, unsigned long n) {
  memcpy(v, (const char *)e + o, n);
}
static BHBlockDamageContext *damage_context(BHEntity *e) {
  BHBlockDamageContext *c;
  memcpy(&c, e->payload + 600, 4);
  return c;
}
int __cdecl bh_block_damage_read(const BHEntity *e, BHBlockDamageState *s) {
  if (!e || !s)
    return 0;
  get(e, 0x148, &s->health, 4);
  get(e, 0x14c, &s->owner, 4);
  return 1;
}
int __cdecl bh_block_damage_bind(BHEntityPool *p, BHEntity *e,
                                 BHBlockDamageContext *c,
                                 const BHBlockDamageState *s, int only) {
  if (!live(p, e) || !c || !c->world || !s)
    return 0;
  put(e, 0x148, &s->health, 4);
  put(e, 0x14c, &s->owner, 4);
  memcpy(e->payload + 600, &c, 4);
  return bh_entity_set_query(
      p, e, only ? bh_block_player_damage_query : bh_block_damage_query);
}
static void damage(BHEntity *e, float value) {
  float health;
  get(e, 0x148, &health, 4);
  bh_round_float(&health, (double)health - (double)value * (-10.0f));
  put(e, 0x148, &health, 4);
}
void __cdecl bh_block_player_damage_query(BHEntity *e, void *packet) {
  long event;
  BHEntity *owner, *watched;
  float amount;
  BHBlockDamageContext *c = damage_context(e);
  memcpy(&event, packet, 4);
  if (event == 0) {
    memcpy(&owner, (char *)packet + 8, 4);
    if (owner == c->player) {
      memcpy(&amount, (char *)packet + 4, 4);
      damage(e, amount);
      put(e, 0x14c, &owner, 4);
    }
  } else if (event == 17) {
    get(e, 0x14c, &watched, 4);
    memcpy(&owner, (char *)packet + 4, 4);
    if (owner == watched) {
      owner = NULL;
      put(e, 0x14c, &owner, 4);
    }
  }
}
void __cdecl bh_block_damage_query(BHEntity *e, void *packet) {
  long event;
  BHEntity *owner, *watched;
  float amount, health, distance, x, y, z;
  double dx, dy, dz, length;
  BHDebrisBlastPacket blast;
  BHBlockDamageContext *c = damage_context(e);
  memcpy(&event, packet, 4);
  switch (event) {
  case 0:
    memcpy(&amount, (char *)packet + 4, 4);
    damage(e, amount);
    memcpy(&owner, (char *)packet + 8, 4);
    put(e, 0x14c, &owner, 4);
    return;
  case 2:
    memcpy(&amount, (char *)packet + 0x20, 4);
    damage(e, amount);
    return;
  case 17:
    get(e, 0x14c, &watched, 4);
    memcpy(&owner, (char *)packet + 4, 4);
    if (owner == watched) {
      owner = NULL;
      put(e, 0x14c, &owner, 4);
    }
    return;
  case 1:
    memcpy(&blast, packet, sizeof(blast));
    /* Original X midpoint is spilled; Y midpoint stays extended until its
       delta is formed. Z midpoint spills before subtraction. Each delta is
       stored before a retained extended copy multiplies that binary32 spill. */
    bh_round_float(&x,
                   ((double)c->world->minimum.x + c->world->maximum.x) * .5);
    bh_round_float(&z,
                   ((double)c->world->minimum.z + c->world->maximum.z) * .5);
    dx = (double)x - blast.center.x;
    dy = ((double)c->world->minimum.y + c->world->maximum.y) * .5 -
         blast.center.y;
    dz = (double)z - blast.center.z;
    bh_round_float(&x, dx);
    bh_round_float(&y, dy);
    bh_round_float(&z, dz);
    length = sqrt((dx * x + dy * y) + dz * z);
    bh_round_float(&distance, length);
    if (distance > 0 && distance < blast.radius) {
      get(e, 0x148, &health, 4);
      bh_round_float(&health, (double)health -
                                  ((double)blast.strength *
                                   (1.0 - (double)distance / blast.radius)) *
                                      (-.0008));
      put(e, 0x148, &health, 4);
    }
    return;
  default:
    return;
  }
}
static BHRadarQueryContext *radar_context(BHEntity *e) {
  BHRadarQueryContext *c;
  memcpy(&c, e->payload + 596, 4);
  return c;
}
static BHEntity *link(const BHEntity *e, unsigned long offset) {
  BHEntity *p;
  get(e, offset, &p, 4);
  return p;
}
int __cdecl bh_radar_query_bind(BHRadarQueryContext *c, BHEntity *e,
                                BHEntity *owner, long id) {
  BHEntity *zero = NULL;
  if (!c || !c->lifetime || c->lifetime->timers.pool != c->pool ||
      !c->lifetime->sounds || !live(c->pool, e) || c->count >= 65535)
    return 0;
  put(e, 0xec, &zero, 4);
  put(e, 0xf0, &c->head, 4);
  if (c->head)
    put(c->head, 0xec, &e, 4);
  put(e, 0x11c, &id, 4);
  put(e, 0x1b8, &owner, 4);
  memcpy(e->payload + 596, &c, 4);
  if (!bh_entity_set_query(c->pool, e, bh_radar_entity_query))
    return 0;
  c->head = e;
  ++c->count;
  return 1;
}
static int retire(BHRadarQueryContext *c, BHEntity *e) {
  BHEntity *prev = link(e, 0xec), *next = link(e, 0xf0);
  if (!c->count || !live(c->pool, e) ||
      (prev && (!live(c->pool, prev) || link(prev, 0xf0) != e)) ||
      (next && (!live(c->pool, next) || link(next, 0xec) != e)) ||
      (!prev && c->head != e)) {
    c->error = 1;
    return 0;
  }
  if (prev)
    put(prev, 0xf0, &next, 4);
  else
    c->head = next;
  if (next)
    put(next, 0xec, &prev, 4);
  --c->count;
  if (!bh_entity_release(c->pool, e)) {
    c->error = 1;
    return 0;
  }
  return 1;
}
void __cdecl bh_radar_entity_query(BHEntity *e, void *packet) {
  BHRadarQueryContext *c = radar_context(e);
  long event, id, matched;
  BHEntity *owner, *incoming, *sound;
  BHScriptLifetimeState state;
  BHVec3 zero = {0, 0, 0};
  memcpy(&event, packet, 4);
  switch (event) {
  case 3:
    memcpy((char *)packet + 4, &e->position, 12);
    return;
  case 16:
    memcpy((char *)packet + 4, &e->position, 12);
    memcpy((char *)packet + 16, &zero, 12);
    memcpy((char *)packet + 28, &zero, 12);
    return;
  case 14:
    get(e, 0x11c, &id, 4);
    memcpy(&matched, (char *)packet + 8, 4);
    if (id && matched == id)
      retire(c, e);
    return;
  case 20:
    retire(c, e);
    return;
  case 17:
    get(e, 0x1b8, &owner, 4);
    memcpy(&incoming, (char *)packet + 4, 4);
    if (owner != incoming)
      return;
    if (!retire(c, e))
      return;
    sound = bh_entity_acquire(c->pool);
    if (!sound)
      return;
    memset(&state, 0, sizeof(state));
    state.mission_resource = c->retired_sound;
    state.mission_countdown = 1.5f;
    if (!bh_script_lifetime_bind(c->lifetime, sound, &state, NULL)) {
      bh_entity_release(c->pool, sound);
      c->error = 1;
      return;
    }
    sound->step = bh_mission_sound_tick;
    return;
  default:
    return;
  }
}
