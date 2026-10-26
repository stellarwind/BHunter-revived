#ifndef BH_SCRIPT_MOTION_H
#define BH_SCRIPT_MOTION_H
#include "bh_script_lifetime.h"
typedef enum BHScriptMotionError {BH_MOTION_OK,BH_MOTION_ARGUMENT,
    BH_MOTION_LIFETIME,BH_MOTION_ATTACHMENTS,BH_MOTION_NUMERICAL} BHScriptMotionError;
typedef struct BHScriptMotionContext {
    BHScriptLifetimeContext *lifetime;
    BHScriptMotionError error;
} BHScriptMotionContext;
/* Field view for the script's movable target class only. This is distinct from
   autonomous route actors whose unrelated payload fields share these offsets.
   Actual storage remains in the owned BHEntity; no competing full layout. */
typedef struct BHScriptMotionActor {
    BHVec3 angles,minimum,maximum;
    BHMat34 local_to_world,world_to_local;
    BHEntity *attachments;
} BHScriptMotionActor;
int __cdecl bh_script_motion_actor_read(const BHEntity *,BHScriptMotionActor *);
int __cdecl bh_script_motion_actor_write(BHEntity *,const BHScriptMotionActor *);
BHEntity *__cdecl bh_script_motion_attachment_next(const BHEntity *);
void __cdecl bh_script_motion_attachment_link(BHEntity *,BHEntity *);
/* Count-zero source binding, not a partial script VM/constructor admission.
   Caller supplies already constructed target matrices, bounds and attachments,
   and (for translation) origin already including this delta as the real parser
   does. Context, actual lifetime context/pool and actor must outlive the step.
   Installs the actual admitted motion/sensor query and complete new step owner.
   Source guards reject nonmembers, self-targets and cyclic/foreign attachments
   before mutation. The actual step does not acquire resources or poll input. */
int __cdecl bh_script_motion_bind(BHScriptMotionContext *,BHEntity *,
    const BHScriptLifetimeState *,int rotate);
void __cdecl bh_script_translate_tick(BHEntity *); /*00443800*/
void __cdecl bh_script_rotate_tick(BHEntity *); /*004439a0*/
#endif
