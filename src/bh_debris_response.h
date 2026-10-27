#ifndef BH_DEBRIS_RESPONSE_H
#define BH_DEBRIS_RESPONSE_H
#include "bh_physics_response.h"
#include "bh_sound_queue.h"

typedef struct BHDebrisContext BHDebrisContext;
typedef struct BHDebrisBody
{
    BHDebrisContext *context;
    BHEntity *entity, *owner;
    unsigned long generation, last_sound;
    BHPhysicsState *physics;
    BHVec3 inertia;
    float mass, friction, sleep_time;
} BHDebrisBody;
struct BHDebrisContext
{
    BHEntityPool *pool;
    BHSoundQueue *sound;
    unsigned long contact_sound;
    BHEntity **registry;
    unsigned long registry_capacity;
    int error;
};
typedef struct BHDebrisBlastPacket
{
    long event;
    BHVec3 center;
    float strength, radius;
} BHDebrisBlastPacket;
typedef struct BHDebrisOwnerPacket
{
    long event;
    BHEntity *owner;
} BHDebrisOwnerPacket;
/* Zero-initialize each body before its first bind. Do not replace callbacks or
   release a bound entity externally; unbind first or use deferred removal.
   Borrow stable source owners and a NULL-terminated entity registry (including
   its sentinel in capacity). Bodies/physics/context must outlive callbacks.
   Binding reserves the final pointer word in this source entity's payload;
   this is a source-only context bridge, not a claim about original field +35c.
   Original mass/inertia/friction/sleep/owner fields become explicit state.
   Caller keeps entity position and physics pose translation synchronized.
   Bind installs the real response query; it supplies no unrecovered tick or
   renderer. Existing real step/refresh callbacks remain caller-owned. */
int __cdecl bh_debris_context_init(BHDebrisContext *context, BHEntityPool *pool, BHSoundQueue *sound,
                                   unsigned long contact_sound, BHEntity **registry, unsigned long capacity);
int __cdecl bh_debris_context_dispose(BHDebrisContext *context);
int __cdecl bh_debris_bind(BHDebrisContext *context, BHDebrisBody *body, BHEntity *entity, BHPhysicsState *physics,
                           float mass, float friction, const BHVec3 *inertia);
int __cdecl bh_debris_unbind(BHDebrisBody *body);
/* Complete 0043c8f0: event 1 blast, 2 contact, 6 stop, 17 detach, 20 retire;
   other events leave state alone. Packets must have the corresponding layout.
   PC53 finite input contract. Original masked zero-distance/mass/inertia
   divisions are retained (may produce nonfinite results); not sanitized.
   Sound is queued after response/damping, before sleep reset; never played. */
void __cdecl bh_debris_receive(BHEntity *entity, void *packet);
/* Complete 0043c8b0: remove first registry occurrence, shift through sentinel,
   then release the entity even when absent from the registry. */
void __cdecl bh_debris_remove(BHEntity *entity);
#endif
