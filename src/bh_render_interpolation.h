#ifndef BH_RENDER_INTERPOLATION_H
#define BH_RENDER_INTERPOLATION_H
#include "bh_draw_queue.h"
#define BH_INTERPOLATION_CAPACITY 3500
#define BH_INTERPOLATION_HASH 8192
/* Snapshots own command bytes only. Resource handles always come from the live
   current tick; no rendering callback can mutate either snapshot or simulation. */
typedef struct BHDrawIdentity
{
    unsigned long owner, generation, ordinal;
    /* One-based model command to which this world-space HUD piece is attached. */
    unsigned long attachment;
    /* New trail heads begin at the previous position of their older node. */
    unsigned long older_owner, older_generation;
} BHDrawIdentity;
typedef struct BHRenderInterpolation
{
    BHDrawCommand previous[BH_INTERPOLATION_CAPACITY], current[BH_INTERPOLATION_CAPACITY],
        output[BH_INTERPOLATION_CAPACITY];
    BHDrawIdentity previous_ids[BH_INTERPOLATION_CAPACITY], current_ids[BH_INTERPOLATION_CAPACITY];
    /* Render-consumer state belongs to a publication, not to its immutable
       motion snapshot. Flare execution normalizes this value in place. */
    unsigned long flare_strength[BH_INTERPOLATION_CAPACITY];
    unsigned long previous_count, current_count, hash[BH_INTERPOLATION_HASH];
    long scene;
    int ready, sampled;
    BHDrawQueue queue;
} BHRenderInterpolation;
typedef struct BHSimulationClock
{
    double last, remainder;
    int ready, sampled;
} BHSimulationClock;
unsigned long __cdecl bh_simulation_advance(BHSimulationClock *, double seconds);
void __cdecl bh_interpolation_bind(BHDrawQueue *);
void __cdecl bh_interpolation_reset(void);
unsigned long __cdecl bh_interpolation_mark(void);
/* Nested draw owners stamp first. Enclosing callbacks preserve those identities. */
void __cdecl bh_interpolation_stamp(unsigned long start, unsigned long identity, unsigned long generation);
void __cdecl bh_interpolation_trail_point(unsigned long index, unsigned long owner, unsigned long generation,
                                         unsigned long older_owner, unsigned long older_generation, unsigned long image);
void __cdecl bh_interpolation_attach(unsigned long start, unsigned long end, unsigned long model);
void __cdecl bh_interpolation_capture(BHRenderInterpolation *, const BHDrawQueue *, long scene);
BHDrawQueue *__cdecl bh_interpolation_sample(BHRenderInterpolation *, float alpha);
/* Retain only mutable consumer fields after the sampled queue is executed. */
void __cdecl bh_interpolation_rendered(BHRenderInterpolation *);
#endif
