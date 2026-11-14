#ifndef BH_ENTITY_BINDINGS_H
#define BH_ENTITY_BINDINGS_H
#include <stddef.h>

/* Source-owned callback bridges, not recovered executable fields. These
   bindings borrow their contexts. Entity kinds deliberately reuse slots;
   only bindings installed together by a kind's constructor may coexist.
   The view overlays payload bytes without adding storage to BHEntity. */
struct BHActorAttackContext;
struct BHActorConstructContext;
struct BHActorDamageContext;
struct BHActorResponseContext;
struct BHAreaProjectileContext;
struct BHBlockDamageContext;
struct BHContactEffects;
struct BHDebrisBody;
struct BHEditorControl;
struct BHEditorInput;
struct BHEditorNode;
struct BHEffectCleanupContext;
struct BHEffectProjectileContext;
struct BHEffectPuffContext;
struct BHEntityPool;
struct BHEntityScheduleRoom;
struct BHFlashFlareContext;
struct BHHitMarkerContext;
struct BHHomingProjectileContext;
struct BHMenuRuntime;
struct BHMenuTransition;
struct BHMissionStory;
struct BHPickupContext;
struct BHPlayerTick;
struct BHProjectileDrawContext;
struct BHProjectileLifetime;
struct BHPulseProjectileContext;
struct BHRadarQueryContext;
struct BHRemotePackets;
struct BHRemoteTick;
struct BHScriptActorContext;
struct BHScriptDebrisContext;
struct BHScriptEmitterContext;
struct BHScriptExplosionContext;
struct BHScriptLifetimeContext;
struct BHScriptMotionContext;
struct BHScriptParticleContext;
struct BHScriptSensorContext;
struct BHScriptTimerContext;
struct BHScriptVMContext;
struct BHTargetConstructSlot;
struct BHTargetStepContext;
struct BHTrailContext;

struct BHInputKeyCapture;
struct BHInputTextCapture;

typedef struct BHEntityBindings
{
    union {
        struct BHInputKeyCapture *key_capture;
        struct BHInputTextCapture *text_capture;
    };
    unsigned char reserved_004[48];
    struct BHScriptTimerContext *script_timer;
    unsigned long wait_generation;
    unsigned char reserved_03c[4];
    union {
        struct BHContactEffects *contact_effects;
        struct BHPickupContext *pickup;
    };
    unsigned char reserved_044[332];
    struct BHScriptLifetimeContext *script_lifetime;
    unsigned char reserved_194[4];
    struct BHScriptMotionContext *script_motion;
    unsigned char reserved_19c[4];
    struct BHScriptSensorContext *script_sensor;
    union {
        unsigned long sensor_generation;
        struct BHEditorInput *editor_input;
    };
    unsigned char reserved_1a8[16];
    struct BHScriptParticleContext *script_particles;
    unsigned char reserved_1bc[4];
    struct BHScriptEmitterContext *script_emitter;
    unsigned char reserved_1c4[4];
    struct BHEntityPool *flash_pool;
    unsigned char reserved_1cc[4];
    struct BHFlashFlareContext *flash_flare;
    struct BHProjectileLifetime *projectile_lifetime;
    union {
        unsigned long lifetime_generation;
        struct BHScriptDebrisContext *script_debris;
    };
    unsigned char reserved_1dc[52];
    struct BHPlayerTick *player_tick;
    unsigned char reserved_214[4];
    struct BHRemotePackets *remote_packets;
    struct BHRemoteTick *remote_tick;
    unsigned char reserved_220[4];
    struct BHMissionStory *mission_story;
    unsigned char reserved_228[4];
    struct BHEditorControl *editor_control;
    union {
        struct BHEditorNode *editor_node;
        struct BHScriptActorContext *script_actor;
    };
    unsigned char reserved_234[12];
    struct BHScriptVMContext *script_vm;
    struct BHScriptVMContext *radar_vm;
    struct BHActorConstructContext *actor_construct;
    struct BHActorResponseContext *actor_response;
    unsigned char reserved_250[4];
    union {
        struct BHActorAttackContext *actor_attack;
        struct BHRadarQueryContext *radar_query;
    };
    union {
        struct BHActorDamageContext *actor_damage;
        struct BHAreaProjectileContext *area_projectile;
        struct BHPulseProjectileContext *pulse_projectile;
        struct BHTrailContext *trail;
        struct BHMenuTransition *menu_transition;
        struct BHBlockDamageContext *block_damage;
    };
    union {
        struct BHEffectCleanupContext *effect_cleanup;
        unsigned long transition_generation;
    };
    union {
        struct BHHomingProjectileContext *homing_projectile;
        struct BHEffectProjectileContext *effect_projectile;
        struct BHEntityPool *expiry_pool;
        struct BHEntityScheduleRoom *room_activity;
        struct BHMenuRuntime *menu_runtime;
        struct BHTargetConstructSlot *target_construct;
    };
    unsigned long projectile_generation;
    union {
        struct BHProjectileDrawContext *projectile_draw;
        struct BHTargetStepContext *target_step;
    };
    unsigned char reserved_26c[4];
    union {
        struct BHEffectPuffContext *effect_puff;
        struct BHHitMarkerContext *hit_marker;
        struct BHScriptExplosionContext *explosion;
    };
    unsigned char reserved_274[4];
    struct BHDebrisBody *debris_body;
} BHEntityBindings;

static_assert(offsetof(BHEntityBindings, script_timer) == 52, "BHEntityBindings.script_timer offset");
static_assert(offsetof(BHEntityBindings, wait_generation) == 56, "BHEntityBindings.wait_generation offset");
static_assert(offsetof(BHEntityBindings, contact_effects) == 64, "BHEntityBindings.contact_effects offset");
static_assert(offsetof(BHEntityBindings, pickup) == 64, "BHEntityBindings.pickup offset");
static_assert(offsetof(BHEntityBindings, script_lifetime) == 400, "BHEntityBindings.script_lifetime offset");
static_assert(offsetof(BHEntityBindings, script_motion) == 408, "BHEntityBindings.script_motion offset");
static_assert(offsetof(BHEntityBindings, script_sensor) == 416, "BHEntityBindings.script_sensor offset");
static_assert(offsetof(BHEntityBindings, sensor_generation) == 420, "BHEntityBindings.sensor_generation offset");
static_assert(offsetof(BHEntityBindings, editor_input) == 420, "BHEntityBindings.editor_input offset");
static_assert(offsetof(BHEntityBindings, script_particles) == 440, "BHEntityBindings.script_particles offset");
static_assert(offsetof(BHEntityBindings, script_emitter) == 448, "BHEntityBindings.script_emitter offset");
static_assert(offsetof(BHEntityBindings, flash_pool) == 456, "BHEntityBindings.flash_pool offset");
static_assert(offsetof(BHEntityBindings, flash_flare) == 464, "BHEntityBindings.flash_flare offset");
static_assert(offsetof(BHEntityBindings, projectile_lifetime) == 468, "BHEntityBindings.projectile_lifetime offset");
static_assert(offsetof(BHEntityBindings, lifetime_generation) == 472, "BHEntityBindings.lifetime_generation offset");
static_assert(offsetof(BHEntityBindings, script_debris) == 472, "BHEntityBindings.script_debris offset");
static_assert(offsetof(BHEntityBindings, player_tick) == 528, "BHEntityBindings.player_tick offset");
static_assert(offsetof(BHEntityBindings, remote_packets) == 536, "BHEntityBindings.remote_packets offset");
static_assert(offsetof(BHEntityBindings, remote_tick) == 540, "BHEntityBindings.remote_tick offset");
static_assert(offsetof(BHEntityBindings, mission_story) == 548, "BHEntityBindings.mission_story offset");
static_assert(offsetof(BHEntityBindings, editor_control) == 556, "BHEntityBindings.editor_control offset");
static_assert(offsetof(BHEntityBindings, editor_node) == 560, "BHEntityBindings.editor_node offset");
static_assert(offsetof(BHEntityBindings, script_actor) == 560, "BHEntityBindings.script_actor offset");
static_assert(offsetof(BHEntityBindings, script_vm) == 576, "BHEntityBindings.script_vm offset");
static_assert(offsetof(BHEntityBindings, radar_vm) == 580, "BHEntityBindings.radar_vm offset");
static_assert(offsetof(BHEntityBindings, actor_construct) == 584, "BHEntityBindings.actor_construct offset");
static_assert(offsetof(BHEntityBindings, actor_response) == 588, "BHEntityBindings.actor_response offset");
static_assert(offsetof(BHEntityBindings, actor_attack) == 596, "BHEntityBindings.actor_attack offset");
static_assert(offsetof(BHEntityBindings, radar_query) == 596, "BHEntityBindings.radar_query offset");
static_assert(offsetof(BHEntityBindings, actor_damage) == 600, "BHEntityBindings.actor_damage offset");
static_assert(offsetof(BHEntityBindings, area_projectile) == 600, "BHEntityBindings.area_projectile offset");
static_assert(offsetof(BHEntityBindings, pulse_projectile) == 600, "BHEntityBindings.pulse_projectile offset");
static_assert(offsetof(BHEntityBindings, trail) == 600, "BHEntityBindings.trail offset");
static_assert(offsetof(BHEntityBindings, menu_transition) == 600, "BHEntityBindings.menu_transition offset");
static_assert(offsetof(BHEntityBindings, block_damage) == 600, "BHEntityBindings.block_damage offset");
static_assert(offsetof(BHEntityBindings, effect_cleanup) == 604, "BHEntityBindings.effect_cleanup offset");
static_assert(offsetof(BHEntityBindings, transition_generation) == 604,
              "BHEntityBindings.transition_generation offset");
static_assert(offsetof(BHEntityBindings, homing_projectile) == 608, "BHEntityBindings.homing_projectile offset");
static_assert(offsetof(BHEntityBindings, effect_projectile) == 608, "BHEntityBindings.effect_projectile offset");
static_assert(offsetof(BHEntityBindings, expiry_pool) == 608, "BHEntityBindings.expiry_pool offset");
static_assert(offsetof(BHEntityBindings, room_activity) == 608, "BHEntityBindings.room_activity offset");
static_assert(offsetof(BHEntityBindings, menu_runtime) == 608, "BHEntityBindings.menu_runtime offset");
static_assert(offsetof(BHEntityBindings, target_construct) == 608, "BHEntityBindings.target_construct offset");
static_assert(offsetof(BHEntityBindings, projectile_generation) == 612,
              "BHEntityBindings.projectile_generation offset");
static_assert(offsetof(BHEntityBindings, projectile_draw) == 616, "BHEntityBindings.projectile_draw offset");
static_assert(offsetof(BHEntityBindings, target_step) == 616, "BHEntityBindings.target_step offset");
static_assert(offsetof(BHEntityBindings, effect_puff) == 624, "BHEntityBindings.effect_puff offset");
static_assert(offsetof(BHEntityBindings, hit_marker) == 624, "BHEntityBindings.hit_marker offset");
static_assert(offsetof(BHEntityBindings, explosion) == 624, "BHEntityBindings.explosion offset");
static_assert(offsetof(BHEntityBindings, debris_body) == 632, "BHEntityBindings.debris_body offset");
static_assert(sizeof(BHEntityBindings) == 636, "BHEntityBindings size");
static_assert(offsetof(BHEntityBindings, key_capture) == 0, "BHEntityBindings.key_capture offset");
static_assert(offsetof(BHEntityBindings, text_capture) == 0, "BHEntityBindings.text_capture offset");
#endif
