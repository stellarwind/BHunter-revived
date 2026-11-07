#ifndef BH_REMOTE_TICK_H
#define BH_REMOTE_TICK_H
#include "bh_remote_fire.h"
typedef struct BHRemoteTick
{
    BHRemoteFire *fire;
    BHWorldSegmentContext *world;
    BHScriptParticleContext *particles;
    BHSoundQueue *sound;
    unsigned long smoke_image;
    unsigned long calls, scheduled, ring_particles, smoke_particles;
    int error;
} BHRemoteTick;
/* Whole00425df0. Nine actual12-byte signed16 firing channels, effect countdown,
   predicted-position surface-ring/periodic smoke and original six-component
   acceleration/velocity damping. Uses existing source prediction, trace, firing,
   particle and queued-voice owners. No actor/game loop or OS input is driven.
   Private source context at payload+540 installs the real step callback.
   PC53 finite moderate fields, coherent borrowed world and actor lifetimes;
   source guards preserve previous channel/effect changes on later failure. */
int __cdecl bh_remote_tick(BHRemoteTick *, BHEntity *);
int __cdecl bh_remote_tick_bind(BHRemoteTick *, BHEntity *);
void __cdecl bh_remote_step(BHEntity *);
#endif
