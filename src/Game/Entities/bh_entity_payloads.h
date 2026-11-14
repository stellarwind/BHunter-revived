#ifndef BH_ENTITY_PAYLOADS_H
#define BH_ENTITY_PAYLOADS_H
#include "../../Math/bh_math.h"
#include <stddef.h>

struct BHEntity;
struct BHWorldGeometry;

/* These views overlap the original entity payload at +e4. A reserved range
   is deliberately unrecovered or belongs to another entity subtype; do not
   initialize it as part of assigning the recovered fields. */

typedef struct BHWorldEntityFields
{
    unsigned char reserved_0e4[0x4];
    long coordinates[3];
    unsigned long orientation_flags;
    unsigned long model_index;
    unsigned long radar_visited;
    unsigned char reserved_100[4];
    unsigned long activity_frames;
    unsigned char reserved_108[0xc];
    unsigned long query_flags;
    unsigned long placement_index;
    long city_id;
    struct BHEntity *previous;
    struct BHEntity *next;
    struct BHEntity *actors;
    struct BHEntity *attachments;
    float selection_phase;
    float editor_yaw;
    float editor_pitch;
    float editor_radius;
    float editor_pan_x;
    float editor_pan_z;
    float damage;
    struct BHEntity *damage_source;
    BHMat34 transform;
    BHMat34 inverse;
    BHVec3 minimum;
    BHVec3 maximum;
    struct BHEntity *owner;
    unsigned char reserved_1cc[0x168];
    short script_values[8];
} BHWorldEntityFields;

typedef struct BHScriptLifetimeFields
{
    unsigned char reserved_0e4[0x8];
    struct BHEntity *attachment_next;
    struct BHEntity *actor;
    long script_loaded;
    unsigned char reserved_0f8[0x4];
    unsigned long sound;
    unsigned char reserved_100[0x4];
    unsigned long mission_resource;
    unsigned char reserved_108[0x20];
    float countdown;
    unsigned char reserved_12c[0x4];
    BHVec3 delta;
    unsigned char reserved_13c[0x44];
    float mission_countdown;
} BHScriptLifetimeFields;

typedef struct BHScriptSensorFields
{
    unsigned char reserved_0e4[0xc];
    struct BHEntity *actor;
    long script_loaded;
    unsigned char reserved_0f8[0x4];
    long remaining;
    long threshold;
    long broadcast_radius;
    long expected;
    unsigned char reserved_10c[0x1c];
    float countdown;
    float latch;
} BHScriptSensorFields;

typedef struct BHScriptVMFields
{
    unsigned char reserved_0e4[0x8];
    struct BHEntity *next;
    unsigned char reserved_0f0[0x4];
    long loaded;
    long opcode;
    long args[5];
    long selectors[5];
    long variable_sum;
} BHScriptVMFields;

typedef struct BHExplosionFields
{
    unsigned char reserved_0e4[0x4];
    float age;
    float duration;
    unsigned char reserved_0f0[0x4];
    BHVec3 direction[8];
    float angle[8];
    float spin[8];
    float radius;
    float magnitude;
    float probability;
    unsigned char reserved_1a0[0xc];
    unsigned long sound_resource;
    unsigned long origin_tag;
    unsigned long target_identity;
    int visual_only;
} BHExplosionFields;

typedef struct BHEffectFields
{
    unsigned char reserved_0e4[0x4];
    float lifetime;
    float grace;
    float roll;
    float multiplier;
    BHVec3 velocity;
    BHVec3 acceleration;
    BHVec3 orbit_first;
    BHVec3 orbit_second;
    unsigned long sound;
    long split_count;
    unsigned long origin_tag;
    unsigned long sync_identity;
    struct BHWorldGeometry *world;
    struct BHEntity *ignore;
    struct BHEntity *target;
    struct BHEntity *source;
    unsigned long model_resource;
} BHEffectFields;

typedef struct BHProjectileFields
{
    unsigned char reserved_0e4[0x4];
    float countdown;
    unsigned char reserved_0ec[0x14];
    BHVec3 target_position;
    unsigned char reserved_10c[0xc];
    unsigned long sound;
    unsigned long identity;
    unsigned long sync_identity;
    struct BHWorldGeometry *world;
    struct BHEntity *ignore;
    struct BHEntity *target;
    struct BHEntity *secondary;
} BHProjectileFields;

typedef struct BHTrailEmitterFields
{
    unsigned char reserved_0e4[0x4];
    unsigned long attachment_generation;
    unsigned long bucket;
    unsigned long point_word;
    struct BHEntity *attached;
    struct BHEntity *newest;
    float lifetime;
    float decay;
    float interval;
    float node_lifetime;
    float width;
    float countdown;
    float width_step;
    float texture_scale;
    unsigned long image;
    BHVec3 drift;
} BHTrailEmitterFields;

typedef struct BHTrailNodeFields
{
    unsigned char reserved_0e4[0x4];
    struct BHEntity *newer;
    struct BHEntity *older;
    float alpha;
    float width;
    float texture_phase;
    float distance;
    float decay;
    float width_step;
    unsigned long bucket;
    unsigned long point_word;
    unsigned long image;
    BHVec3 drift;
} BHTrailNodeFields;

typedef struct BHHitMarkerFields
{
    unsigned char reserved_0e4[0x4];
    void (*draw)(struct BHEntity *);
    struct BHEntity *owner_older;
    struct BHEntity *owner_newer;
    struct BHEntity *owner;
    struct BHEntity *older;
    struct BHEntity *newer;
    float age;
    float lifetime;
    float size;
    float u;
    float v;
    BHVec3 normal;
    BHVec3 up;
} BHHitMarkerFields;

typedef struct BHTargetFields
{
    unsigned char reserved_0e4[0x8];
    float countdown;
    BHVec3 local_anchor;
    BHVec3 minimum;
    BHVec3 maximum;
    long emission;
    union {
        long number, token;
    };
    long loot;
    union {
        unsigned long flags;
        long counter;
    };
    unsigned long resource;
    struct BHEntity *owner;
} BHTargetFields;

typedef struct BHPuffFields
{
    unsigned char reserved_0e4[0x4];
    float lifetime;
    float duration;
    unsigned long image;
    unsigned char reserved_0f4[0xa0];
    float size;
    float delay;
    unsigned char reserved_19c[0xc];
    unsigned long seed;
} BHPuffFields;

typedef struct BHScriptDebrisFields
{
    unsigned char reserved_0e4[0x4];
    float lifetime;
    unsigned char reserved_0ec[0xa8];
    float angle;
    float spin;
    unsigned char reserved_19c[0x4];
    float width;
    float height;
    unsigned long frame;
    unsigned char reserved_1ac[0x4];
    unsigned long origin_tag;
} BHScriptDebrisFields;

typedef struct BHAttachedSoundFields
{
    unsigned char reserved_0e4[0xc];
    struct BHEntity *actor;
    long script_loaded;
    unsigned char reserved_0f8[0x4];
    unsigned long handle;
    unsigned long resource;
    float level;
    float target;
} BHAttachedSoundFields;

typedef struct BHScriptEmitterFields
{
    unsigned char reserved_0e4[0xc];
    struct BHEntity *actor;
    long script_loaded;
    unsigned char reserved_0f8[0x14];
    long size;
    unsigned char reserved_110[0x18];
    float countdown;
    float spread;
    BHVec3 velocity;
    BHVec3 acceleration;
} BHScriptEmitterFields;

static_assert(offsetof(BHWorldEntityFields, coordinates) == 0x4, "BHWorldEntityFields.coordinates offset");
static_assert(offsetof(BHWorldEntityFields, orientation_flags) == 0x10, "BHWorldEntityFields.orientation_flags offset");
static_assert(offsetof(BHWorldEntityFields, model_index) == 0x14, "BHWorldEntityFields.model_index offset");
static_assert(offsetof(BHWorldEntityFields, radar_visited) == 0x18, "BHWorldEntityFields.radar_visited offset");
static_assert(offsetof(BHWorldEntityFields, query_flags) == 0x30, "BHWorldEntityFields.query_flags offset");
static_assert(offsetof(BHWorldEntityFields, placement_index) == 0x34, "BHWorldEntityFields.placement_index offset");
static_assert(offsetof(BHWorldEntityFields, city_id) == 0x38, "BHWorldEntityFields.city_id offset");
static_assert(offsetof(BHWorldEntityFields, previous) == 0x3c, "BHWorldEntityFields.previous offset");
static_assert(offsetof(BHWorldEntityFields, next) == 0x40, "BHWorldEntityFields.next offset");
static_assert(offsetof(BHWorldEntityFields, attachments) == 0x48, "BHWorldEntityFields.attachments offset");
static_assert(offsetof(BHWorldEntityFields, selection_phase) == 0x4c, "BHWorldEntityFields.selection_phase offset");
static_assert(offsetof(BHWorldEntityFields, editor_yaw) == 0x50, "BHWorldEntityFields.editor_yaw offset");
static_assert(offsetof(BHWorldEntityFields, editor_pitch) == 0x54, "BHWorldEntityFields.editor_pitch offset");
static_assert(offsetof(BHWorldEntityFields, editor_radius) == 0x58, "BHWorldEntityFields.editor_radius offset");
static_assert(offsetof(BHWorldEntityFields, editor_pan_x) == 0x5c, "BHWorldEntityFields.editor_pan_x offset");
static_assert(offsetof(BHWorldEntityFields, editor_pan_z) == 0x60, "BHWorldEntityFields.editor_pan_z offset");
static_assert(offsetof(BHWorldEntityFields, damage) == 0x64, "BHWorldEntityFields.damage offset");
static_assert(offsetof(BHWorldEntityFields, damage_source) == 0x68, "BHWorldEntityFields.damage_source offset");
static_assert(offsetof(BHWorldEntityFields, transform) == 0x6c, "BHWorldEntityFields.transform offset");
static_assert(offsetof(BHWorldEntityFields, inverse) == 0x9c, "BHWorldEntityFields.inverse offset");
static_assert(offsetof(BHWorldEntityFields, minimum) == 0xcc, "BHWorldEntityFields.minimum offset");
static_assert(offsetof(BHWorldEntityFields, maximum) == 0xd8, "BHWorldEntityFields.maximum offset");
static_assert(offsetof(BHWorldEntityFields, owner) == 0xe4, "BHWorldEntityFields.owner offset");
static_assert(offsetof(BHWorldEntityFields, script_values) == 0x250, "BHWorldEntityFields.script_values offset");
static_assert(sizeof(BHWorldEntityFields) == 0x260, "BHWorldEntityFields size");
static_assert(offsetof(BHScriptLifetimeFields, attachment_next) == 0x8,
              "BHScriptLifetimeFields.attachment_next offset");
static_assert(offsetof(BHScriptLifetimeFields, actor) == 0xc, "BHScriptLifetimeFields.actor offset");
static_assert(offsetof(BHScriptLifetimeFields, script_loaded) == 0x10, "BHScriptLifetimeFields.script_loaded offset");
static_assert(offsetof(BHScriptLifetimeFields, sound) == 0x18, "BHScriptLifetimeFields.sound offset");
static_assert(offsetof(BHScriptLifetimeFields, mission_resource) == 0x20,
              "BHScriptLifetimeFields.mission_resource offset");
static_assert(offsetof(BHScriptLifetimeFields, countdown) == 0x44, "BHScriptLifetimeFields.countdown offset");
static_assert(offsetof(BHScriptLifetimeFields, delta) == 0x4c, "BHScriptLifetimeFields.delta offset");
static_assert(offsetof(BHScriptLifetimeFields, mission_countdown) == 0x9c,
              "BHScriptLifetimeFields.mission_countdown offset");
static_assert(sizeof(BHScriptLifetimeFields) == 0xa0, "BHScriptLifetimeFields size");
static_assert(offsetof(BHScriptSensorFields, actor) == 0xc, "BHScriptSensorFields.actor offset");
static_assert(offsetof(BHScriptSensorFields, script_loaded) == 0x10, "BHScriptSensorFields.script_loaded offset");
static_assert(offsetof(BHScriptSensorFields, remaining) == 0x18, "BHScriptSensorFields.remaining offset");
static_assert(offsetof(BHScriptSensorFields, threshold) == 0x1c, "BHScriptSensorFields.threshold offset");
static_assert(offsetof(BHScriptSensorFields, broadcast_radius) == 0x20, "BHScriptSensorFields.broadcast_radius offset");
static_assert(offsetof(BHScriptSensorFields, expected) == 0x24, "BHScriptSensorFields.expected offset");
static_assert(offsetof(BHScriptSensorFields, countdown) == 0x44, "BHScriptSensorFields.countdown offset");
static_assert(offsetof(BHScriptSensorFields, latch) == 0x48, "BHScriptSensorFields.latch offset");
static_assert(sizeof(BHScriptSensorFields) == 0x4c, "BHScriptSensorFields size");
static_assert(offsetof(BHScriptVMFields, next) == 0x8, "BHScriptVMFields.next offset");
static_assert(offsetof(BHScriptVMFields, loaded) == 0x10, "BHScriptVMFields.loaded offset");
static_assert(offsetof(BHScriptVMFields, opcode) == 0x14, "BHScriptVMFields.opcode offset");
static_assert(offsetof(BHScriptVMFields, args) == 0x18, "BHScriptVMFields.args offset");
static_assert(offsetof(BHScriptVMFields, selectors) == 0x2c, "BHScriptVMFields.selectors offset");
static_assert(offsetof(BHScriptVMFields, variable_sum) == 0x40, "BHScriptVMFields.variable_sum offset");
static_assert(sizeof(BHScriptVMFields) == 0x44, "BHScriptVMFields size");
static_assert(offsetof(BHExplosionFields, age) == 0x4, "BHExplosionFields.age offset");
static_assert(offsetof(BHExplosionFields, duration) == 0x8, "BHExplosionFields.duration offset");
static_assert(offsetof(BHExplosionFields, direction) == 0x10, "BHExplosionFields.direction offset");
static_assert(offsetof(BHExplosionFields, angle) == 0x70, "BHExplosionFields.angle offset");
static_assert(offsetof(BHExplosionFields, spin) == 0x90, "BHExplosionFields.spin offset");
static_assert(offsetof(BHExplosionFields, radius) == 0xb0, "BHExplosionFields.radius offset");
static_assert(offsetof(BHExplosionFields, magnitude) == 0xb4, "BHExplosionFields.magnitude offset");
static_assert(offsetof(BHExplosionFields, probability) == 0xb8, "BHExplosionFields.probability offset");
static_assert(offsetof(BHExplosionFields, sound_resource) == 0xc8, "BHExplosionFields.sound_resource offset");
static_assert(offsetof(BHExplosionFields, origin_tag) == 0xcc, "BHExplosionFields.origin_tag offset");
static_assert(offsetof(BHExplosionFields, target_identity) == 0xd0, "BHExplosionFields.target_identity offset");
static_assert(offsetof(BHExplosionFields, visual_only) == 0xd4, "BHExplosionFields.visual_only offset");
static_assert(sizeof(BHExplosionFields) == 0xd8, "BHExplosionFields size");
static_assert(offsetof(BHEffectFields, lifetime) == 0x4, "BHEffectFields.lifetime offset");
static_assert(offsetof(BHEffectFields, grace) == 0x8, "BHEffectFields.grace offset");
static_assert(offsetof(BHEffectFields, roll) == 0xc, "BHEffectFields.roll offset");
static_assert(offsetof(BHEffectFields, multiplier) == 0x10, "BHEffectFields.multiplier offset");
static_assert(offsetof(BHEffectFields, velocity) == 0x14, "BHEffectFields.velocity offset");
static_assert(offsetof(BHEffectFields, acceleration) == 0x20, "BHEffectFields.acceleration offset");
static_assert(offsetof(BHEffectFields, orbit_first) == 0x2c, "BHEffectFields.orbit_first offset");
static_assert(offsetof(BHEffectFields, orbit_second) == 0x38, "BHEffectFields.orbit_second offset");
static_assert(offsetof(BHEffectFields, sound) == 0x44, "BHEffectFields.sound offset");
static_assert(offsetof(BHEffectFields, split_count) == 0x48, "BHEffectFields.split_count offset");
static_assert(offsetof(BHEffectFields, origin_tag) == 0x4c, "BHEffectFields.origin_tag offset");
static_assert(offsetof(BHEffectFields, sync_identity) == 0x50, "BHEffectFields.sync_identity offset");
static_assert(offsetof(BHEffectFields, world) == 0x54, "BHEffectFields.world offset");
static_assert(offsetof(BHEffectFields, ignore) == 0x58, "BHEffectFields.ignore offset");
static_assert(offsetof(BHEffectFields, target) == 0x5c, "BHEffectFields.target offset");
static_assert(offsetof(BHEffectFields, source) == 0x60, "BHEffectFields.source offset");
static_assert(offsetof(BHEffectFields, model_resource) == 0x64, "BHEffectFields.model_resource offset");
static_assert(sizeof(BHEffectFields) == 0x68, "BHEffectFields size");
static_assert(offsetof(BHProjectileFields, countdown) == 0x4, "BHProjectileFields.countdown offset");
static_assert(offsetof(BHProjectileFields, target_position) == 0x1c, "BHProjectileFields.target_position offset");
static_assert(offsetof(BHProjectileFields, sound) == 0x34, "BHProjectileFields.sound offset");
static_assert(offsetof(BHProjectileFields, identity) == 0x38, "BHProjectileFields.identity offset");
static_assert(offsetof(BHProjectileFields, sync_identity) == 0x3c, "BHProjectileFields.sync_identity offset");
static_assert(offsetof(BHProjectileFields, world) == 0x40, "BHProjectileFields.world offset");
static_assert(offsetof(BHProjectileFields, ignore) == 0x44, "BHProjectileFields.ignore offset");
static_assert(offsetof(BHProjectileFields, target) == 0x48, "BHProjectileFields.target offset");
static_assert(offsetof(BHProjectileFields, secondary) == 0x4c, "BHProjectileFields.secondary offset");
static_assert(sizeof(BHProjectileFields) == 0x50, "BHProjectileFields size");
static_assert(offsetof(BHTrailEmitterFields, attachment_generation) == 0x4,
              "BHTrailEmitterFields.attachment_generation offset");
static_assert(offsetof(BHTrailEmitterFields, bucket) == 0x8, "BHTrailEmitterFields.bucket offset");
static_assert(offsetof(BHTrailEmitterFields, point_word) == 0xc, "BHTrailEmitterFields.point_word offset");
static_assert(offsetof(BHTrailEmitterFields, attached) == 0x10, "BHTrailEmitterFields.attached offset");
static_assert(offsetof(BHTrailEmitterFields, newest) == 0x14, "BHTrailEmitterFields.newest offset");
static_assert(offsetof(BHTrailEmitterFields, lifetime) == 0x18, "BHTrailEmitterFields.lifetime offset");
static_assert(offsetof(BHTrailEmitterFields, decay) == 0x1c, "BHTrailEmitterFields.decay offset");
static_assert(offsetof(BHTrailEmitterFields, interval) == 0x20, "BHTrailEmitterFields.interval offset");
static_assert(offsetof(BHTrailEmitterFields, node_lifetime) == 0x24, "BHTrailEmitterFields.node_lifetime offset");
static_assert(offsetof(BHTrailEmitterFields, width) == 0x28, "BHTrailEmitterFields.width offset");
static_assert(offsetof(BHTrailEmitterFields, countdown) == 0x2c, "BHTrailEmitterFields.countdown offset");
static_assert(offsetof(BHTrailEmitterFields, width_step) == 0x30, "BHTrailEmitterFields.width_step offset");
static_assert(offsetof(BHTrailEmitterFields, texture_scale) == 0x34, "BHTrailEmitterFields.texture_scale offset");
static_assert(offsetof(BHTrailEmitterFields, image) == 0x38, "BHTrailEmitterFields.image offset");
static_assert(offsetof(BHTrailEmitterFields, drift) == 0x3c, "BHTrailEmitterFields.drift offset");
static_assert(sizeof(BHTrailEmitterFields) == 0x48, "BHTrailEmitterFields size");
static_assert(offsetof(BHTrailNodeFields, newer) == 0x4, "BHTrailNodeFields.newer offset");
static_assert(offsetof(BHTrailNodeFields, older) == 0x8, "BHTrailNodeFields.older offset");
static_assert(offsetof(BHTrailNodeFields, alpha) == 0xc, "BHTrailNodeFields.alpha offset");
static_assert(offsetof(BHTrailNodeFields, width) == 0x10, "BHTrailNodeFields.width offset");
static_assert(offsetof(BHTrailNodeFields, texture_phase) == 0x14, "BHTrailNodeFields.texture_phase offset");
static_assert(offsetof(BHTrailNodeFields, distance) == 0x18, "BHTrailNodeFields.distance offset");
static_assert(offsetof(BHTrailNodeFields, decay) == 0x1c, "BHTrailNodeFields.decay offset");
static_assert(offsetof(BHTrailNodeFields, width_step) == 0x20, "BHTrailNodeFields.width_step offset");
static_assert(offsetof(BHTrailNodeFields, bucket) == 0x24, "BHTrailNodeFields.bucket offset");
static_assert(offsetof(BHTrailNodeFields, point_word) == 0x28, "BHTrailNodeFields.point_word offset");
static_assert(offsetof(BHTrailNodeFields, image) == 0x2c, "BHTrailNodeFields.image offset");
static_assert(offsetof(BHTrailNodeFields, drift) == 0x30, "BHTrailNodeFields.drift offset");
static_assert(sizeof(BHTrailNodeFields) == 0x3c, "BHTrailNodeFields size");
static_assert(offsetof(BHHitMarkerFields, draw) == 0x4, "BHHitMarkerFields.draw offset");
static_assert(offsetof(BHHitMarkerFields, owner_older) == 0x8, "BHHitMarkerFields.owner_older offset");
static_assert(offsetof(BHHitMarkerFields, owner_newer) == 0xc, "BHHitMarkerFields.owner_newer offset");
static_assert(offsetof(BHHitMarkerFields, owner) == 0x10, "BHHitMarkerFields.owner offset");
static_assert(offsetof(BHHitMarkerFields, older) == 0x14, "BHHitMarkerFields.older offset");
static_assert(offsetof(BHHitMarkerFields, newer) == 0x18, "BHHitMarkerFields.newer offset");
static_assert(offsetof(BHHitMarkerFields, age) == 0x1c, "BHHitMarkerFields.age offset");
static_assert(offsetof(BHHitMarkerFields, lifetime) == 0x20, "BHHitMarkerFields.lifetime offset");
static_assert(offsetof(BHHitMarkerFields, size) == 0x24, "BHHitMarkerFields.size offset");
static_assert(offsetof(BHHitMarkerFields, u) == 0x28, "BHHitMarkerFields.u offset");
static_assert(offsetof(BHHitMarkerFields, v) == 0x2c, "BHHitMarkerFields.v offset");
static_assert(offsetof(BHHitMarkerFields, normal) == 0x30, "BHHitMarkerFields.normal offset");
static_assert(offsetof(BHHitMarkerFields, up) == 0x3c, "BHHitMarkerFields.up offset");
static_assert(sizeof(BHHitMarkerFields) == 0x48, "BHHitMarkerFields size");
static_assert(offsetof(BHTargetFields, countdown) == 0x8, "BHTargetFields.countdown offset");
static_assert(offsetof(BHTargetFields, local_anchor) == 0xc, "BHTargetFields.local_anchor offset");
static_assert(offsetof(BHTargetFields, minimum) == 0x18, "BHTargetFields.minimum offset");
static_assert(offsetof(BHTargetFields, maximum) == 0x24, "BHTargetFields.maximum offset");
static_assert(offsetof(BHTargetFields, emission) == 0x30, "BHTargetFields.emission offset");
static_assert(offsetof(BHTargetFields, number) == 0x34, "BHTargetFields.number offset");
static_assert(offsetof(BHTargetFields, loot) == 0x38, "BHTargetFields.loot offset");
static_assert(offsetof(BHTargetFields, flags) == 0x3c, "BHTargetFields.flags offset");
static_assert(offsetof(BHTargetFields, resource) == 0x40, "BHTargetFields.resource offset");
static_assert(offsetof(BHTargetFields, owner) == 0x44, "BHTargetFields.owner offset");
static_assert(sizeof(BHTargetFields) == 0x48, "BHTargetFields size");
static_assert(offsetof(BHPuffFields, lifetime) == 0x4, "BHPuffFields.lifetime offset");
static_assert(offsetof(BHPuffFields, duration) == 0x8, "BHPuffFields.duration offset");
static_assert(offsetof(BHPuffFields, image) == 0xc, "BHPuffFields.image offset");
static_assert(offsetof(BHPuffFields, size) == 0xb0, "BHPuffFields.size offset");
static_assert(offsetof(BHPuffFields, delay) == 0xb4, "BHPuffFields.delay offset");
static_assert(offsetof(BHPuffFields, seed) == 0xc4, "BHPuffFields.seed offset");
static_assert(sizeof(BHPuffFields) == 0xc8, "BHPuffFields size");
static_assert(offsetof(BHScriptDebrisFields, lifetime) == 0x4, "BHScriptDebrisFields.lifetime offset");
static_assert(offsetof(BHScriptDebrisFields, angle) == 0xb0, "BHScriptDebrisFields.angle offset");
static_assert(offsetof(BHScriptDebrisFields, spin) == 0xb4, "BHScriptDebrisFields.spin offset");
static_assert(offsetof(BHScriptDebrisFields, width) == 0xbc, "BHScriptDebrisFields.width offset");
static_assert(offsetof(BHScriptDebrisFields, height) == 0xc0, "BHScriptDebrisFields.height offset");
static_assert(offsetof(BHScriptDebrisFields, frame) == 0xc4, "BHScriptDebrisFields.frame offset");
static_assert(offsetof(BHScriptDebrisFields, origin_tag) == 0xcc, "BHScriptDebrisFields.origin_tag offset");
static_assert(sizeof(BHScriptDebrisFields) == 0xd0, "BHScriptDebrisFields size");
static_assert(offsetof(BHAttachedSoundFields, actor) == 0xc, "BHAttachedSoundFields.actor offset");
static_assert(offsetof(BHAttachedSoundFields, script_loaded) == 0x10, "BHAttachedSoundFields.script_loaded offset");
static_assert(offsetof(BHAttachedSoundFields, handle) == 0x18, "BHAttachedSoundFields.handle offset");
static_assert(offsetof(BHAttachedSoundFields, resource) == 0x1c, "BHAttachedSoundFields.resource offset");
static_assert(offsetof(BHAttachedSoundFields, level) == 0x20, "BHAttachedSoundFields.level offset");
static_assert(offsetof(BHAttachedSoundFields, target) == 0x24, "BHAttachedSoundFields.target offset");
static_assert(sizeof(BHAttachedSoundFields) == 0x28, "BHAttachedSoundFields size");
static_assert(offsetof(BHScriptEmitterFields, actor) == 0xc, "BHScriptEmitterFields.actor offset");
static_assert(offsetof(BHScriptEmitterFields, script_loaded) == 0x10, "BHScriptEmitterFields.script_loaded offset");
static_assert(offsetof(BHScriptEmitterFields, size) == 0x28, "BHScriptEmitterFields.size offset");
static_assert(offsetof(BHScriptEmitterFields, countdown) == 0x44, "BHScriptEmitterFields.countdown offset");
static_assert(offsetof(BHScriptEmitterFields, spread) == 0x48, "BHScriptEmitterFields.spread offset");
static_assert(offsetof(BHScriptEmitterFields, velocity) == 0x4c, "BHScriptEmitterFields.velocity offset");
static_assert(offsetof(BHScriptEmitterFields, acceleration) == 0x58, "BHScriptEmitterFields.acceleration offset");
static_assert(sizeof(BHScriptEmitterFields) == 0x64, "BHScriptEmitterFields size");
typedef struct BHAreaProjectileFields
{
    unsigned char reserved_0e4[0x4];
    float strength;
    float radius;
    unsigned long sound;
    unsigned long identity;
    struct BHWorldGeometry *world;
    struct BHEntity *ignore;
    unsigned char reserved_100[0x4];
    struct BHEntity *source;
} BHAreaProjectileFields;
static_assert(offsetof(BHAreaProjectileFields, strength) == 0x4, "BHAreaProjectileFields.strength offset");
static_assert(offsetof(BHAreaProjectileFields, radius) == 0x8, "BHAreaProjectileFields.radius offset");
static_assert(offsetof(BHAreaProjectileFields, sound) == 0xc, "BHAreaProjectileFields.sound offset");
static_assert(offsetof(BHAreaProjectileFields, identity) == 0x10, "BHAreaProjectileFields.identity offset");
static_assert(offsetof(BHAreaProjectileFields, world) == 0x14, "BHAreaProjectileFields.world offset");
static_assert(offsetof(BHAreaProjectileFields, ignore) == 0x18, "BHAreaProjectileFields.ignore offset");
static_assert(offsetof(BHAreaProjectileFields, source) == 0x20, "BHAreaProjectileFields.source offset");
static_assert(sizeof(BHAreaProjectileFields) == 0x24, "BHAreaProjectileFields size");

typedef struct BHPulseFields
{
    unsigned char reserved_0e4[0x8];
    float phase;
    float phase_step;
    BHVec3 velocity;
    unsigned char reserved_100[0xc];
    BHVec3 base;
} BHPulseFields;
static_assert(offsetof(BHPulseFields, phase) == 0x8, "BHPulseFields.phase offset");
static_assert(offsetof(BHPulseFields, phase_step) == 0xc, "BHPulseFields.phase_step offset");
static_assert(offsetof(BHPulseFields, velocity) == 0x10, "BHPulseFields.velocity offset");
static_assert(offsetof(BHPulseFields, base) == 0x28, "BHPulseFields.base offset");
static_assert(sizeof(BHPulseFields) == 0x34, "BHPulseFields size");

struct BHPlayerInputList;
#pragma pack(push, 4)
typedef struct BHMenuFields
{
    unsigned char reserved_0e4[0xa4];
    struct BHPlayerInputList *input_list;
    unsigned char reserved_18c[0x30];
    unsigned long input_phase;
    unsigned long attempts;
    unsigned char reserved_1c4[0x8];
    unsigned long row_count;
    unsigned char reserved_1d0[0x28];
    double transition_time;
    unsigned char reserved_200[0x84];
    unsigned long tag;
} BHMenuFields;
#pragma pack(pop)
static_assert(offsetof(BHMenuFields, input_list) == 0xa4, "BHMenuFields.input_list offset");
static_assert(offsetof(BHMenuFields, input_phase) == 0xd8, "BHMenuFields.input_phase offset");
static_assert(offsetof(BHMenuFields, attempts) == 0xdc, "BHMenuFields.attempts offset");
static_assert(offsetof(BHMenuFields, row_count) == 0xe8, "BHMenuFields.row_count offset");
static_assert(offsetof(BHMenuFields, transition_time) == 0x114, "BHMenuFields.transition_time offset");
static_assert(offsetof(BHMenuFields, tag) == 0x1a0, "BHMenuFields.tag offset");
static_assert(sizeof(BHMenuFields) == 0x1a4, "BHMenuFields size");

static_assert(offsetof(BHWorldEntityFields, activity_frames) == 0x20, "BHWorldEntityFields.activity_frames offset");
typedef struct BHFlashFields
{
    unsigned char reserved[4];
    float countdown, strength, size;
} BHFlashFields;
static_assert(offsetof(BHFlashFields, countdown) == 4, "BHFlashFields.countdown offset");
static_assert(offsetof(BHFlashFields, strength) == 8, "BHFlashFields.strength offset");
static_assert(offsetof(BHFlashFields, size) == 12, "BHFlashFields.size offset");
static_assert(sizeof(BHFlashFields) == 16, "BHFlashFields size");
static_assert(offsetof(BHWorldEntityFields, actors) == 0x44, "BHWorldEntityFields.actors offset");
#endif
