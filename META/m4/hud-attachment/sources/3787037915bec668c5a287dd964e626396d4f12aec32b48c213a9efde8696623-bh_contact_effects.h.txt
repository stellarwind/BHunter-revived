#ifndef BH_CONTACT_EFFECTS_H
#define BH_CONTACT_EFFECTS_H
#include "bh_entities.h"
#include "bh_draw_queue.h"

typedef struct BHSparkState {
    float lifetime,angle,spin,width,height;
    unsigned long frame;
    float unused[3];
    BHVec3 velocity;
} BHSparkState;
typedef struct BHImpactState {
    unsigned long unused[6];
    float age,duration,size;
    unsigned long spare[2];
    BHVec3 normal;
} BHImpactState;
typedef enum BHContactEffectError {
    BH_EFFECT_OK,BH_EFFECT_ARGUMENT,BH_EFFECT_CONTEXT,BH_EFFECT_NUMERICAL,BH_EFFECT_POOL,BH_EFFECT_QUEUE
} BHContactEffectError;
typedef struct BHContactEffects {
    BHEntityPool *pool;
    BHDrawQueue *queue;
    unsigned long spark_texture,quad_texture,rays_texture,line_texture;
    int has_camera;
    float camera_yaw;
    BHContactEffectError error;
} BHContactEffects;
/* Borrow real source pool/queue. Context must outlive every effect it owns.
   Texture identifiers and optional camera yaw are explicit caller bindings.
   Generated commands preserve original selectors/data; no renderer callback
   pointers, texture objects or Direct3D backend implementations are fabricated. */
int __cdecl bh_contact_effects_init(BHContactEffects *context,BHEntityPool *pool,
    BHDrawQueue *queue,unsigned long spark,unsigned long quad,unsigned long rays,unsigned long line);
int __cdecl bh_contact_effects_dispose(BHContactEffects *context);
BHSparkState *__cdecl bh_contact_spark_state(BHContactEffects *context,BHEntity *entity);
BHImpactState *__cdecl bh_contact_impact_state(BHContactEffects *context,BHEntity *entity);
/* 00407510 installs actual 004073f0 tick / 004073b0 refresh callbacks.
   Successful allocation consumes exactly eight shared CRT random draws.
   Return source handle added for ownership, not an original EAX claim. */
BHEntity *__cdecl bh_contact_spark_create(BHContactEffects *context,const BHVec3 *point,
    float lifetime,float speed);
/* Source adapter for the verified physics special-surface allocation fields.
   flags bit 2 takes priority over 4, selects 16-unit quad vs 12-unit rays.
   Real callbacks: age 00407ad0 and refresh 00407670 / 00407720. No audio is queued
   here; the full physics effects/audio/network orchestration remains WIP. */
BHEntity *__cdecl bh_contact_impact_create(BHContactEffects *context,const BHVec3 *point,
    const BHVec3 *normal,unsigned long flags);
#endif
