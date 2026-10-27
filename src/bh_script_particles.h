#ifndef BH_SCRIPT_PARTICLES_H
#define BH_SCRIPT_PARTICLES_H
#include "bh_script_lifetime.h"
#include "bh_draw_queue.h"
typedef struct BHScriptParticleState
{
    unsigned long resource;
    float lifetime, angle, angular_step, size, size_step;
    float angular_decay, size_decay, alpha, alpha_step;
    unsigned long frame, bucket, reserved[3];
    BHVec3 velocity, acceleration;
} BHScriptParticleState;
typedef enum BHScriptParticleError
{
    BH_PARTICLE_OK,
    BH_PARTICLE_ARGUMENT,
    BH_PARTICLE_POOL,
    BH_PARTICLE_QUEUE,
    BH_PARTICLE_ACTOR,
    BH_PARTICLE_NUMERICAL
} BHScriptParticleError;
typedef struct BHScriptParticleContext
{
    BHEntityPool *pool;
    BHDrawQueue *queue;
    BHScriptParticleError error;
} BHScriptParticleContext;
/* Count-zero source context binding. Real pool/queue outlive all particles.
   The complete84-byte state begins at original entity+e8; private context is
   stored separately at payload+440. Original reserved bytes remain retained. */
int __cdecl bh_script_particles_init(BHScriptParticleContext *, BHEntityPool *, BHDrawQueue *);
BHScriptParticleState *__cdecl bh_script_particle_state(BHScriptParticleContext *, BHEntity *);
/* Whole004025c0. Position required, NULL velocity/acceleration mean zero.
   Constructor consumes one shared CRT rand AFTER successful allocation only.
   Size/angular rates scale by exact double1/30; opacity slope is
   (alpha*(-1/30))/lifetime with no intervening float store. Zero lifetime
   deliberately retains IEEE divide behavior under masked exceptions.
   Source handle is an added ownership return, not an original EAX claim. */
BHEntity *__cdecl bh_script_particle_create(BHScriptParticleContext *, unsigned long resource, unsigned long bucket,
                                            const BHVec3 *position, const BHVec3 *velocity, const BHVec3 *acceleration,
                                            float size, float angle, float size_rate, float angular_rate,
                                            float size_decay, float angular_decay, float lifetime, float alpha);
void __cdecl bh_script_particle_tick(BHEntity *);            /*004024c0*/
void __cdecl bh_script_particle_refresh(BHEntity *, void *); /*00402470*/

typedef struct BHScriptEmitterState
{
    BHEntity *actor;
    long script_loaded;
    float countdown, spread;
    BHVec3 velocity, acceleration;
    long size;
} BHScriptEmitterState;
typedef struct BHScriptEmitterContext
{
    BHScriptLifetimeContext *lifetime;
    BHScriptParticleContext *particles;
    unsigned long fountain_texture, steam_texture;
    BHScriptParticleError error;
} BHScriptEmitterContext;
/* Count-zero VM binding adapter. Actual actor table/activity/one-level owner,
   entity pool, queue, source textures and contexts remain borrowed. The real
   admitted sensor query performs immediate/deferred lifetime cleanup. */
int __cdecl bh_script_emitter_bind(BHScriptEmitterContext *, BHEntity *, const BHScriptEmitterState *, int steam);
int __cdecl bh_script_emitter_read(BHEntity *, BHScriptEmitterState *);
void __cdecl bh_script_fountain_tick(BHEntity *);         /*00443390*/
void __cdecl bh_script_steam_tick(BHEntity *);            /*004434b0*/
void __cdecl bh_script_steam_refresh(BHEntity *, void *); /*00443640*/
#endif
