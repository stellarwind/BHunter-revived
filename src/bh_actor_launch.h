#ifndef BH_ACTOR_LAUNCH_H
#define BH_ACTOR_LAUNCH_H
#include "bh_actor_construct.h"
typedef struct BHActorLaunchContext
{
    BHActorConstructContext *actors;
    int error;
} BHActorLaunchContext;
/* Single-player path of 0043bca0. Launch consumes two shared CRT draws. */
BHEntity *__cdecl bh_actor_launch(BHActorLaunchContext *, BHWorldGeometry *, unsigned long type, long token,
                                  float speed, long death_tag);
#endif
